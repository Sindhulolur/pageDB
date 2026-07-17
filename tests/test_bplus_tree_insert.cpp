#include "../include/bplus_tree.h"
#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>

int main() {
    DiskManager dm("test_bplus_insert.db");
    BufferPool bpm(20, &dm);

    // Start with a single, empty leaf as the "tree" (root = this leaf).
    page_id_t root_id = dm.AllocatePage();
    {
        Page* page = bpm.FetchPage(root_id);
        BPlusTreeNode node(page);
        node.SetIsLeaf(true);
        node.SetNumKeys(0);
        node.SetNextLeafId(0);
        bpm.UnpinPage(root_id, true);
    }

    BPlusTree tree(&bpm, root_id);

    // Insert 3 keys in NON-sorted order — MAX_KEYS_PER_NODE is 3, so this
    // fills the leaf exactly to capacity without overflowing (Day 3 scope).
    tree.Insert(20, 200);
    tree.Insert(10, 100);
    tree.Insert(30, 300);
    printf("[PASS] Inserted 3 keys in random order without error\n");

    // --- Verify via Search ---
    value_t v;
    assert(tree.Search(10, &v) && v == 100);
    assert(tree.Search(20, &v) && v == 200);
    assert(tree.Search(30, &v) && v == 300);
    printf("[PASS] Search finds all 3 inserted keys with correct values\n");

    // --- Verify the leaf's internal array is actually sorted ---
    // (Directly inspect the node, not just via Search, to confirm Insert's
    // shifting logic placed keys in the right order internally.)
    Page* page = bpm.FetchPage(root_id);
    BPlusTreeNode node(page);
    assert(node.GetNumKeys() == 3);
    assert(node.GetKeyAt(0) == 10);
    assert(node.GetKeyAt(1) == 20);
    assert(node.GetKeyAt(2) == 30);
    printf("[PASS] Leaf's internal key array is correctly sorted: %d, %d, %d\n",
           node.GetKeyAt(0), node.GetKeyAt(1), node.GetKeyAt(2));
    bpm.UnpinPage(root_id, false);

    printf("\nAll Day 3 Insert tests passed!\n");
    return 0;
}
