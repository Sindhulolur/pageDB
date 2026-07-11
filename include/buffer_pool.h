#pragma once

#include "page.h"
#include "disk_manager.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <cstdint>

using frame_id_t = int32_t;

// A Frame is one fixed slot in the buffer pool. It holds exactly one Page's
// worth of data, plus bookkeeping about that page's current usage state.
struct Frame {
    Page page;
    int pin_count = 0;
    bool is_dirty = false;

    // Which page_id currently occupies this frame. Only meaningful if the
    // frame is actually in use (i.e. tracked in the page table).
    page_id_t page_id = 0;
};

class BufferPool {
public:
    BufferPool(size_t pool_size, DiskManager* disk_manager);

    // Returns a pointer to the requested page, loading it from disk if it's
    // not already cached. Increments pin count either way.
    // Returns nullptr if the page isn't cached AND no free frame is available
    // (this will change once eviction is added in Day 5).
    Page* FetchPage(page_id_t page_id);

    // Decrements pin count for the given page. If is_dirty is true, marks
    // the page dirty (a page can be marked dirty by one caller and stay
    // dirty even if a later caller passes is_dirty = false).
    void UnpinPage(page_id_t page_id, bool is_dirty);

private:
    std::vector<Frame> frames_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::list<frame_id_t> free_list_;  // frame IDs not currently holding a page
    DiskManager* disk_manager_;
};
