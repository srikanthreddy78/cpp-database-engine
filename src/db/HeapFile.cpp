#include <db/Database.hpp>
#include <db/HeapFile.hpp>
#include <db/HeapPage.hpp>
#include <stdexcept>

using namespace db;

HeapFile::HeapFile(const std::string &name, const TupleDesc &td) : DbFile(name, td) {}

void HeapFile::insertTuple(const Tuple &t) {
    if (!td.compatible(t)) {
        throw std::runtime_error("Tuple not compatible with TupleDesc");
    }

    BufferPool &bufferPool = getDatabase().getBufferPool();
    PageId pid{name, numPages - 1}; // Start with the last page

    Page &p = bufferPool.getPage(pid);
    HeapPage hp(p, td);

    // Try inserting into the existing last page
    if (!hp.insertTuple(t)) {
        // If insertion fails, allocate a new page
        pid.page = numPages++; // Increment numPages and assign new page ID
        Page &np = bufferPool.getPage(pid);
        HeapPage nhp(np, td);

        if (!nhp.insertTuple(t)) {
            throw std::runtime_error("Failed to insert tuple into new page");
        }
    }

    bufferPool.markDirty(pid);
}

void HeapFile::deleteTuple(const Iterator &it) {
    BufferPool &bufferPool = getDatabase().getBufferPool();
    PageId pid{name, it.page};

    // Fetch the page from the buffer pool
    Page &page = bufferPool.getPage(pid);
    HeapPage heapPage(page, td);

    // Delete the tuple from the page
    heapPage.deleteTuple(it.slot);

    // Mark the page as dirty after modification
    bufferPool.markDirty(pid);
}

Tuple HeapFile::getTuple(const Iterator &it) const {
    BufferPool &bufferPool = getDatabase().getBufferPool();
    PageId pid{name, it.page};

    // Fetch the page from the buffer pool
    Page &page = bufferPool.getPage(pid);
    HeapPage heapPage(page, td);

    // Retrieve and return the requested tuple
    return heapPage.getTuple(it.slot);
}

void HeapFile::next(Iterator &it) const {
    if (numPages == 0) {
        // No pages exist, reset iterator
        it.page = 0;
        it.slot = 0;
        return;
    }

    BufferPool &bufferPool = getDatabase().getBufferPool();

    while (it.page < numPages) {
        PageId pid{name, it.page};
        Page &page = bufferPool.getPage(pid);
        const HeapPage heapPage(page, td);

        // If the iterator is at a valid slot, try moving to the next tuple
        if (it.slot != static_cast<size_t>(-1)) {
            heapPage.next(it.slot);
        } else {
            it.slot = heapPage.begin();
        }

        // If we find a valid slot, return immediately
        if (it.slot != heapPage.end()) {
            return;
        }

        // Move to the next page if the current page has no more tuples
        ++it.page;
        it.slot = static_cast<size_t>(-1); // Reset slot so that begin() is called on the next page
    }

    // If no more valid tuples are found, reset iterator
    it.page = numPages; // Move past the last page to indicate end
    it.slot = 0;
}


Iterator HeapFile::begin() const {
    if (numPages == 0) {
        // No pages exist, return an iterator pointing to the end
        return {*this, 0, 0};
    }

    BufferPool &bufferPool = getDatabase().getBufferPool();
    for (size_t page = 0; page < numPages; ++page) {
        PageId pid{name, page};
        Page &pageData = bufferPool.getPage(pid);
        const HeapPage heapPage(pageData, td);

        size_t slot = heapPage.begin();
        if (slot != heapPage.end()) {
            return {*this, page, slot};  // Found a valid tuple
        }
    }

    return {*this, numPages, 0};
}

Iterator HeapFile::end() const { return {*this, numPages, 0}; }

