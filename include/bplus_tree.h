#pragma once

#include "bplus_tree_node.h"
#include "buffer_pool.h"
#include <vector>
#include <cstdint>

class BPlusTree {
public:
    BPlusTree(BufferPool* bpm, page_id_t root_page_id)
        : bpm_(bpm), root_page_id_(root_page_id) {}

    bool Search(key_t key, value_t* value);
    std::vector<std::pair<key_t, value_t>> RangeScan(key_t start, key_t end);

    // Inserts key/value into the correct leaf. Does NOT handle overflow yet
    // (that's Day 4) — caller must ensure the target leaf has room.
    void Insert(key_t key, value_t value);

private:
    uint32_t FindChildIndex(BPlusTreeNode& node, key_t key);

    BufferPool* bpm_;
    page_id_t root_page_id_;
};
