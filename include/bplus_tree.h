#pragma once

#include "bplus_tree_node.h"
#include "buffer_pool.h"
#include <vector>
#include <cstdint>

class BPlusTree {
public:
    BPlusTree(BufferPool* bpm, page_id_t root_page_id)
        : bpm_(bpm), root_page_id_(root_page_id) {}

    // Returns true and sets *value if key is found, false otherwise.
    bool Search(key_t key, value_t* value);

    // Collects all (key, value) pairs with start <= key <= end, in order.
    std::vector<std::pair<key_t, value_t>> RangeScan(key_t start, key_t end);

private:
    // Given an internal node and a key, returns which child index to descend into.
    uint32_t FindChildIndex(BPlusTreeNode& node, key_t key);

    BufferPool* bpm_;
    page_id_t root_page_id_;
};
