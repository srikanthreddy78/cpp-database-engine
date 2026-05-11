# Modern C++ Database Engine

A robust, high-performance relational database storage engine implemented from scratch in **C++20**, developed as part of the rigorous **CS 660 Graduate Introduction to Database Systems** curriculum at **Boston University**.

---

## 🏆 Academic Performance
- **Course:** CS 660 — Graduate Introduction to Database Systems, Boston University
- **Grade Achieved:** 100.0 (Perfect Score via Autograder)
- **Developer:** Srikanth Reddy Gondireddy

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        QUERY LAYER                          │
│   filter()   projection()   join()   aggregate()            │
│              ColumnStats (Cardinality Estimator)            │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                     DATABASE CATALOG                        │
│         Database (Singleton) — manages all DbFiles          │
└──────────┬──────────────────────────────┬───────────────────┘
           │                              │
┌──────────▼──────────┐     ┌─────────────▼─────────────────┐
│      HeapFile       │     │          BTreeFile             │
│  (Unordered CRUD)   │     │    (Sorted Index + Search)     │
│  ┌───────────────┐  │     │  ┌──────────────────────────┐  │
│  │   HeapPage    │  │     │  │  IndexPage (Internal)    │  │
│  │ Bitmap Header │  │     │  │  Keys + Children[]       │  │
│  │ Tuple Slots   │  │     │  ├──────────────────────────┤  │
│  └───────────────┘  │     │  │  LeafPage (Leaf)         │  │
└──────────┬──────────┘     │  │  Sorted Tuples + next_   │  │
           │                │  │  leaf pointer            │  │
           │                │  └──────────────────────────┘  │
           │                └─────────────┬──────────────────┘
           │                              │
┌──────────▼──────────────────────────────▼──────────────────┐
│                      BUFFER POOL                           │
│         LRU Eviction Policy — 50 Pages in Memory           │
│  ┌──────────────┐  ┌─────────────┐  ┌──────────────────┐  │
│  │  Page Cache  │  │  LRU List   │  │   Dirty Set      │  │
│  │  (50 x 4KB)  │  │ (linked)    │  │ (flush on evict) │  │
│  └──────────────┘  └─────────────┘  └──────────────────┘  │
└────────────────────────────┬───────────────────────────────┘
                             │
┌────────────────────────────▼───────────────────────────────┐
│                        DISK (DbFile)                       │
│         pread() / pwrite() — Page-level I/O (4KB pages)    │
└────────────────────────────────────────────────────────────┘
```

---

## 🚀 Core Components

### 1. Buffer Pool Manager
- **LRU eviction** using `std::list` + `std::unordered_map` for O(1) access and eviction
- Manages **50 pages × 4KB = 200KB** in-memory cache
- **Dirty page tracking** — only modified pages are flushed to disk on eviction
- Lazy flush on `~BufferPool()` destructor ensures no data loss
- Prevents redundant disk I/O through `contains()` check before every page read

### 2. Heap Storage Engine
- Stores records in **unordered pages** with **bitmap-based slot management**
- Bitmap header tracks occupied/free slots using bitwise operations (`slot / 8`, `1 << (7 - slot % 8)`)
- Supports full **CRUD** — insert into first available slot, delete by clearing bitmap bit
- Iterator pattern (`begin()`, `end()`, `next()`) for sequential tuple traversal
- Handles multi-page scans transparently

### 3. B+ Tree Index
- **Sorted index structure** for high-performance key-based lookups
- Leaf pages store sorted tuples with a `next_leaf` pointer enabling range scans
- Internal (index) pages store separator keys + child page pointers
- **Recursive split propagation** — leaf splits bubble up through internal nodes
- Root split handled by creating two child subtrees and updating root in place
- `begin()` traverses to the leftmost leaf for full ordered iteration

### 4. Query Execution Engine
Implements a full relational algebra operator set:

| Operator | Description |
|---|---|
| `filter()` | Applies `FilterPredicate` conditions (EQ, NE, LT, LE, GT, GE) |
| `projection()` | Selects a subset of fields by name |
| `join()` | Nested loop join across two DbFiles with predicate support |
| `aggregate()` | SUM, AVG, MIN, MAX, COUNT with optional GROUP BY |

### 5. Cardinality Estimator (ColumnStats)
- **Histogram-based** cardinality estimation for query optimization
- Fixed-width buckets over `[min, max]` range with uniform distribution assumption
- Supports all predicate types — estimates fraction of tuples satisfying each condition
- Powers cost-based query planning decisions

---

## ⚙️ Data Types Supported

| Type | Size | C++ Representation |
|---|---|---|
| `INT` | 4 bytes | `int` |
| `DOUBLE` | 8 bytes | `double` |
| `CHAR` | 64 bytes | `std::string` |

---

## 🔧 Code Quality & Engineering Standards

- **C++20** — `std::holds_alternative`, `std::lower_bound`, defaulted comparison operators, `[[maybe_unused]]`
- **Zero Warnings** — compiles cleanly under strict `-Wall -Wextra` enterprise compiler flags
- **Low-level memory management** — bitwise bitmap operations, pointer arithmetic for page layouts, direct `pread`/`pwrite` syscalls
- **RAII design** — BufferPool destructor flushes all dirty pages, DbFile destructor closes file descriptor
- **Singleton Database** — thread-safe global catalog via `getDatabase()`
- **Google Test (GTest)** — unit tests for core data structures and buffer pool behavior

---

## 📊 Performance Characteristics

| Operation | Complexity | Notes |
|---|---|---|
| Buffer pool lookup | O(1) | unordered_map hash lookup |
| LRU eviction | O(1) | Linked list splice |
| B+ tree insert | O(log n) | Tree height traversal |
| B+ tree range scan | O(k) | Leaf linked list traversal |
| Heap insert | O(1) amortized | Last page slot scan |
| Nested loop join | O(n²) | Hash join would improve to O(n) avg |
| Cardinality estimate | O(1) | Histogram bucket lookup |

---

## ⚙️ Building the Engine

Requires a C++20 compatible compiler and CMake 3.14+.

```sh
mkdir build
cd build
cmake ..
make
```

---

## 🧪 Running Tests

```sh
./build/db_tests
```

To run with verbose output:

```sh
./build/db_tests --gtest_verbose
```

---

## 🗂️ Project Structure

```
database-storage-engine/
├── include/db/
│   ├── BufferPool.hpp      # LRU buffer pool manager
│   ├── Database.hpp        # Singleton database catalog
│   ├── DbFile.hpp          # Abstract base file class
│   ├── HeapFile.hpp        # Unordered heap storage
│   ├── HeapPage.hpp        # Bitmap-based page layout
│   ├── BTreeFile.hpp       # B+ tree index file
│   ├── IndexPage.hpp       # Internal B+ tree node
│   ├── LeafPage.hpp        # Leaf B+ tree node
│   ├── Query.hpp           # Relational operators
│   ├── ColumnStats.hpp     # Cardinality estimator
│   ├── Tuple.hpp           # Tuple and TupleDesc
│   ├── Iterator.hpp        # Tuple iterator
│   └── types.hpp           # Core type definitions
├── src/db/
│   ├── BufferPool.cpp
│   ├── Database.cpp
│   ├── DbFile.cpp
│   ├── HeapFile.cpp
│   ├── HeapPage.cpp
│   ├── BTreeFile.cpp
│   ├── IndexPage.cpp
│   ├── LeafPage.cpp
│   ├── Query.cpp
│   ├── ColumnStats.cpp
│   ├── Tuple.cpp
│   └── Iterator.cpp
├── tests/
│   └── test_core.cpp       # Google Test suite
├── CMakeLists.txt
└── README.md
```

---

## 💡 Key Engineering Decisions

**Why LRU for buffer eviction?**
LRU approximates temporal locality well for database workloads — recently accessed pages are likely to be accessed again soon (e.g., B+ tree root, hot index pages).

**Why bitmap headers for HeapPage?**
Bitmap headers use minimal space (1 bit per slot) and enable O(1) slot status checks via bitwise operations, avoiding per-slot metadata overhead.

**Why B+ tree over B tree?**
All data lives in leaf nodes, enabling efficient range scans via the `next_leaf` linked list without traversing internal nodes.

**Why histogram-based cardinality estimation?**
Histograms provide a space-efficient approximation of value distributions, enabling the query optimizer to estimate result sizes for different predicates without storing raw data.
