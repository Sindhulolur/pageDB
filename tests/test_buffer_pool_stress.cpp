#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include "../include/page.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

int main() {
    DiskManager dm("test_buffer_pool_stress.db");

    const int NUM_PAGES = 1000;
    const size_t POOL_SIZE = 50;

    std::vector<page_id_t> page_ids;
    for (int i = 0; i < NUM_PAGES; i++) {
        page_ids.push_back(dm.AllocatePage());
    }

    // --- Write a unique pattern into every page, forcing constant eviction ---
    {
        BufferPool bp(POOL_SIZE, &dm);

        for (int i = 0; i < NUM_PAGES; i++) {
            Page* page = bp.FetchPage(page_ids[i]);
            assert(page != nullptr);

            page_id_t pid = page_ids[i];
            memcpy(page->GetData() + PAGE_HEADER_SIZE, &pid, sizeof(pid));

            bp.UnpinPage(page_ids[i], true);  // mark dirty
        }
        printf("[PASS] Wrote unique pattern into all %d pages (pool size %zu)\n", NUM_PAGES, POOL_SIZE);

        // Re-access already-evicted pages, interleaved, to exercise eviction further.
        for (int i = 0; i < 100; i++) {
            Page* page = bp.FetchPage(page_ids[i]);
            assert(page != nullptr);
            bp.UnpinPage(page_ids[i], false);
        }
        printf("[PASS] Re-fetched first 100 pages again without error\n");
    }

    // --- Verify: every page's data survived, fresh BufferPool instance ---
    {
        BufferPool bp(POOL_SIZE, &dm);
        int mismatches = 0;

        for (int i = 0; i < NUM_PAGES; i++) {
            Page* page = bp.FetchPage(page_ids[i]);
            assert(page != nullptr);

            page_id_t stored;
            memcpy(&stored, page->GetData() + PAGE_HEADER_SIZE, sizeof(stored));
            if (stored != page_ids[i]) {
                mismatches++;
                printf("[FAIL] Page %u: expected %u, got %u\n", page_ids[i], page_ids[i], stored);
            }
            bp.UnpinPage(page_ids[i], false);
        }
        assert(mismatches == 0);
        printf("[PASS] All %d pages verified correct after eviction + reload\n", NUM_PAGES);
    }

    // --- Edge case 1: double fetch without unpin -> pin count 2, not reset to 1 ---
    {
        BufferPool bp(POOL_SIZE, &dm);
        Page* p1 = bp.FetchPage(page_ids[0]);
        Page* p2 = bp.FetchPage(page_ids[0]);
        assert(p1 == p2);
        bp.UnpinPage(page_ids[0], false);
        bp.UnpinPage(page_ids[0], false);
        printf("[PASS] Double-fetch + double-unpin handled correctly\n");
    }

    // --- Edge case 2: all frames pinned -> FetchPage fails gracefully ---
    {
        BufferPool bp(5, &dm);
        for (int i = 0; i < 5; i++) {
            assert(bp.FetchPage(page_ids[i]) != nullptr);  // fill pool, never unpin
        }
        Page* overflow = bp.FetchPage(page_ids[5]);
        assert(overflow == nullptr);
        printf("[PASS] FetchPage returns nullptr when pool full and all pinned\n");
    }

    // --- Edge case 3: over-unpinning doesn't crash or go negative ---
    {
        BufferPool bp(5, &dm);
        Page* p = bp.FetchPage(page_ids[0]);
        assert(p != nullptr);
        bp.UnpinPage(page_ids[0], false);
        bp.UnpinPage(page_ids[0], false);  // extra
        bp.UnpinPage(page_ids[0], false);  // extra
        printf("[PASS] Over-unpinning handled safely\n");
    }

    printf("\nAll Day 6 stress tests passed!\n");
    return 0;
}
