#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unistd.h>

// Include core database headers
#include "db/Tuple.hpp"
#include "db/BufferPool.hpp"
#include "db/Database.hpp"
#include "db/HeapFile.hpp"
#include "db/BTreeFile.hpp"
#include "db/Query.hpp"

using namespace db;

class DatabaseEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        unlink("test_heap.db");
        unlink("test_btree.db");
    }

    void TearDown() override {
        // Cleanup test files to maintain clean state
        try { getDatabase().remove("test_heap.db"); } catch(...) {}
        try { getDatabase().remove("test_btree.db"); } catch(...) {}
        unlink("test_heap.db");
        unlink("test_btree.db");
    }
};

TEST_F(DatabaseEngineTest, TupleCreationAndFieldAccess) {
    std::vector<field_t> fields = { 1, 2.5, std::string("test") };
    Tuple t(fields);
    
    EXPECT_EQ(t.size(), 3);
    EXPECT_EQ(t.field_type(0), type_t::INT);
    EXPECT_EQ(t.field_type(1), type_t::DOUBLE);
    EXPECT_EQ(t.field_type(2), type_t::CHAR);
    
    EXPECT_EQ(std::get<int>(t.get_field(0)), 1);
    EXPECT_DOUBLE_EQ(std::get<double>(t.get_field(1)), 2.5);
    EXPECT_EQ(std::get<std::string>(t.get_field(2)), "test");
}

TEST_F(DatabaseEngineTest, TupleDescriptorInitialization) {
    std::vector<type_t> types = { type_t::INT, type_t::DOUBLE, type_t::CHAR };
    std::vector<std::string> names = { "id", "value", "name" };
    
    TupleDesc td(types, names);
    
    EXPECT_EQ(td.size(), 3);
    EXPECT_EQ(td.index_of("id"), 0);
    EXPECT_EQ(td.index_of("value"), 1);
    EXPECT_EQ(td.index_of("name"), 2);
}

TEST_F(DatabaseEngineTest, BufferPoolLRUEviction) {
    auto& db = getDatabase();
    TupleDesc td({type_t::INT}, {"id"});
    db.add(std::make_unique<HeapFile>("test_heap.db", td));
    
    // Fill buffer pool to trigger LRU eviction
    for (size_t i = 0; i < DEFAULT_NUM_PAGES + 5; ++i) {
        PageId pid{"test_heap.db", i};
        db.getBufferPool().getPage(pid);
    }
    
    // The first few pages should be evicted
    EXPECT_FALSE(db.getBufferPool().contains({"test_heap.db", 0}));
    EXPECT_TRUE(db.getBufferPool().contains({"test_heap.db", DEFAULT_NUM_PAGES + 4}));
}

TEST_F(DatabaseEngineTest, HeapFileOperations) {
    auto& db = getDatabase();
    TupleDesc td({type_t::INT, type_t::DOUBLE}, {"id", "value"});
    db.add(std::make_unique<HeapFile>("test_heap.db", td));
    DbFile& file = db.get("test_heap.db");
    
    // Insert
    Tuple t1({10, 3.14});
    Tuple t2({20, 6.28});
    file.insertTuple(t1);
    file.insertTuple(t2);
    
    // Retrieve via Iterator
    auto it = file.begin();
    ASSERT_NE(it, file.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 10);
    
    ++it;
    ASSERT_NE(it, file.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 20);
    
    // Delete
    file.deleteTuple(it);
    
    // Verify deletion (iteration should now only hit t1)
    auto it2 = file.begin();
    ASSERT_NE(it2, file.end());
    ++it2;
    EXPECT_EQ(it2, file.end());
}

TEST_F(DatabaseEngineTest, BTreeInsertion) {
    auto& db = getDatabase();
    TupleDesc td({type_t::INT}, {"id"});
    // Key index is 0
    db.add(std::make_unique<BTreeFile>("test_btree.db", td, 0));
    DbFile& file = db.get("test_btree.db");
    
    file.insertTuple(Tuple({42}));
    file.insertTuple(Tuple({15}));
    file.insertTuple(Tuple({99}));
    
    // BTree should return items in sorted order
    auto it = file.begin();
    ASSERT_NE(it, file.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 15);
    
    ++it;
    ASSERT_NE(it, file.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 42);
    
    ++it;
    ASSERT_NE(it, file.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 99);
}

TEST_F(DatabaseEngineTest, QueryFilterOperator) {
    auto& db = getDatabase();
    TupleDesc td({type_t::INT}, {"value"});
    db.add(std::make_unique<HeapFile>("test_heap.db", td));
    db.add(std::make_unique<HeapFile>("test_btree.db", td)); // Using as output file
    
    DbFile& in = db.get("test_heap.db");
    DbFile& out = db.get("test_btree.db");
    
    in.insertTuple(Tuple({10}));
    in.insertTuple(Tuple({50}));
    in.insertTuple(Tuple({100}));
    
    std::vector<FilterPredicate> predicates = {
        {"value", PredicateOp::GT, 20}
    };
    
    db::filter(in, out, predicates);
    
    auto it = out.begin();
    ASSERT_NE(it, out.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 50);
    ++it;
    ASSERT_NE(it, out.end());
    EXPECT_EQ(std::get<int>((*it).get_field(0)), 100);
}
