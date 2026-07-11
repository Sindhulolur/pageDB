#pragma once

#include <list>
#include <unordered_map>
#include <cstdint>

using frame_id_t = int32_t;

// Tracks which frames are currently "evictable" (unpinned) and picks the
// least-recently-used one when asked for a victim. Kept separate from
// BufferPool on purpose — it knows nothing about pages or disk, only
// frame_ids and recency order.
class LRUReplacer {
public:
    explicit LRUReplacer(size_t num_frames);

    // Removes a frame from eviction candidacy (it's now in active use).
    void Pin(frame_id_t frame_id);

    // Adds a frame back into eviction candidacy, marked most-recently-used.
    void Unpin(frame_id_t frame_id);

    // Picks the least-recently-used evictable frame, removes it from
    // tracking, and writes its id into *frame_id. Returns false if nothing
    // is evictable.
    bool Victim(frame_id_t* frame_id);

private:
    std::list<frame_id_t> lru_list_;  // front = most recently used, back = least
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> position_;
};
