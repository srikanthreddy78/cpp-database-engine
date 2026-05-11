#include <gtest/gtest.h>
#include <vector>
#include <string>

// Include core database headers
#include "db/Tuple.hpp"
#include "db/BufferPool.hpp"

using namespace db;

class DatabaseEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed before each test
    }

    void TearDown() override {
        // Cleanup code if needed after each test
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

TEST_F(DatabaseEngineTest, TupleDescriptorInvalidInitialization) {
    std::vector<type_t> types = { type_t::INT, type_t::DOUBLE };
    std::vector<std::string> names = { "id", "value", "name" }; // Mismatched sizes
    
    EXPECT_THROW(TupleDesc(types, names), std::invalid_argument);
}

TEST_F(DatabaseEngineTest, BufferPoolInitialization) {
    // Basic test to ensure buffer pool can be instantiated
    BufferPool pool;
    // BufferPool doesn't have many public accessors for its state, but we ensure no crashes
    SUCCEED();
}
