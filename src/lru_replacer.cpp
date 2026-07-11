#include "../include/lru_replacer.h"

LRUReplacer::LRUReplacer(size_t num_frames) {
    (void)num_frames;  // not needed with a list-based approach, kept for API clarity
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    auto it = position_.find(frame_id);
    if (it != position_.end()) {
        lru_list_.erase(it->second);
        position_.erase(it);
    }
    // If it wasn't tracked at all, this is a safe no-op.
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    if (position_.find(frame_id) != position_.end()) {
        return;  // already tracked as evictable — don't duplicate
    }
    lru_list_.push_front(frame_id);
    position_[frame_id] = lru_list_.begin();
}

bool LRUReplacer::Victim(frame_id_t* frame_id) {
    if (lru_list_.empty()) {
        return false;
    }
    *frame_id = lru_list_.back();  // least-recently-used
    lru_list_.pop_back();
    position_.erase(*frame_id);
    return true;
}
