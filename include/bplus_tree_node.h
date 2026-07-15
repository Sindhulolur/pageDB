#pragma once

#include "page.h"
#include <cstdint>
#include <cstring>
#include <vector>

using key_t = int32_t;
using value_t = int32_t;  // for now, values are just ints; later this could be a record id

// Byte offsets within the page body (right after Page's own 12-byte header).
constexpr uint32_t NODE_IS_LEAF_OFFSET = PAGE_HEADER_SIZE;
constexpr uint32_t NODE_NUM_KEYS_OFFSET = PAGE_HEADER_SIZE + 1;
constexpr uint32_t NODE_PARENT_ID_OFFSET = PAGE_HEADER_SIZE + 5;
constexpr uint32_t NODE_DATA_OFFSET = PAGE_HEADER_SIZE + 9;  // keys/children/values start here

// A simple upper bound on keys per node, sized conservatively so
// key + child arrays comfortably fit within PAGE_SIZE. We'll refine
// this later if needed — today's goal is correctness, not maxing fanout.
constexpr uint32_t MAX_KEYS_PER_NODE = 3;

class BPlusTreeNode {
public:
    // Wraps an existing Page — does NOT own it, just provides a
    // structured view over its raw bytes (same pattern as Page itself
    // wrapping a char[4096]).
    explicit BPlusTreeNode(Page* page) : page_(page) {}

    // --- header fields ---
    void SetIsLeaf(bool is_leaf) {
        char val = is_leaf ? 1 : 0;
        memcpy(page_->GetData() + NODE_IS_LEAF_OFFSET, &val, sizeof(val));
    }
    bool IsLeaf() const {
        char val;
        memcpy(&val, page_->GetData() + NODE_IS_LEAF_OFFSET, sizeof(val));
        return val != 0;
    }

    void SetNumKeys(uint32_t n) {
        memcpy(page_->GetData() + NODE_NUM_KEYS_OFFSET, &n, sizeof(n));
    }
    uint32_t GetNumKeys() const {
        uint32_t n;
        memcpy(&n, page_->GetData() + NODE_NUM_KEYS_OFFSET, sizeof(n));
        return n;
    }

    void SetParentId(uint32_t parent_id) {
        memcpy(page_->GetData() + NODE_PARENT_ID_OFFSET, &parent_id, sizeof(parent_id));
    }
    uint32_t GetParentId() const {
        uint32_t parent_id;
        memcpy(&parent_id, page_->GetData() + NODE_PARENT_ID_OFFSET, sizeof(parent_id));
        return parent_id;
    }

    // --- key array (shared by leaf and internal nodes) ---
    void SetKeyAt(uint32_t index, key_t key) {
        uint32_t offset = NODE_DATA_OFFSET + index * sizeof(key_t);
        memcpy(page_->GetData() + offset, &key, sizeof(key));
    }
    key_t GetKeyAt(uint32_t index) const {
        key_t key;
        uint32_t offset = NODE_DATA_OFFSET + index * sizeof(key_t);
        memcpy(&key, page_->GetData() + offset, sizeof(key));
        return key;
    }

    // --- leaf-only: values array, positioned right after the key array ---
    uint32_t ValuesOffset() const {
        return NODE_DATA_OFFSET + MAX_KEYS_PER_NODE * sizeof(key_t);
    }
    void SetValueAt(uint32_t index, value_t value) {
        uint32_t offset = ValuesOffset() + index * sizeof(value_t);
        memcpy(page_->GetData() + offset, &value, sizeof(value));
    }
    value_t GetValueAt(uint32_t index) const {
        value_t value;
        uint32_t offset = ValuesOffset() + index * sizeof(value_t);
        memcpy(&value, page_->GetData() + offset, sizeof(value));
        return value;
    }

    // --- leaf-only: next_leaf_id, positioned right after the values array ---
    uint32_t NextLeafIdOffset() const {
        return ValuesOffset() + MAX_KEYS_PER_NODE * sizeof(value_t);
    }
    void SetNextLeafId(uint32_t next_leaf_id) {
        memcpy(page_->GetData() + NextLeafIdOffset(), &next_leaf_id, sizeof(next_leaf_id));
    }
    uint32_t GetNextLeafId() const {
        uint32_t next_leaf_id;
        memcpy(&next_leaf_id, page_->GetData() + NextLeafIdOffset(), sizeof(next_leaf_id));
        return next_leaf_id;
    }

    // --- internal-only: child id array, same position as values array
    //     would occupy (a node is only ever leaf OR internal, never both,
    //     so it's safe to reuse this region for a different purpose) ---
    uint32_t ChildrenOffset() const {
        return ValuesOffset();
    }
    void SetChildAt(uint32_t index, uint32_t child_page_id) {
        uint32_t offset = ChildrenOffset() + index * sizeof(uint32_t);
        memcpy(page_->GetData() + offset, &child_page_id, sizeof(child_page_id));
    }
    uint32_t GetChildAt(uint32_t index) const {
        uint32_t child_page_id;
        uint32_t offset = ChildrenOffset() + index * sizeof(uint32_t);
        memcpy(&child_page_id, page_->GetData() + offset, sizeof(child_page_id));
        return child_page_id;
    }

private:
    Page* page_;
};
