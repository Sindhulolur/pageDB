#include "../include/buffer_pool.h"
#include <stdexcept>

BufferPool::BufferPool(size_t pool_size, DiskManager* disk_manager)
    : disk_manager_(disk_manager) {
    frames_.resize(pool_size);

    // Every frame starts empty, so every frame_id starts in the free list.
    for (size_t i = 0; i < pool_size; i++) {
        free_list_.push_back(static_cast<frame_id_t>(i));
    }
}

Page* BufferPool::FetchPage(page_id_t page_id) {
    // Case 1: page is already cached in some frame.
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t frame_id = it->second;
        frames_[frame_id].pin_count++;
        return &frames_[frame_id].page;
    }

    // Case 2: page not cached. Need a free frame (no eviction yet today).
    if (free_list_.empty()) {
        // Pool is full and nothing free — Day 5's eviction logic will
        // handle this case. For now, fail explicitly rather than corrupt
        // state.
        return nullptr;
    }

    frame_id_t frame_id = free_list_.front();
    free_list_.pop_front();

    Frame& frame = frames_[frame_id];
    disk_manager_->ReadPage(page_id, frame.page.GetData());
    frame.page_id = page_id;
    frame.pin_count = 1;
    frame.is_dirty = false;

    page_table_[page_id] = frame_id;
    return &frame.page;
}

void BufferPool::UnpinPage(page_id_t page_id, bool is_dirty) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        // Unpinning a page that isn't even tracked — defensive no-op.
        // (Day 6 will test this exact edge case more rigorously.)
        return;
    }

    frame_id_t frame_id = it->second;
    Frame& frame = frames_[frame_id];

    if (frame.pin_count > 0) {
        frame.pin_count--;
    }
    // OR the dirty flag: once dirty, stays dirty until written back,
    // regardless of what later callers pass.
    frame.is_dirty = frame.is_dirty || is_dirty;
}
