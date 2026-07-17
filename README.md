# pageDB

A disk-oriented database storage engine built in C++, following the CMU 15-445 course structure as a portfolio project.

## What it does
Implements the foundational layer of a disk-based DBMS:
- **Page** — fixed-size (4096 byte) unit of data with a small header (page_id, page_type, free_space_offset)
- **DiskManager** — persists pages to a backing file using `pread`/`pwrite` at computed byte offsets
- **BufferPool** — caches pages in RAM (fixed-size frame table), using LRU eviction when full, flushing dirty pages back to disk before reuse

## Build & run tests
```bash
g++ tests/test_page.cpp -Iinclude -o test_page && ./test_page
g++ tests/test_disk_manager.cpp src/disk_manager.cpp -Iinclude -o test_disk_manager && ./test_disk_manager
g++ tests/test_lru_replacer.cpp src/lru_replacer.cpp -Iinclude -o test_lru_replacer && ./test_lru_replacer
g++ tests/test_buffer_pool.cpp src/buffer_pool.cpp src/disk_manager.cpp src/lru_replacer.cpp -Iinclude -o test_buffer_pool && ./test_buffer_pool
g++ tests/test_buffer_pool_stress.cpp src/buffer_pool.cpp src/disk_manager.cpp src/lru_replacer.cpp -Iinclude -o test_buffer_pool_stress && ./test_buffer_pool_stress
```

## Design decisions

**Why 4KB pages?** Small enough to avoid wasting I/O and buffer pool memory when only a small piece of data changes; large enough to amortize the fixed overhead of each disk I/O operation and keep related data together for locality. It also matches the OS's own virtual memory page size. Not a universal law — Postgres uses 8KB — but 4KB is a well-tested sweet spot.

**Why LRU eviction?** Simple, well-understood, O(1) with a list + hash map, and a reasonable proxy for "which page is least likely to be needed again soon" without needing workload-specific tuning.

**Why a single backing file instead of one file per page?** Matches how real DBMSs work — avoids filesystem overhead of managing millions of tiny files, and lets page location be computed directly (`page_id * PAGE_SIZE`) instead of needing a filesystem lookup per page.

## Checkpoint: Why 4KB pages and not 1 byte or 1MB?

Too small (e.g. 1 byte): every read/write becomes its own disk I/O operation, or at minimum its own bookkeeping entry (page table entry, header, pin count) — the metadata overhead would dwarf the actual data.

Too large (e.g. 1MB): a single small update wastes huge I/O bandwidth and buffer pool memory rewriting/reading far more than needed, and one pinned page could occupy a disproportionate share of the entire buffer pool.

4KB (or a small multiple of it) historically matches the OS's own virtual memory page size, and for spinning disks roughly matched a reasonable unit of sequential transfer relative to seek overhead. It's the sweet spot: small enough to avoid waste on small updates, large enough to amortize per-I/O overhead and keep related data together for locality. This is a tunable trade-off, not a universal law — Postgres, for instance, uses 8KB.

## B+ Tree

### Node layout (on-disk page format)
Each node is a fixed-size `Page` wrapped by `BPlusTreeNode`:

| Field        | Offset                          | Notes                          |
|--------------|----------------------------------|---------------------------------|
| is_leaf      | PAGE_HEADER_SIZE                | 1 byte                         |
| num_keys     | PAGE_HEADER_SIZE + 1            | uint32_t                       |
| parent_id    | PAGE_HEADER_SIZE + 5            | uint32_t                       |
| keys[]       | PAGE_HEADER_SIZE + 9            | MAX_KEYS_PER_NODE (3) x key_t  |
| values[] / children[] | after keys[]           | leaf: value_t values; internal: page_id_t children (same offset, reused) |
| next_leaf_id | after values[]/children[]        | leaf-only, forms the leaf linked list |

`MAX_KEYS_PER_NODE = 3` — kept small deliberately for correctness-first
development; not tuned for real fanout yet.

### Insert
Descends like `Search`. If the target leaf has room, inserts directly
(sorted, shifting elements). If the leaf is full, `SplitLeaf` runs:
splits the (n+1) sorted keys/values in half, writes left half back to
the original page, right half to a new page, links `next_leaf_id`, and
returns a promoted key (copied) + new sibling page_id.

That result propagates up via `InsertRecursive`'s return value. Each
ancestor either absorbs the new (key, child_id) pair or itself
overflows and splits via `SplitInternal` (promoted key is *removed*,
not copied, since internal nodes have no value slot). If the split
reaches the top, `Insert()` allocates a brand new root with the
promoted key and the two halves as children — this is the only way
tree height grows.

**Duplicate key policy:** rejected. Inserting an existing key leaves
the original value untouched and does not modify the tree.

### Search
Standard root-to-leaf descent using `FindChildIndex` (first key
strictly greater than the search key determines which child to
follow). Leaf is scanned linearly for an exact match.

### RangeScan
Descends to the leaf containing `start`, then walks the leaf linked
list via `next_leaf_id`, collecting all (key, value) pairs with
`start <= key <= end`, stopping once a key exceeds `end` or the list
ends. This is what makes ordered range queries cheap without
revisiting internal nodes.

### Testing
- `test_bplus_tree_insert.cpp` / `_search.cpp` — Day 2/3 happy path
- `test_bplus_tree_split.cpp` — single leaf split + new root
- `test_bplus_tree_split_internal.cpp` — 2-level split, hand-traced
- `test_bplus_tree_stress.cpp` — 10k random inserts, sequential
  inserts, empty-tree first insert, duplicate rejection, full and
  partial range scans
