#pragma once

#include "page.h"
#include "disk_manager.h"
#include "lru_replacer.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <cstdint>

using frame_id_t = int32_t;

struct Frame {
    Page page;
    int pin_count = 0;
    bool is_dirty = false;
    page_id_t page_id = 0;
};

class BufferPool {
public:
    BufferPool(size_t pool_size, DiskManager* disk_manager);

    Page* FetchPage(page_id_t page_id);
    void UnpinPage(page_id_t page_id, bool is_dirty);

private:
    std::vector<Frame> frames_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::list<frame_id_t> free_list_;
    LRUReplacer replacer_;
    DiskManager* disk_manager_;
};
