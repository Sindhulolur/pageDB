#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>

int main() {
    DiskManager dm("test_buffer_pool.db");

    // Allocate 5 pages up front on disk.
    page_id_t page_ids[5];
    for (int i = 0; i < 5; i++) {
        page_ids[i] = dm.AllocatePage();
    }

    // Pool bigger than the number of pages we're testing with today.
    BufferPool bp(10, &dm);

    // --- Fetch all 5 pages ---
    Page* pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = bp.FetchPage(page_ids[i]);
        assert(pages[i] != nullptr);
    }
    assert(dm.GetReadCount() == 5);
    printf("[PASS] Fetched 5 pages, disk read count = %d (expected 5)\n", dm.GetReadCount());

    // --- Fetch page 0 AGAIN without unpinning first ---
    // Should come from cache (page table hit), NOT trigger another disk read,
    // and pin count should become 2, not reset to 1.
    Page* page0_again = bp.FetchPage(page_ids[0]);
    assert(page0_again == pages[0]);  // same underlying Page pointer
    assert(dm.GetReadCount() == 5);   // still 5 — no new disk read happened
    printf("[PASS] Re-fetched page 0 from cache, read count still %d\n", dm.GetReadCount());

    // --- Unpin everything ---
    for (int i = 0; i < 5; i++) {
        bp.UnpinPage(page_ids[i], false);
    }
    // page 0 was pinned twice (fetched twice), so it needs a second unpin.
    bp.UnpinPage(page_ids[0], false);
    printf("[PASS] Unpinned all pages\n");

    // --- Fetch page 0 one more time — should STILL be cached (still in page table) ---
    Page* page0_third = bp.FetchPage(page_ids[0]);
    assert(page0_third == pages[0]);
    assert(dm.GetReadCount() == 5);  // still no new disk read
    printf("[PASS] Page 0 still cached after unpin, read count still %d\n", dm.GetReadCount());

    printf("\nAll BufferPool Day 4 tests passed!\n");
    return 0;
}
