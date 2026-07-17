#include "../include/bplus_tree.h"

uint32_t BPlusTree::FindChildIndex(BPlusTreeNode& node, key_t key) {
    uint32_t num_keys = node.GetNumKeys();
    for (uint32_t i = 0; i < num_keys; i++) {
        if (key < node.GetKeyAt(i)) {
            return i;
        }
    }
    return num_keys;
}

bool BPlusTree::Search(key_t key, value_t* value) {
    page_id_t current_page_id = root_page_id_;

    while (true) {
        Page* page = bpm_->FetchPage(current_page_id);
        if (page == nullptr) {
            return false;
        }
        BPlusTreeNode node(page);

        if (node.IsLeaf()) {
            uint32_t num_keys = node.GetNumKeys();
            for (uint32_t i = 0; i < num_keys; i++) {
                if (node.GetKeyAt(i) == key) {
                    *value = node.GetValueAt(i);
                    bpm_->UnpinPage(current_page_id, false);
                    return true;
                }
            }
            bpm_->UnpinPage(current_page_id, false);
            return false;
        }

        uint32_t child_index = FindChildIndex(node, key);
        page_id_t child_page_id = node.GetChildAt(child_index);
        bpm_->UnpinPage(current_page_id, false);
        current_page_id = child_page_id;
    }
}

std::vector<std::pair<key_t, value_t>> BPlusTree::RangeScan(key_t start, key_t end) {
    std::vector<std::pair<key_t, value_t>> results;

    page_id_t current_page_id = root_page_id_;
    while (true) {
        Page* page = bpm_->FetchPage(current_page_id);
        BPlusTreeNode node(page);

        if (node.IsLeaf()) {
            bpm_->UnpinPage(current_page_id, false);
            break;
        }

        uint32_t child_index = FindChildIndex(node, start);
        page_id_t child_page_id = node.GetChildAt(child_index);
        bpm_->UnpinPage(current_page_id, false);
        current_page_id = child_page_id;
    }

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

void BPlusTree::Insert(key_t key, value_t value) {
    // Step 1: descend to the correct leaf, exactly like Search does.
    page_id_t current_page_id = root_page_id_;

    while (true) {
        Page* page = bpm_->FetchPage(current_page_id);
        BPlusTreeNode node(page);

        if (node.IsLeaf()) {
            // Step 2: find the correct sorted insertion position.
            uint32_t num_keys = node.GetNumKeys();
            uint32_t insert_pos = 0;
            while (insert_pos < num_keys && node.GetKeyAt(insert_pos) < key) {
                insert_pos++;
            }

            // Step 3: shift everything from insert_pos onward one slot to
            // the right, to make room. Must shift from the END backward,
            // or we'd overwrite values before reading them.
            for (uint32_t i = num_keys; i > insert_pos; i--) {
                node.SetKeyAt(i, node.GetKeyAt(i - 1));
                node.SetValueAt(i, node.GetValueAt(i - 1));
            }

            // Step 4: place the new key/value and update the count.
            node.SetKeyAt(insert_pos, key);
            node.SetValueAt(insert_pos, value);
            node.SetNumKeys(num_keys + 1);

            bpm_->UnpinPage(current_page_id, true);  // dirty: we modified it
            return;
        }

        // Internal node: descend further, same as Search.
        uint32_t child_index = FindChildIndex(node, key);
        page_id_t child_page_id = node.GetChildAt(child_index);
        bpm_->UnpinPage(current_page_id, false);
        current_page_id = child_page_id;
    }
}
