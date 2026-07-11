#include "../include/disk_manager.h"
#include "../include/page.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>

DiskManager::DiskManager(const std::string& db_file) : next_page_id_(0) {
    fd_ = open(db_file.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd_ == -1) {
        perror("DiskManager: failed to open db file");
        throw std::runtime_error("Could not open database file");
    }
}

DiskManager::~DiskManager() {
    if (fd_ != -1) {
        close(fd_);
    }
}

void DiskManager::WritePage(page_id_t page_id, const char* data) {
    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    ssize_t bytes_written = pwrite(fd_, data, PAGE_SIZE, offset);
    if (bytes_written != PAGE_SIZE) {
        perror("DiskManager: pwrite failed or wrote fewer bytes than expected");
        throw std::runtime_error("WritePage failed");
    }
}

void DiskManager::ReadPage(page_id_t page_id, char* dest) {
    read_count_++;  // track that a real disk read was attempted

    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    ssize_t bytes_read = pread(fd_, dest, PAGE_SIZE, offset);

    if (bytes_read == 0) {
        memset(dest, 0, PAGE_SIZE);
        return;
    }
    if (bytes_read != PAGE_SIZE) {
        perror("DiskManager: pread failed or read fewer bytes than expected");
        throw std::runtime_error("ReadPage failed");
    }
}

page_id_t DiskManager::AllocatePage() {
    return next_page_id_++;
}
