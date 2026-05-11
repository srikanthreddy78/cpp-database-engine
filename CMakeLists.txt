cmake_minimum_required(VERSION 3.14)
project(cpp-database-engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup include directory
include_directories(include)

# Gather source files
file(GLOB DB_SOURCES "src/db/*.cpp")

# Create a static library for the database engine
add_library(db_engine STATIC ${DB_SOURCES})

# Enforce strict compiler warnings for production-quality code
target_compile_options(db_engine PRIVATE -Wall -Wextra -Wpedantic)

# --- Google Test Integration ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Add test executable
add_executable(db_tests tests/test_core.cpp)
target_link_libraries(db_tests PRIVATE db_engine gtest_main)

# Apply strict warnings to test executable as well
target_compile_options(db_tests PRIVATE -Wall -Wextra -Wpedantic)

# Discover tests automatically
include(GoogleTest)
gtest_discover_tests(db_tests)
