#include "../include/bplus_tree.h"

uint32_t BPlusTree::FindChildIndex(BPlusTreeNode& node, key_t key) {
    // Linear scan for now — correctness first, binary search is a trivial
    // optimization to swap in later since num_keys is small either way.
    uint32_t num_keys = node.GetNumKeys();
    for (uint32_t i = 0; i < num_keys; i++) {
        if (key < node.GetKeyAt(i)) {
            return i;  // descend into child[i]
        }
    }
    return num_keys;  // key >= all keys -> descend into the last child
}

bool BPlusTree::Search(key_t key, value_t* value) {
    page_id_t current_page_id = root_page_id_;

    while (true) {
        Page* page = bpm_->FetchPage(current_page_id);
        if (page == nullptr) {
            return false;  // shouldn't normally happen, but fail safely
        }
        BPlusTreeNode node(page);

        if (node.IsLeaf()) {
            // Linear scan the leaf for an exact match.
            uint32_t num_keys = node.GetNumKeys();
            for (uint32_t i = 0; i < num_keys; i++) {
                if (node.GetKeyAt(i) == key) {
                    *value = node.GetValueAt(i);
                    bpm_->UnpinPage(current_page_id, false);
                    return true;
                }
            }
            bpm_->UnpinPage(current_page_id, false);
            return false;  // reached a leaf, key not present
        }

        // Internal node: find which child to descend into, then move on.
        uint32_t child_index = FindChildIndex(node, key);
        page_id_t child_page_id = node.GetChildAt(child_index);

        // Unpin the parent BEFORE moving to the child — we're done with it.
        bpm_->UnpinPage(current_page_id, false);
        current_page_id = child_page_id;
    }
}

std::vector<std::pair<key_t, value_t>> BPlusTree::RangeScan(key_t start, key_t end) {
    std::vector<std::pair<key_t, value_t>> results;

    // Step 1: descend like Search(start) to find the correct starting leaf.
    page_id_t current_page_id = root_page_id_;
    while (true) {
        Page* page = bpm_->FetchPage(current_page_id);
        BPlusTreeNode node(page);

        if (node.IsLeaf()) {
            bpm_->UnpinPage(current_page_id, false);
            break;  // current_page_id now holds the starting leaf's id
        }

        uint32_t child_index = FindChildIndex(node, start);
        page_id_t child_page_id = node.GetChildAt(child_index);
        bpm_->UnpinPage(current_page_id, false);
        current_page_id = child_page_id;
    }

    // Step 2: walk forward via next_leaf_id, collecting keys in [start, end].
    while (current_page_id != 0 || results.empty()) {
        Page* page = bpm_->FetchPage(current_page_id);
        if (page == nullptr) break;
        BPlusTreeNode node(page);

        uint32_t num_keys = node.GetNumKeys();
        bool past_end = false;
        for (uint32_t i = 0; i < num_keys; i++) {
            key_t k = node.GetKeyAt(i);
            if (k >= start && k <= end) {
                results.push_back({k, node.GetValueAt(i)});
            }
            if (k > end) {
                past_end = true;
            }
        }

        uint32_t next_leaf_id = node.GetNextLeafId();
        bpm_->UnpinPage(current_page_id, false);

        if (past_end || next_leaf_id == 0) break;
        current_page_id = next_leaf_id;
    }

    return results;
}
