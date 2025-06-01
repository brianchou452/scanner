cmake_minimum_required(VERSION 3.31)
project(scanner C CXX)
include(FetchContent)

set(CMAKE_C_STANDARD 17)

# SLRE - Super Light Regular Expression library
FetchContent_Declare(
  slre
  GIT_REPOSITORY https://github.com/aquefir/slre.git
  GIT_TAG a2c3d3c6416a58521ef3a884d9cb7d305c491d32
)

FetchContent_MakeAvailable(slre)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(scanner scanner.c
    ${slre_SOURCE_DIR}/slre.c
)

target_include_directories(scanner PRIVATE
    ${slre_SOURCE_DIR}
)

# GoogleTest unit test
enable_testing()
add_executable(scanner_test test/scanner_test.cpp 
    ${slre_SOURCE_DIR}/slre.c
)
set_target_properties(scanner_test PROPERTIES CXX_STANDARD 23)
target_include_directories(scanner_test PRIVATE ${slre_SOURCE_DIR})
target_compile_definitions(scanner_test PRIVATE UNIT_TEST)
target_link_libraries(scanner_test PRIVATE gtest_main)
add_test(NAME ScannerUnitTest COMMAND scanner_test)