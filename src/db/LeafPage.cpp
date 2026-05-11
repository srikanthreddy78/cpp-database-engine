#include <db/LeafPage.hpp>
#include <stdexcept>
#include <cstring>  // for memcpy
#include <cstdlib>  // for free

using namespace db;


// Constructor to set up a LeafPage
// Sets up the page header and reserves space to hold tuples
// Calculates the page's capacity based on the available space and the size of tuples
LeafPage::LeafPage([[maybe_unused]] Page &page, const TupleDesc &td, size_t key_index)
    : td(td), key_index(key_index), capacity(0), header(nullptr), data(nullptr) {

  // Initialize header
  header = new LeafPageHeader;

  // Calculate capacity based on available space
  size_t tuple_size = td.length();
  capacity = (DEFAULT_PAGE_SIZE - sizeof(LeafPageHeader)) / tuple_size;

  // Allocate memory for data
  data = static_cast<uint8_t *>(malloc(capacity * tuple_size));
}

LeafPage::~LeafPage() {
  delete header;
  free(data);
}

bool LeafPage::insertTuple(const Tuple &t) {
  int insertPos = 0;
  bool isLargest = true;
  bool isDuplicate = false;

  // Find the first INT field index
  int intFieldIndex = -1;
  for (size_t i = 0; i < td.size(); ++i) {
    if (t.field_type(i) == db::type_t::INT) {
      intFieldIndex = i;
      break;
    }
  }

  // Determine insertion position and check for duplicates
  for (int i = 0; i < header->size; ++i) {
    Tuple current = td.deserialize(data + i * td.length());
    auto currentVal = current.get_field(intFieldIndex);
    auto newVal = t.get_field(intFieldIndex);

    if (currentVal > newVal) {
      insertPos = i;
      isLargest = false;
      break;
    }

    if (currentVal == newVal) {
      insertPos = i;
      isDuplicate = true;
      break;
    }
  }

  // Shift tuples if not inserting at the end and not a duplicate
  if (!isLargest && !isDuplicate) {
    for (int i = header->size; i > insertPos; --i) {
      Tuple temp = td.deserialize(data + (i - 1) * td.length());
      td.serialize(data + i * td.length(), temp);
    }
  } else if (isLargest && !isDuplicate) {
    insertPos = header->size;
  }

  // Serialize the new tuple into the data section
  td.serialize(data + insertPos * td.length(), t);

  // Increment size if not a duplicate
  if (!isDuplicate) {
    header->size++;
  }

  // Return true if the page is full after insertion
  return header->size == capacity;
}
// Splits the current full leaf page into two pages.
// Transfers the second half of the tuples to the provided new page
// and updates the metadata accordingly.
// Returns the key of the first tuple in the new page for indexing purposes.
int LeafPage::split(LeafPage &new_page) {
  // Determine the number of tuples for each page after the split
  header->size = capacity / 2;
  new_page.header->size = capacity - header->size;
  new_page.header->next_leaf = header->next_leaf;

  // Copy the second half of tuples to the new page
  for (int i = 0; i < new_page.header->size; ++i) {
    Tuple t = td.deserialize(data + (header->size + i) * td.length());
    new_page.td.serialize(new_page.data + i * td.length(), t);
  }

  // Return the key of the first tuple in the new page
  return std::get<int>(td.deserialize(new_page.data).get_field(key_index));
}
// Retrieves the tuple stored at the given slot index in the page.
// Throws an exception if the slot is out of bounds.
Tuple LeafPage::getTuple(size_t slot) const {
  if (slot >= header->size) {
    throw std::runtime_error("Slot index out of range");
  }

  return td.deserialize(data + slot * td.length());
}
