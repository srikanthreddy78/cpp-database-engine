# Modern C++ Database Engine

A robust, high-performance database engine implemented in C++20. This engine was architected and developed as part of the rigorous **CS 660 Graduate Introduction to Database Systems** curriculum. It demonstrates a deep understanding of database internals, systems programming, and modern C++ best practices.

## 🚀 System Architecture & Features

This project implements standard enterprise database components from the ground up:

- **Buffer Pool Manager:** A latency-optimized caching layer implementing an LRU (Least Recently Used) policy. Features quick access/eviction mapping and precise dirty page tracking to minimize costly disk I/O.
- **Heap Storage Engine:** Core table storage featuring bitmap headers to efficiently manage dynamic tuple slots across database pages.
- **B+ Tree Indexing:** Highly scalable indexing structure supporting recursive node splits, internal node handling, and leaf page sorting for high-performance data retrieval.
- **Query Execution Engine:** Capable of executing complex relational queries including projection, filtering, grouping, aggregation, and joining across records. Includes a cardinality estimator based on histograms for query optimization.

## 🛠️ Code Quality & Engineering Standards

- **Modern C++20:** Leverages advanced C++ features (e.g., `std::holds_alternative`, `std::lower_bound`, defaulted comparison operators) for clean, safe, and performant code.
- **Low-Level Memory Management:** Leverages direct byte-level memory manipulation (e.g., bitwise operations for bitmap headers, pointer arithmetic for page layouts) to achieve highly efficient data serialization/deserialization, coupled with strict bounds checking to prevent buffer overflows.
- **Zero Warnings:** The codebase compiles cleanly under strict `-Wall -Wextra` enterprise compiler flags.
- **Unit Testing:** Integrated with **Google Test (GTest)** to ensure robust behavior and prevent regressions in core data structures.

## 🎓 Academic Performance
- **Course:** CS 660 Graduate Introduction to Database Systems
- **Grade Achieved:** 100.0 (Perfect Score via Autograder)
- **Developer:** Srikanth Reddy Gondireddy.

## ⚙️ Building the Engine

The project uses CMake and `FetchContent` to automatically handle dependencies (like Google Test). A C++20 compatible compiler is required.

```sh
mkdir build
cd build
cmake ..
make
```

## 🧪 Running Tests

The test suite is built on Google Test to verify core database constructs. Run them using the generated executable:

```sh
./build/db_tests
```
