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
