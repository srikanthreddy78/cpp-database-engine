#include <db/Database.hpp>
#include <db/HeapPage.hpp>
#include <stdexcept>

using namespace db;

HeapPage::HeapPage(Page &page, const TupleDesc &td) : td(td) {
    if (td.length() <= 0) {
        throw std::invalid_argument("TupleDesc length must be greater than zero");
    }
    size_t tupleSize = td.length();
    if (tupleSize > DEFAULT_PAGE_SIZE) {
        throw std::runtime_error("Tuple size exceeds page size");
    }

    capacity = std::max<size_t>(1, (DEFAULT_PAGE_SIZE * 8) / (tupleSize * 8 + 1));
    header = page.data();
    data = header + DEFAULT_PAGE_SIZE - (tupleSize * capacity);

    if (data < header) {
        throw std::runtime_error("Invalid page layout: data pointer is out of bounds");
    }
}

size_t HeapPage::begin() const {
    for (size_t i = 0; i < capacity; i++) {
        if (!empty(i)) {
            return i;
        }
    }
    return capacity;
}

size_t HeapPage::end() const { return capacity; }

bool HeapPage::insertTuple(const Tuple &t) {
    if (!td.compatible(t)) {
        throw std::invalid_argument("Tuple is not compatible with TupleDesc");
    }

    // Ensure the page has a valid capacity
    if (capacity == 0 || data == nullptr || header == nullptr) {
        throw std::runtime_error("HeapPage is uninitialized or corrupted");
    }

    // Find an empty slot using the header bitmap
    size_t slot = 0;
    while (slot < capacity) {
        if (!(header[slot / 8] & (1 << (7 - slot % 8)))) {
            break;  // Found an empty slot
        }
        slot++;
    }

    // If no empty slot is available, return false
    if (slot == capacity) {
        return false;
    }

    // Mark the slot as occupied in the header bitmap
    header[slot / 8] |= (1 << (7 - slot % 8));

    // Compute the slot's data location and ensure it's valid
    uint8_t *slotData = data + (slot * td.length());
    if (slotData < data || slotData + td.length() > header + DEFAULT_PAGE_SIZE) {
        throw std::runtime_error("Tuple insertion caused memory overflow");
    }

    // Serialize the tuple into the allocated space
    td.serialize(slotData, t);
    return true;
}

void HeapPage::deleteTuple(size_t slot) {
    // Ensure the page is properly initialized
    if (capacity == 0 || header == nullptr || data == nullptr) {
        throw std::runtime_error("HeapPage is uninitialized or corrupted");
    }

    // Validate the slot index
    if (slot >= capacity) {
        throw std::out_of_range("Slot index is out of bounds");
    }

    // Check if the slot is occupied
    if (!(header[slot / 8] & (1 << (7 - slot % 8)))) {
        throw std::runtime_error("Cannot delete: Slot is already empty");
    }

    // Mark the slot as empty
    header[slot / 8] &= ~(1 << (7 - slot % 8));

    // Get pointer to the slot data
    uint8_t *slotData = data + (slot * td.length());

    // Validate memory integrity before deletion
    if (slotData < data || slotData + td.length() > header + DEFAULT_PAGE_SIZE) {
        throw std::runtime_error("Memory corruption detected in slot deletion");
    }

    // Manually reset slot data by iterating
    for (size_t i = 0; i < td.length(); ++i) {
        slotData[i] = 0;
    }
}

Tuple HeapPage::getTuple(size_t slot) const {
    if (capacity == 0 || header == nullptr || data == nullptr) {
        throw std::runtime_error("HeapPage is uninitialized or corrupted");
    }

    if (slot >= capacity) {
        throw std::out_of_range("Slot index is out of bounds");
    }

    if (!(header[slot / 8] & (1 << (7 - slot % 8)))) {
        throw std::runtime_error("Slot is not occupied");
    }

    uint8_t *slotData = data + (slot * td.length());
    if (slotData < data || slotData + td.length() > header + DEFAULT_PAGE_SIZE) {
        throw std::runtime_error("Memory corruption detected while retrieving tuple");
    }

    return td.deserialize(slotData);
}

void HeapPage::next(size_t &slot) const {
    if (capacity == 0 || header == nullptr) {
        throw std::runtime_error("HeapPage is uninitialized or corrupted");
    }

    while (++slot < capacity) {
        if (header[slot / 8] & (1 << (7 - slot % 8))) {
            return;  // Found the next occupied slot
        }
    }

    slot = capacity;
}

bool HeapPage::empty(size_t slot) const { return !(header[slot / 8] & (1 << (7 - slot % 8))); }
