#include <db/DbFile.hpp>
#include <stdexcept>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace db;

const TupleDesc &DbFile::getTupleDesc() const { return td; }

DbFile::DbFile(const std::string &name, const TupleDesc &td) : name(name), td(td) {
    // Open or create the file with appropriate permissions
    fd = open(name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file: " + name);
    }
    // Retrieve file metadata
    struct stat fileStat{};
    if (fstat(fd, &fileStat) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get file status: " + name);
    }
    // Compute the number of pages
    numPages = std::max<size_t>(1, fileStat.st_size / DEFAULT_PAGE_SIZE);
}

DbFile::~DbFile() {
    close(fd);
}

const std::string &DbFile::getName() const { return name; }

void DbFile::readPage(Page &page, const size_t id) const {
    reads.push_back(id);
    std::fill(page.begin(), page.end(), 0);
    pread(fd, page.data(), DEFAULT_PAGE_SIZE, id * DEFAULT_PAGE_SIZE);
}

void DbFile::writePage(const Page &page, const size_t id) const {
    writes.push_back(id);
    pwrite(fd, page.data(), DEFAULT_PAGE_SIZE, id * DEFAULT_PAGE_SIZE);
}

const std::vector<size_t> &DbFile::getReads() const { return reads; }

const std::vector<size_t> &DbFile::getWrites() const { return writes; }

void DbFile::insertTuple([[maybe_unused]] const Tuple &t) { throw std::runtime_error("Not implemented"); }

void DbFile::deleteTuple([[maybe_unused]] const Iterator &it) { throw std::runtime_error("Not implemented"); }

Tuple DbFile::getTuple([[maybe_unused]] const Iterator &it) const { throw std::runtime_error("Not implemented"); }

void DbFile::next([[maybe_unused]] Iterator &it) const { throw std::runtime_error("Not implemented"); }

Iterator DbFile::begin() const { throw std::runtime_error("Not implemented"); }

Iterator DbFile::end() const { throw std::runtime_error("Not implemented"); }

size_t DbFile::getNumPages() const { return numPages; }
