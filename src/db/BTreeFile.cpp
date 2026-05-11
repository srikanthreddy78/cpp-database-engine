#include <cstring>
#include <db/BTreeFile.hpp>
#include <db/Database.hpp>
#include <db/IndexPage.hpp>
#include <db/LeafPage.hpp>
#include <stdexcept>

using namespace db;

// Custom constructor initializing BTreeFile with metadata
BTreeFile::BTreeFile(const std::string &name, const TupleDesc &td, size_t key_index)
    : DbFile(name, td), key_index(key_index) {}

// Insert a tuple into the tree, managing splits recursively if needed
void BTreeFile::insertTuple(const Tuple &tuple) {
  BufferPool &bp = getDatabase().getBufferPool();
  PageId currentId{name, root_id};
  std::vector<size_t> ancestors;

  Page &initial = bp.getPage(currentId);
  IndexPage root(initial);

  // Setup first leaf if root is unused
  if (root.header->size == 0 && root.children[0] != 1) {
    bp.markDirty(currentId);
    currentId.page = numPages++;
    root.children[0] = currentId.page;
  } else {
    // Navigate to leaf level by traversing the internal nodes
    while (true) {
      Page &intermediate = bp.getPage(currentId);
      IndexPage node(intermediate);

      int tupleKey = std::get<int>(tuple.get_field(key_index));
      size_t pos = std::distance(node.keys,
                    std::lower_bound(node.keys, node.keys + node.header->size, tupleKey));

      ancestors.push_back(currentId.page);
      currentId.page = node.children[pos];

      if (!node.header->index_children) break;
    }
  }

  // At leaf level, try inserting the tuple
  Page &leafPage = bp.getPage(currentId);
  bp.markDirty(currentId);
  LeafPage leaf(leafPage, td, key_index);

  if (!leaf.insertTuple(tuple)) return;  // Insert successful, no split required

  // Leaf full — perform split and propagate
  currentId.page = numPages++;
  Page &newLeafPage = bp.getPage(currentId);
  bp.markDirty(currentId);
  LeafPage newLeaf(newLeafPage, td, key_index);

  int promotedKey = leaf.split(newLeaf);
  leaf.header->next_leaf = currentId.page;
  size_t newChildId = currentId.page;

  // Bubble up split if necessary
  while (!ancestors.empty()) {
    size_t parentId = ancestors.back();
    ancestors.pop_back();

    currentId.page = parentId;
    Page &parentPage = bp.getPage(currentId);
    bp.markDirty(currentId);
    IndexPage parent(parentPage);

    if (!parent.insert(promotedKey, newChildId)) return;

    // If parent is full, continue splitting
    currentId.page = numPages++;
    Page &newParentPage = bp.getPage(currentId);
    bp.markDirty(currentId);
    IndexPage sibling(newParentPage);

    promotedKey = parent.split(sibling);
    newChildId = currentId.page;
  }

  // Original root split — form a new root
  bp.markDirty({name, root_id});
  if (!root.insert(promotedKey, newChildId)) return;

  // Construct two subtrees and reassign root contents
  PageId newId{name, numPages++};
  Page &firstChild = bp.getPage(newId);
  bp.markDirty(newId);
  firstChild = initial; // Copy old root
  size_t left = newId.page;
  IndexPage leftPage(firstChild);

  newId.page = numPages++;
  Page &secondChild = bp.getPage(newId);
  bp.markDirty(newId);
  size_t right = newId.page;
  IndexPage rightPage(secondChild);

  int newSplitKey = leftPage.split(rightPage);

  root.header->size = 1;
  root.header->index_children = true;
  root.keys[0] = newSplitKey;
  root.children[0] = left;
  root.children[1] = right;
}

// Stub function for tuple deletion (to be implemented)
void BTreeFile::deleteTuple([[maybe_unused]] const Iterator &it) {
  // Note: Deletion not required for this implementation scope.
  // Full deletion would require complex rebalancing, potential node merging, 
  // and borrowing from siblings to maintain the B+ Tree invariants.
}

// Fetch tuple pointed by the iterator
Tuple BTreeFile::getTuple(const Iterator &it) const {
  BufferPool &bp = getDatabase().getBufferPool();
  PageId pid{name, it.page};
  Page &pg = bp.getPage(pid);
  LeafPage leaf(pg, td, key_index);
  return leaf.getTuple(it.slot);
}

// Move iterator to the next tuple in the B+ tree
void BTreeFile::next(Iterator &it) const {
  BufferPool &bp = getDatabase().getBufferPool();
  PageId pid{name, it.page};
  Page &pg = bp.getPage(pid);
  LeafPage leaf(pg, td, key_index);

  if (it.slot + 1 < leaf.header->size) {
    ++it.slot;
  } else {
    it.page = leaf.header->next_leaf;
    it.slot = 0;
  }
}

// Return an iterator positioned at the smallest tuple in the tree
Iterator BTreeFile::begin() const {
  BufferPool &bp = getDatabase().getBufferPool();
  PageId pid{name, root_id};

  // Walk to leftmost leaf node
  while (true) {
    Page &pg = bp.getPage(pid);
    IndexPage node(pg);
    pid.page = node.children[0];
    if (!node.header->index_children) break;
  }

  return {*this, pid.page, 0};
}

// Returns an iterator that denotes the end of the sequence
Iterator BTreeFile::end() const {
  return {*this, 0, 0};  // Conventionally represents end
}