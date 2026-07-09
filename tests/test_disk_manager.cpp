#include "../include/disk_manager.h"
#include "../include/page.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    const std::string db_file = "test_disk_manager.db";

    // --- Phase 1: write 10 pages, each filled with a distinct byte pattern ---
    {
        DiskManager dm(db_file);

        for (page_id_t i = 0; i < 10; i++) {
            page_id_t page_id = dm.AllocatePage();
            char buffer[PAGE_SIZE];
            memset(buffer, static_cast<int>(page_id), PAGE_SIZE);  // fill page N with byte value N
            dm.WritePage(page_id, buffer);
        }
        printf("[PASS] Wrote 10 pages with distinct patterns\n");
        // DiskManager destructor runs here -> closes the file, simulating shutdown.
    }

    // --- Phase 2: reopen (simulate restart) and verify contents ---
    {
        DiskManager dm(db_file);  // fresh object, fresh file descriptor

        for (page_id_t i = 0; i < 10; i++) {
            char buffer[PAGE_SIZE];
            dm.ReadPage(i, buffer);

            // Every byte in this page should equal `i`, since that's what we wrote.
            bool matches = true;
            for (uint32_t b = 0; b < PAGE_SIZE; b++) {
                if (static_cast<unsigned char>(buffer[b]) != i) {
                    matches = false;
                    break;
                }
            }
            assert(matches);
            printf("[PASS] Page %u: contents match after reopen\n", i);
        }
    }

    printf("\nAll DiskManager persistence tests passed!\n");
    return 0;
}
