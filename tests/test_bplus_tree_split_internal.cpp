#include "../include/bplus_tree.h"
#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>

int main() {
    DiskManager dm("test_bplus_split_internal.db");
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

    // Ascending inserts: forces repeated rightmost-leaf splits until the
    // root (an internal node) itself overflows and must split.
    for (int k = 10; k <= 100; k += 10) {
        tree.Insert(k, k * 10);
    }
    printf("[PASS] Inserted 10 keys (10..100) without crashing\n");

    // Top-level root must now be a NEW internal node with exactly 1 key.
    page_id_t top_root_id = root_id;
    Page* top_page = bpm.FetchPage(top_root_id);
    BPlusTreeNode top_root(top_page);
    assert(!top_root.IsLeaf());
    assert(top_root.GetNumKeys() == 1);
    key_t top_key = top_root.GetKeyAt(0);
    printf("[PASS] Top root is internal with 1 key (promoted_key=%d)\n", top_key);
    assert(top_key == 70);
    printf("[PASS] Promoted key at top matches hand-traced value (70)\n");

    page_id_t r_left_id = top_root.GetChildAt(0);
    page_id_t r_right_id = top_root.GetChildAt(1);
    bpm.UnpinPage(top_root_id, false);

    // R_left: internal node, 2 keys (30, 50), 3 children.
    Page* left_page = bpm.FetchPage(r_left_id);
    BPlusTreeNode r_left(left_page);
    assert(!r_left.IsLeaf());
    assert(r_left.GetNumKeys() == 2);
    assert(r_left.GetKeyAt(0) == 30 && r_left.GetKeyAt(1) == 50);
    printf("[PASS] R_left is internal with keys 30, 50\n");
    page_id_t leaf1 = r_left.GetChildAt(0);
    page_id_t leaf2 = r_left.GetChildAt(1);
    page_id_t leaf3 = r_left.GetChildAt(2);
    uint32_t r_left_parent = r_left.GetParentId();
    bpm.UnpinPage(r_left_id, false);
    assert(r_left_parent == top_root_id);
    printf("[PASS] R_left's parent_id correctly points at top root\n");

    // R_right: internal node, 1 key (90), 2 children.
    Page* right_page = bpm.FetchPage(r_right_id);
    BPlusTreeNode r_right(right_page);
    assert(!r_right.IsLeaf());
    assert(r_right.GetNumKeys() == 1);
    assert(r_right.GetKeyAt(0) == 90);
    printf("[PASS] R_right is internal with key 90\n");
    page_id_t leaf4 = r_right.GetChildAt(0);
    page_id_t leaf5 = r_right.GetChildAt(1);
    uint32_t r_right_parent = r_right.GetParentId();
    bpm.UnpinPage(r_right_id, false);
    assert(r_right_parent == top_root_id);
    printf("[PASS] R_right's parent_id correctly points at top root\n");

    // Critical check: leaf4 and leaf5 moved to R_right during SplitInternal,
    // so their parent_id must have been updated to r_right_id (not r_left_id
    // or the old root id). This is the re-parenting step that's easy to miss.
    Page* l4_page = bpm.FetchPage(leaf4);
    BPlusTreeNode l4(l4_page);
    assert(l4.IsLeaf());
    assert(l4.GetNumKeys() == 2);
    assert(l4.GetKeyAt(0) == 70 && l4.GetKeyAt(1) == 80);
    uint32_t l4_parent = l4.GetParentId();
    bpm.UnpinPage(leaf4, false);
    assert(l4_parent == r_right_id);
    printf("[PASS] Leaf (70,80) correctly re-parented to R_right\n");

    Page* l5_page = bpm.FetchPage(leaf5);
    BPlusTreeNode l5(l5_page);
    assert(l5.IsLeaf());
    assert(l5.GetNumKeys() == 2);
    assert(l5.GetKeyAt(0) == 90 && l5.GetKeyAt(1) == 100);
    uint32_t l5_parent = l5.GetParentId();
    bpm.UnpinPage(leaf5, false);
    assert(l5_parent == r_right_id);
    printf("[PASS] Leaf (90,100) correctly re-parented to R_right\n");

    // Sanity on the three leaves that stayed under R_left.
    Page* l1_page = bpm.FetchPage(leaf1);
    BPlusTreeNode l1(l1_page);
    assert(l1.GetKeyAt(0) == 10 && l1.GetKeyAt(1) == 20);
    bpm.UnpinPage(leaf1, false);

    Page* l2_page = bpm.FetchPage(leaf2);
    BPlusTreeNode l2(l2_page);
    assert(l2.GetKeyAt(0) == 30 && l2.GetKeyAt(1) == 40);
    bpm.UnpinPage(leaf2, false);

    Page* l3_page = bpm.FetchPage(leaf3);
    BPlusTreeNode l3(l3_page);
    assert(l3.GetKeyAt(0) == 50 && l3.GetKeyAt(1) == 60);
    bpm.UnpinPage(leaf3, false);
    printf("[PASS] All three R_left leaves have correct keys\n");

    // Search must now descend through TWO internal levels correctly.
    value_t v;
    for (int k = 10; k <= 100; k += 10) {
        assert(tree.Search(k, &v) && v == k * 10);
    }
    printf("[PASS] Search finds all 10 keys through the 2-level internal tree\n");

    key_t not_inserted[] = {5, 15, 35, 65, 95, 105};
    for (key_t k : not_inserted) {
        assert(!tree.Search(k, &v));
    }
    printf("[PASS] Search correctly reports non-inserted keys as not found\n");

    // RangeScan must walk the leaf linked list across all 5 leaves,
    // which only works if next_leaf_id survived every split correctly.
    auto results = tree.RangeScan(0, 1000);
    assert(results.size() == 10);
    for (size_t i = 0; i < results.size(); i++) {
        key_t expected_key = (int)(i + 1) * 10;
        assert(results[i].first == expected_key);
        assert(results[i].second == expected_key * 10);
    }
    printf("[PASS] RangeScan(0,1000) returns all 10 keys in order, no gaps/dupes\n");

    printf("\nAll Day 4 Step 3 (SplitInternal) tests passed!\n");
    return 0;
}
