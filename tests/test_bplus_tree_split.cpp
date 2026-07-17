#include "../include/bplus_tree.h"
#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>

int main() {
    DiskManager dm("test_bplus_split.db");
    BufferPool bpm(20, &dm);

    page_id_t root_id = dm.AllocatePage();
    {
        Page* page = bpm.FetchPage(root_id);
        BPlusTreeNode node(page);
        node.SetIsLeaf(true);
        node.SetNumKeys(0);
        node.SetParentId(0);
        node.SetNextLeafId(0);
        bpm.UnpinPage(root_id, true);
    }

    BPlusTree tree(&bpm, &dm, &root_id);

    // Fill the leaf to exact capacity (Day 3 happy path — no split yet).
    tree.Insert(20, 200);
    tree.Insert(10, 100);
    tree.Insert(30, 300);
    printf("[PASS] Filled leaf to capacity (3 keys) without splitting\n");

    // One more insert must trigger SplitLeaf + new-root creation.
    tree.Insert(40, 400);
    printf("[PASS] 4th insert completed without crashing\n");

    // root_id should have changed — it now points at a new internal node.
    page_id_t new_root_id = root_id;
    Page* root_page = bpm.FetchPage(new_root_id);
    BPlusTreeNode root_node(root_page);
    assert(!root_node.IsLeaf());
    assert(root_node.GetNumKeys() == 1);
    printf("[PASS] New root created: internal node with 1 key (promoted_key=%d)\n",
           root_node.GetKeyAt(0));

    page_id_t left_id = root_node.GetChildAt(0);
    page_id_t right_id = root_node.GetChildAt(1);
    bpm.UnpinPage(new_root_id, false);

    // Left leaf should have 2 keys, right leaf should have 2 keys (4 total, split 2/2).
    Page* left_page = bpm.FetchPage(left_id);
    BPlusTreeNode left_node(left_page);
    assert(left_node.IsLeaf());
    assert(left_node.GetNumKeys() == 2);
    printf("[PASS] Left leaf has %d keys: %d, %d\n",
           left_node.GetNumKeys(), left_node.GetKeyAt(0), left_node.GetKeyAt(1));
    uint32_t next_id = left_node.GetNextLeafId();
    bpm.UnpinPage(left_id, false);

    assert(next_id == right_id);
    printf("[PASS] Left leaf's next_leaf_id correctly points at right leaf\n");

    Page* right_page = bpm.FetchPage(right_id);
    BPlusTreeNode right_node(right_page);
    assert(right_node.IsLeaf());
    assert(right_node.GetNumKeys() == 2);
    printf("[PASS] Right leaf has %d keys: %d, %d\n",
           right_node.GetNumKeys(), right_node.GetKeyAt(0), right_node.GetKeyAt(1));
    bpm.UnpinPage(right_id, false);

    // Search must now descend through the new internal root correctly.
    value_t v;
    assert(tree.Search(10, &v) && v == 100);
    assert(tree.Search(20, &v) && v == 200);
    assert(tree.Search(30, &v) && v == 300);
    assert(tree.Search(40, &v) && v == 400);
    printf("[PASS] Search finds all 4 keys correctly through the new internal root\n");

    // RangeScan must cross the leaf boundary via next_leaf_id.
    auto results = tree.RangeScan(0, 100);
    assert(results.size() == 4);
    assert(results[0].first == 10 && results[1].first == 20 &&
           results[2].first == 30 && results[3].first == 40);
    printf("[PASS] RangeScan(0,100) returns all 4 keys in order across the split\n");

    printf("\nAll Day 4 split tests passed!\n");
    return 0;
}
