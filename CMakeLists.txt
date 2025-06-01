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

# Code Coverage Support
option(CODE_COVERAGE "Enable coverage reporting" ON)

if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Enable code coverage flags")
    target_compile_options(scanner_test PRIVATE --coverage -O0 -g)
    target_link_libraries(scanner_test PRIVATE --coverage)

    # Coverage target
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info --ignore-errors inconsistent,unsupported,format
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/_deps/*' --output-file coverage.info.cleaned --ignore-errors inconsistent,unsupported,format
            COMMAND ${GENHTML_PATH} -o coverage_report coverage.info.cleaned
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report in coverage_report/index.html"
            DEPENDS scanner_test
        )
    endif()
endif()