#include <db/Database.hpp>
#include <unordered_set>

using namespace db;

BufferPool &Database::getBufferPool() { return bufferPool; }

Database &db::getDatabase() {
    static Database instance;
    return instance;
}

void Database::add(std::unique_ptr<DbFile> file) {
    // Get the file name
    const std::string &name = file->getName();
    // Check if the file already exists in the database's collection
    if (files.contains(name)) {
        throw std::logic_error("File already existsin th database");
    }
    // Add the new file to the storage, using the file name as the key
    files[name] = std::move(file);
}

std::unique_ptr<DbFile> Database::remove(const std::string &name) {
    // Extract the file entry from the map using the provided file name
    auto fileEntry = files.extract(name);
    // If the file entry is empty, the file doesn't exist in the database
    if (fileEntry.empty()) {
        throw std::logic_error("File does not exist in the database");
    }
    // Flush the file from the buffer pool to ensure any changes are written
    Database::getBufferPool().flushFile(fileEntry.key());

    // Return the file by moving it from the extracted entry
    return std::move(fileEntry.mapped());
}

DbFile &Database::get(const std::string &name) const { return *files.at(name); }

