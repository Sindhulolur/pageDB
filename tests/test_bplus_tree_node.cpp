#include "../include/bplus_tree_node.h"
#include "../include/page.h"
#include <cassert>
#include <cstdio>

int main() {
    Page page;
    BPlusTreeNode node(&page);

    // Build a leaf node with 3 keys in memory.
    node.SetIsLeaf(true);
    node.SetNumKeys(3);
    node.SetParentId(0);  // 0 = no parent (root), fine for this isolated test

    node.SetKeyAt(0, 10);
    node.SetKeyAt(1, 20);
    node.SetKeyAt(2, 30);

    node.SetValueAt(0, 100);
    node.SetValueAt(1, 200);
    node.SetValueAt(2, 300);

    node.SetNextLeafId(99);

    // "Deserialize" by wrapping a fresh BPlusTreeNode around the SAME
    // underlying page bytes — proves the data really lives in the byte
    // array, not just in some in-memory object state.
    BPlusTreeNode reloaded(&page);

    assert(reloaded.IsLeaf() == true);
    printf("[PASS] is_leaf round-trips correctly\n");

    assert(reloaded.GetNumKeys() == 3);
    printf("[PASS] num_keys round-trips as %u\n", reloaded.GetNumKeys());

    assert(reloaded.GetKeyAt(0) == 10);
    assert(reloaded.GetKeyAt(1) == 20);
    assert(reloaded.GetKeyAt(2) == 30);
    printf("[PASS] All 3 keys round-trip correctly: %d, %d, %d\n",
           reloaded.GetKeyAt(0), reloaded.GetKeyAt(1), reloaded.GetKeyAt(2));

    assert(reloaded.GetValueAt(0) == 100);
    assert(reloaded.GetValueAt(1) == 200);
    assert(reloaded.GetValueAt(2) == 300);
    printf("[PASS] All 3 values round-trip correctly: %d, %d, %d\n",
           reloaded.GetValueAt(0), reloaded.GetValueAt(1), reloaded.GetValueAt(2));

    assert(reloaded.GetNextLeafId() == 99);
    printf("[PASS] next_leaf_id round-trips as %u\n", reloaded.GetNextLeafId());

    printf("\nAll BPlusTreeNode serialization tests passed!\n");
    return 0;
}
