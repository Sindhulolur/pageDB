#include "../include/bplus_tree.h"
#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>

int main() {
    DiskManager dm("test_bplus_search.db");
    BufferPool bpm(20, &dm);

    // Manually build: root (internal) -> 2 leaves
    // Root has 1 key: 20. child[0] = leaf A (keys < 20), child[1] = leaf B (keys >= 20)
    page_id_t root_id = dm.AllocatePage();
    page_id_t leaf_a_id = dm.AllocatePage();
    page_id_t leaf_b_id = dm.AllocatePage();

    // Build leaf A: keys 10, 15 -> values 100, 150. next_leaf = leaf B.
    {
        Page* page = bpm.FetchPage(leaf_a_id);
        BPlusTreeNode node(page);
        node.SetIsLeaf(true);
        node.SetNumKeys(2);
        node.SetKeyAt(0, 10); node.SetValueAt(0, 100);
        node.SetKeyAt(1, 15); node.SetValueAt(1, 150);
        node.SetNextLeafId(leaf_b_id);
        bpm.UnpinPage(leaf_a_id, true);
    }

    // Build leaf B: keys 20, 25, 30 -> values 200, 250, 300. no next leaf (0).
    {
        Page* page = bpm.FetchPage(leaf_b_id);
        BPlusTreeNode node(page);
        node.SetIsLeaf(true);
        node.SetNumKeys(3);
        node.SetKeyAt(0, 20); node.SetValueAt(0, 200);
        node.SetKeyAt(1, 25); node.SetValueAt(1, 250);
        node.SetKeyAt(2, 30); node.SetValueAt(2, 300);
        node.SetNextLeafId(0);
        bpm.UnpinPage(leaf_b_id, true);
    }

    // Build root: 1 key (20), 2 children (leaf A, leaf B)
    {
        Page* page = bpm.FetchPage(root_id);
        BPlusTreeNode node(page);
        node.SetIsLeaf(false);
        node.SetNumKeys(1);
        node.SetKeyAt(0, 20);
        node.SetChildAt(0, leaf_a_id);
        node.SetChildAt(1, leaf_b_id);
        bpm.UnpinPage(root_id, true);
    }

    BPlusTree tree(&bpm, root_id);

    // --- Search tests ---
    value_t v;
    assert(tree.Search(10, &v) && v == 100);
    printf("[PASS] Search(10) = %d\n", v);

    assert(tree.Search(25, &v) && v == 250);
    printf("[PASS] Search(25) = %d\n", v);

    assert(tree.Search(20, &v) && v == 200);  // exact boundary key
    printf("[PASS] Search(20) [boundary key] = %d\n", v);

    assert(!tree.Search(99, &v));
    printf("[PASS] Search(99) correctly not found\n");

    assert(!tree.Search(12, &v));
    printf("[PASS] Search(12) [gap in leaf A] correctly not found\n");

    // --- Range scan across the leaf boundary (leaf A -> leaf B) ---
    auto results = tree.RangeScan(12, 27);
    // Expected: 15, 20, 25 (10 excluded, 30 excluded)
    assert(results.size() == 3);
    assert(results[0].first == 15);
    assert(results[1].first == 20);
    assert(results[2].first == 25);
    printf("[PASS] RangeScan(12, 27) across leaf boundary returned: ");
    for (auto& p : results) printf("(%d,%d) ", p.first, p.second);
    printf("\n");

    printf("\nAll Day 2 search/range_scan tests passed!\n");
    return 0;
}
