#pragma once

#include <cstdint>
#include <cstring>  // memcpy

// A page is always exactly this many bytes — matches the OS's typical
// disk block size, which is why 4096 (4KB) is the standard choice.
constexpr uint32_t PAGE_SIZE = 4096;

// What kind of data this page holds. INVALID is the default until
// something actually initializes the page.
enum class PageType : uint32_t {
    INVALID_PAGE = 0,
    DATA_PAGE = 1,
    INDEX_PAGE = 2
};

// Byte offsets of each header field within the page's raw data array.
// Header layout: [page_id (4 bytes)][page_type (4 bytes)][free_space_offset (4 bytes)]
constexpr uint32_t PAGE_ID_OFFSET = 0;
constexpr uint32_t PAGE_TYPE_OFFSET = 4;
constexpr uint32_t FREE_SPACE_OFFSET_OFFSET = 8;
constexpr uint32_t PAGE_HEADER_SIZE = 12;

class Page {
public:
    Page() {
        // Zero out the entire page on creation, so we start from a
        // clean, predictable state (no garbage memory).
        memset(data_, 0, PAGE_SIZE);
    }

    // --- page_id ---
    void SetPageId(uint32_t page_id) {
        memcpy(data_ + PAGE_ID_OFFSET, &page_id, sizeof(page_id));
    }
    uint32_t GetPageId() const {
        uint32_t page_id;
        memcpy(&page_id, data_ + PAGE_ID_OFFSET, sizeof(page_id));
        return page_id;
    }

    // --- page_type ---
    void SetPageType(PageType type) {
        memcpy(data_ + PAGE_TYPE_OFFSET, &type, sizeof(type));
    }
    PageType GetPageType() const {
        PageType type;
        memcpy(&type, data_ + PAGE_TYPE_OFFSET, sizeof(type));
        return type;
    }

    // --- free_space_offset ---
    void SetFreeSpaceOffset(uint32_t offset) {
        memcpy(data_ + FREE_SPACE_OFFSET_OFFSET, &offset, sizeof(offset));
    }
    uint32_t GetFreeSpaceOffset() const {
        uint32_t offset;
        memcpy(&offset, data_ + FREE_SPACE_OFFSET_OFFSET, sizeof(offset));
        return offset;
    }

    // Gives raw access to the full byte array — this is what disk_manager
    // will eventually write to / read from disk directly.
    char* GetData() { return data_; }

private:
    char data_[PAGE_SIZE];
};
