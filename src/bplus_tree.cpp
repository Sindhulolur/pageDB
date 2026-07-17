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
    page_id_t current_page_id = *root_page_id_ptr_;

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

    page_id_t current_page_id = *root_page_id_ptr_;
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

// --- Day 4: leaf split ---
BPlusTree::InsertResult BPlusTree::SplitLeaf(page_id_t node_page_id, BPlusTreeNode& node,
                                              key_t new_key, value_t new_value) {
    uint32_t num_keys = node.GetNumKeys();

    uint32_t insert_pos = 0;
    while (insert_pos < num_keys && node.GetKeyAt(insert_pos) < new_key) {
        insert_pos++;
    }

    std::vector<key_t> keys;
    std::vector<value_t> values;
    keys.reserve(num_keys + 1);
    values.reserve(num_keys + 1);

    for (uint32_t i = 0; i < num_keys; i++) {
        if (i == insert_pos) {
            keys.push_back(new_key);
            values.push_back(new_value);
        }
        keys.push_back(node.GetKeyAt(i));
        values.push_back(node.GetValueAt(i));
    }
    if (insert_pos == num_keys) {
        keys.push_back(new_key);
        values.push_back(new_value);
    }

    uint32_t total = num_keys + 1;
    uint32_t left_count = total / 2;
    uint32_t right_count = total - left_count;

    page_id_t new_page_id = dm_->AllocatePage();
    Page* new_page = bpm_->FetchPage(new_page_id);
    BPlusTreeNode new_node(new_page);
    new_node.SetIsLeaf(true);
    new_node.SetParentId(node.GetParentId());

    for (uint32_t i = 0; i < left_count; i++) {
        node.SetKeyAt(i, keys[i]);
        node.SetValueAt(i, values[i]);
    }
    node.SetNumKeys(left_count);

    for (uint32_t i = 0; i < right_count; i++) {
        new_node.SetKeyAt(i, keys[left_count + i]);
        new_node.SetValueAt(i, values[left_count + i]);
    }
    new_node.SetNumKeys(right_count);

    new_node.SetNextLeafId(node.GetNextLeafId());
    node.SetNextLeafId(new_page_id);

    bpm_->UnpinPage(new_page_id, true);

    InsertResult result;
    result.split_occurred = true;
    result.promoted_key = keys[left_count];
    result.new_sibling_id = new_page_id;
    return result;
}

// --- Day 4 Step 3: internal-node split ---
BPlusTree::InsertResult BPlusTree::SplitInternal(page_id_t node_page_id, BPlusTreeNode& node,
                                                   key_t promoted_key, page_id_t new_child_id,
                                                   uint32_t insert_pos) {
    uint32_t num_keys = node.GetNumKeys();

    std::vector<key_t> keys;
    std::vector<page_id_t> children;
    keys.reserve(num_keys + 1);
    children.reserve(num_keys + 2);

    for (uint32_t i = 0; i < num_keys; i++) {
        keys.push_back(node.GetKeyAt(i));
    }
    keys.insert(keys.begin() + insert_pos, promoted_key);

    for (uint32_t i = 0; i <= num_keys; i++) {
        children.push_back(node.GetChildAt(i));
    }
    children.insert(children.begin() + insert_pos + 1, new_child_id);

    uint32_t total_keys = num_keys + 1;
    uint32_t mid = total_keys / 2;

    page_id_t new_page_id = dm_->AllocatePage();
    Page* new_page = bpm_->FetchPage(new_page_id);
    BPlusTreeNode new_node(new_page);
    new_node.SetIsLeaf(false);
    new_node.SetParentId(node.GetParentId());

    for (uint32_t i = 0; i < mid; i++) {
        node.SetKeyAt(i, keys[i]);
    }
    for (uint32_t i = 0; i <= mid; i++) {
        node.SetChildAt(i, children[i]);
    }
    node.SetNumKeys(mid);

    uint32_t right_num_keys = total_keys - mid - 1;
    for (uint32_t i = 0; i < right_num_keys; i++) {
        new_node.SetKeyAt(i, keys[mid + 1 + i]);
    }
    for (uint32_t i = 0; i <= right_num_keys; i++) {
        new_node.SetChildAt(i, children[mid + 1 + i]);
    }
    new_node.SetNumKeys(right_num_keys);

    for (uint32_t i = 0; i <= right_num_keys; i++) {
        Page* child_page = bpm_->FetchPage(new_node.GetChildAt(i));
        BPlusTreeNode child_node(child_page);
        child_node.SetParentId(new_page_id);
        bpm_->UnpinPage(new_node.GetChildAt(i), true);
    }

    bpm_->UnpinPage(new_page_id, true);

    InsertResult result;
    result.split_occurred = true;
    result.promoted_key = keys[mid];
    result.new_sibling_id = new_page_id;
    return result;
}

BPlusTree::InsertResult BPlusTree::InsertRecursive(page_id_t node_page_id, key_t key, value_t value) {
    Page* page = bpm_->FetchPage(node_page_id);
    BPlusTreeNode node(page);

    if (node.IsLeaf()) {
        uint32_t num_keys = node.GetNumKeys();

        if (num_keys < MAX_KEYS_PER_NODE) {
            uint32_t insert_pos = 0;
            while (insert_pos < num_keys && node.GetKeyAt(insert_pos) < key) {
                insert_pos++;
            }
            for (uint32_t i = num_keys; i > insert_pos; i--) {
                node.SetKeyAt(i, node.GetKeyAt(i - 1));
                node.SetValueAt(i, node.GetValueAt(i - 1));
            }
            node.SetKeyAt(insert_pos, key);
            node.SetValueAt(insert_pos, value);
            node.SetNumKeys(num_keys + 1);

            bpm_->UnpinPage(node_page_id, true);
            return InsertResult{};
        }

        InsertResult result = SplitLeaf(node_page_id, node, key, value);
        bpm_->UnpinPage(node_page_id, true);
        return result;
    }

    uint32_t child_index = FindChildIndex(node, key);
    page_id_t child_page_id = node.GetChildAt(child_index);
    InsertResult child_result = InsertRecursive(child_page_id, key, value);

    if (!child_result.split_occurred) {
        bpm_->UnpinPage(node_page_id, false);
        return InsertResult{};
    }

    uint32_t num_keys = node.GetNumKeys();

    if (num_keys < MAX_KEYS_PER_NODE) {
        for (uint32_t i = num_keys; i > child_index; i--) {
            node.SetKeyAt(i, node.GetKeyAt(i - 1));
        }
        for (uint32_t i = num_keys + 1; i > child_index + 1; i--) {
            node.SetChildAt(i, node.GetChildAt(i - 1));
        }
        node.SetKeyAt(child_index, child_result.promoted_key);
        node.SetChildAt(child_index + 1, child_result.new_sibling_id);
        node.SetNumKeys(num_keys + 1);

        bpm_->UnpinPage(node_page_id, true);
        return InsertResult{};
    }

    InsertResult result = SplitInternal(node_page_id, node, child_result.promoted_key,
                                         child_result.new_sibling_id, child_index);
    bpm_->UnpinPage(node_page_id, true);
    return result;
}

// --- Day 5: duplicate key policy = REJECT ---
// If the key already exists anywhere in the tree, Insert() is a silent
// no-op (the original value is preserved). This is checked with a plain
// Search() before touching the tree, so it costs one extra descent per
// insert but keeps the split logic itself completely untouched.
void BPlusTree::Insert(key_t key, value_t value) {
    value_t existing_value;
    if (Search(key, &existing_value)) {
        return;  // duplicate key: reject, keep original value
    }

    page_id_t old_root_id = *root_page_id_ptr_;
    InsertResult result = InsertRecursive(old_root_id, key, value);

    if (!result.split_occurred) {
        return;
    }

    page_id_t new_root_id = dm_->AllocatePage();
    Page* new_root_page = bpm_->FetchPage(new_root_id);
    BPlusTreeNode new_root(new_root_page);
    new_root.SetIsLeaf(false);
    new_root.SetParentId(0);
    new_root.SetNumKeys(1);
    new_root.SetKeyAt(0, result.promoted_key);
    new_root.SetChildAt(0, old_root_id);
    new_root.SetChildAt(1, result.new_sibling_id);
    bpm_->UnpinPage(new_root_id, true);

    Page* old_root_page = bpm_->FetchPage(old_root_id);
    BPlusTreeNode old_root(old_root_page);
    old_root.SetParentId(new_root_id);
    bpm_->UnpinPage(old_root_id, true);

    Page* sibling_page = bpm_->FetchPage(result.new_sibling_id);
    BPlusTreeNode sibling(sibling_page);
    sibling.SetParentId(new_root_id);
    bpm_->UnpinPage(result.new_sibling_id, true);

    *root_page_id_ptr_ = new_root_id;
}
