#pragma once

#include <cstdint>
#include <string>

using page_id_t = uint32_t;

class DiskManager {
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();

    void WritePage(page_id_t page_id, const char* data);
    void ReadPage(page_id_t page_id, char* dest);
    page_id_t AllocatePage();

    // Testing helper: counts how many times ReadPage has actually issued
    // a disk read. Lets tests verify the buffer pool is serving cached
    // pages instead of re-reading from disk unnecessarily.
    int GetReadCount() const { return read_count_; }

private:
    int fd_;
    page_id_t next_page_id_;
    int read_count_ = 0;
};
