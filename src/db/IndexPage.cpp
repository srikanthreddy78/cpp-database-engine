#include <db/IndexPage.hpp>
#include <stdexcept>
#include <algorithm>

using namespace db;

// Constructor that initializes the index page structure from a raw page
IndexPage::IndexPage(Page &page) {
    // Compute the maximum number of keys that can fit in a page
    int overhead = sizeof(IndexPageHeader) + sizeof(int);
    int entry_size = sizeof(int) + sizeof(size_t);
    capacity = (DEFAULT_PAGE_SIZE - overhead) / entry_size;

    // Interpret raw memory into structured parts: header, keys, and children pointers
    header = reinterpret_cast<IndexPageHeader *>(page.data());
    keys = reinterpret_cast<int *>(header + 1);
    children = reinterpret_cast<size_t *>(keys + capacity + 1); // +1 to support an extra child pointer
}

// Insert a key-child pair into the node in sorted order
bool IndexPage::insert(int new_key, size_t child_pointer) {
    // Determine the insertion position by binary search
    int *end = keys + header->size;
    int *pos_ptr = std::lower_bound(keys, end, new_key);
    size_t index = static_cast<size_t>(pos_ptr - keys);

    // Shift keys and children to the right to make space for new entry
    for (size_t i = header->size; i > index; --i) {
        keys[i] = keys[i - 1];
        children[i + 1] = children[i];
    }

    // Insert the new key and corresponding child
    keys[index] = new_key;
    children[index + 1] = child_pointer;
    header->size += 1;

    // Return whether the node has reached capacity after insertion
    return (header->size == capacity);
}

// Split the current node into two nodes
// Returns the middle key to be pushed up
int IndexPage::split(IndexPage &receiver) {
    // Find the midpoint of the node
    size_t mid = header->size / 2;

    // Setup the new sibling node
    receiver.header->index_children = header->index_children;
    receiver.header->size = header->size - mid - 1;

    // Transfer upper half of keys and children to the new node
    for (size_t i = 0; i < receiver.header->size; ++i) {
        receiver.keys[i] = keys[mid + 1 + i];
        receiver.children[i] = children[mid + 1 + i];
    }
    // Don't forget the final child pointer
    receiver.children[receiver.header->size] = children[header->size];

    // Shrink current node
    header->size = mid;

    // Return the key to be promoted
    return keys[mid];
}