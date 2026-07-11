#include "../include/buffer_pool.h"
#include <stdexcept>

BufferPool::BufferPool(size_t pool_size, DiskManager* disk_manager)
    : replacer_(pool_size), disk_manager_(disk_manager) {
    frames_.resize(pool_size);
    for (size_t i = 0; i < pool_size; i++) {
        free_list_.push_back(static_cast<frame_id_t>(i));
    }
}

Page* BufferPool::FetchPage(page_id_t page_id) {
    // Case 1: already cached.
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t frame_id = it->second;
        frames_[frame_id].pin_count++;
        replacer_.Pin(frame_id);  // in active use again, not evictable
        return &frames_[frame_id].page;
    }

    // Case 2: not cached. Need a frame — try free list first.
    frame_id_t frame_id;
    if (!free_list_.empty()) {
        frame_id = free_list_.front();
        free_list_.pop_front();
    } else {
        // Case 3: pool full. Ask the replacer for a victim.
        if (!replacer_.Victim(&frame_id)) {
            return nullptr;  // everything is pinned — nothing evictable
        }

        Frame& victim = frames_[frame_id];
        if (victim.is_dirty) {
            // Must flush modified data before this frame's memory is reused.
            disk_manager_->WritePage(victim.page_id, victim.page.GetData());
        }
        page_table_.erase(victim.page_id);
    }

    Frame& frame = frames_[frame_id];
    disk_manager_->ReadPage(page_id, frame.page.GetData());
    frame.page_id = page_id;
    frame.pin_count = 1;
    frame.is_dirty = false;

    page_table_[page_id] = frame_id;
    replacer_.Pin(frame_id);  // freshly loaded and pinned, not evictable
    return &frame.page;
}

void BufferPool::UnpinPage(page_id_t page_id, bool is_dirty) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return;  // not tracked — safe no-op
    }

    frame_id_t frame_id = it->second;
    Frame& frame = frames_[frame_id];

    if (frame.pin_count > 0) {
        frame.pin_count--;
    }
    frame.is_dirty = frame.is_dirty || is_dirty;

    if (frame.pin_count == 0) {
        replacer_.Unpin(frame_id);  // now a candidate for eviction
    }
}
