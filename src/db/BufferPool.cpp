#include <db/BufferPool.hpp>
#include <db/Database.hpp>
#include <numeric>

using namespace db;

BufferPool::BufferPool() : available(DEFAULT_NUM_PAGES) {
  std::iota(available.rbegin(), available.rend(), 0);
}


BufferPool::~BufferPool() {
    for (auto it = dirty.begin(); it != dirty.end(); ++it) {
        size_t pos = *it;
        const Page &page = pages[pos];
        const PageId &pid = posToPid[pos];
        getDatabase().get(pid.file).writePage(page, pid.page);
    }
}

Page &BufferPool::getPage(const PageId &pid) {
    // Check if the page is already in the buffer pool
    auto it = pidToPos.find(pid);
    if (it != pidToPos.end()) {
        size_t pos = it->second;
        // Move the page to the front of the LRU list (most recent)
        lruList.splice(lruList.begin(), lruList, posToLru[pos]);
        posToLru[pos] = lruList.begin();
        return pages[pos];
    }

    // Evict the least recently used page if no available pages
    if (available.empty()) {
        size_t pos = lruList.back();
        const PageId &old_pid = posToPid[pos];
        // If the page is dirty, flush it to disk
        if (isDirty(old_pid)) {
            flushPage(old_pid);
        }
        discardPage(old_pid);
    }

    // Read the requested page from disk and make it the most recent
    size_t pos = available.back();
    available.pop_back();

    Page &page = pages[pos];
    getDatabase().get(pid.file).readPage(page, pid.page);
    // Update mapping structures
    pidToPos[pid] = pos;
    posToPid[pos] = pid;
    // Mark this page as the most recent in the LRU list
    lruList.push_front(pos);
    posToLru[pos] = lruList.begin();

    return page;
}

void BufferPool::markDirty(const PageId &pid) {
    size_t pos = pidToPos.at(pid);
    dirty.insert(pos);
}

bool BufferPool::isDirty(const PageId &pid) const {
    size_t pos = pidToPos.at(pid);
    return dirty.contains(pos);
}

bool BufferPool::contains(const PageId &pid) const { return pidToPos.contains(pid); }


void BufferPool::discardPage(const PageId &pid) {
    auto it = pidToPos.find(pid);
    if (it == pidToPos.end()) {
        return; // Page not found, exit early
    }

    size_t pos = it->second;
    pidToPos.erase(it);  // Remove from pidToPos

    // Reset the array entry for posToPid
    posToPid[pos] = PageId();  // Assuming PageId has a default constructor

    // Safely remove from LRU structures
    if (posToLru.find(pos) != posToLru.end()) {
        lruList.erase(posToLru[pos]);
        posToLru.erase(pos);

    }

    // Remove from dirty set if present
    dirty.erase(pos);
    // Mark the page as available again
    available.push_back(pos);
}



void BufferPool::flushPage(const PageId &pid) {
    auto it = pidToPos.find(pid);
    if (it == pidToPos.end()) {
        return; // Page not found, exit early
    }

    size_t pos = it->second;

    if (dirty.find(pos) != dirty.end()) {
        dirty.erase(pos);  // Remove from dirty set
        const Page &page = pages[pos];
        getDatabase().get(pid.file).writePage(page, pid.page);
    }
}

void BufferPool::flushFile(const std::string &file) {
    std::vector<size_t> to_flush;
    for (auto it = dirty.begin(); it != dirty.end(); ++it) {
        size_t pos = *it;
        const PageId &pid = posToPid[pos];
        if (pid.file == file) {
            to_flush.emplace_back(pid.page);
        }
    }
    for (const auto &page : to_flush) {
        flushPage({file, page});
    }
}
