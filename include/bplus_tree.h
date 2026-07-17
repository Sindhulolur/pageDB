#pragma once

#include "bplus_tree_node.h"
#include "buffer_pool.h"
#include "disk_manager.h"
#include <vector>
#include <cstdint>

class BPlusTree {
public:
    BPlusTree(BufferPool* bpm, DiskManager* dm, page_id_t* root_page_id_ptr)
        : bpm_(bpm), dm_(dm), root_page_id_ptr_(root_page_id_ptr) {}

    bool Search(key_t key, value_t* value);
    std::vector<std::pair<key_t, value_t>> RangeScan(key_t start, key_t end);
    void Insert(key_t key, value_t value);

private:
    uint32_t FindChildIndex(BPlusTreeNode& node, key_t key);

    struct InsertResult {
        bool split_occurred = false;
        key_t promoted_key = 0;
        page_id_t new_sibling_id = 0;
    };

    InsertResult InsertRecursive(page_id_t node_page_id, key_t key, value_t value);

    // Helpers that do the actual splitting work, used by InsertRecursive.
    InsertResult SplitLeaf(page_id_t node_page_id, BPlusTreeNode& node,
                            key_t new_key, value_t new_value);
    InsertResult SplitInternal(page_id_t node_page_id, BPlusTreeNode& node,
                                key_t promoted_key, page_id_t new_child_id,
                                uint32_t insert_pos);

    BufferPool* bpm_;
    DiskManager* dm_;
    page_id_t* root_page_id_ptr_;
};
