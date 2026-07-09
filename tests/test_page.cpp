#include "../include/page.h"
#include <cassert>
#include <cstdio>

int main() {
    Page page;

    // Test 1: page_id round-trips correctly
    page.SetPageId(5);
    assert(page.GetPageId() == 5);
    printf("[PASS] page_id set to 5, read back as %u\n", page.GetPageId());

    // Test 2: page_type round-trips correctly
    page.SetPageType(PageType::DATA_PAGE);
    assert(page.GetPageType() == PageType::DATA_PAGE);
    printf("[PASS] page_type set to DATA_PAGE, read back correctly\n");

    // Test 3: free_space_offset round-trips correctly
    page.SetFreeSpaceOffset(PAGE_HEADER_SIZE);
    assert(page.GetFreeSpaceOffset() == PAGE_HEADER_SIZE);
    printf("[PASS] free_space_offset set to %u, read back as %u\n",
           PAGE_HEADER_SIZE, page.GetFreeSpaceOffset());

    printf("\nAll Page tests passed!\n");
    return 0;
}
