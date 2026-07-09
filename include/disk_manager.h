#pragma once

#include <cstdint>
#include <string>

using page_id_t = uint32_t;

class DiskManager {
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();

    // Writes PAGE_SIZE bytes from `data` to the given page's location on disk.
    void WritePage(page_id_t page_id, const char* data);

    // Reads PAGE_SIZE bytes from the given page's location on disk into `dest`.
    // If the page has never been written (beyond current file size), fills
    // `dest` with zeros instead of reading garbage/failing.
    void ReadPage(page_id_t page_id, char* dest);

    // Hands out the next unused page_id. Simple incrementing counter.
    page_id_t AllocatePage();

private:
    int fd_;                  // file descriptor for the backing .db file
    page_id_t next_page_id_;  // counter for AllocatePage()
};
