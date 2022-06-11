cmake_minimum_required(VERSION 3.12)
project(clsa)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti)
SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

find_package(Clang REQUIRED CONFIG)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")

include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})
set(CLANG_LIBRARIES clangFrontend clangTooling clang)

find_package(Z3 REQUIRED)
find_path(Z3_INCLUDE_DIRS
        NAMES z3.h
        PATH_SUFFIXES z3
        DOC "Z3 C header")
if (Z3_INCLUDE_DIRS)
    message(STATUS "Found Z3 include path: \"${Z3_INCLUDE_DIRS}\"")
else()
    message(STATUS "Could not find Z3 include path")
endif()

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

set(LIB_SRCS lib/analyzer/core/analyzer_parameters.h lib/analyzer/frontend/analyzer.cpp lib/analyzer/frontend/analyzer.h lib/analyzer/core/ast_consumer.cpp lib/analyzer/core/ast_consumer.h lib/analyzer/core/ast_visitor.cpp lib/analyzer/core/ast_visitor.h lib/analyzer/core/block.cpp lib/analyzer/core/block.h lib/analyzer/core/analyzer_context.h lib/analyzer/checkers/restrict_checker.cpp lib/analyzer/checkers/restrict_checker.h lib/analyzer/core/abstract_checker.cpp lib/analyzer/core/abstract_checker.h lib/analyzer/core/analyzer_context.cpp lib/analyzer/checkers/bounds_checker.cpp lib/analyzer/checkers/bounds_checker.h lib/analyzer/core/violation.h lib/analyzer/core/optional_value.cpp lib/analyzer/core/optional_value.h lib/analyzer/core/memory_access_type.h lib/analyzer/c_api.h lib/analyzer/c_api.cpp lib/analyzer/core/pseudocl.h lib/analyzer/core/pseudocl.cpp lib/analyzer/core/analyzer_options.h)
set(LIBS ${OpenCL_LIBRARIES} ${CLANG_LIBRARIES} ${Z3_LIBRARIES})

add_executable(clsa ${LIB_SRCS} main.cpp)
target_link_libraries(clsa PRIVATE ${LIBS})

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

get_filename_component(TEST_BOUNDS_FILE_PATH test/resources/test_bounds.cl ABSOLUTE)
get_filename_component(TEST_RESTRICT_FILE_PATH test/resources/test_restrict.cl ABSOLUTE)
get_filename_component(TEST_ONE_DNN_FILE_PATH test/resources/test_one_dnn.cl ABSOLUTE)
get_filename_component(TEST_PIPE_CNN_FILE_PATH test/resources/test_pipe_cnn.cl ABSOLUTE)
configure_file(test/test_resources.h.in test_resources.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(tests ${LIB_SRCS} ${TEST_RESOURCES} test/test_utils.h test/test_utils.cpp test/test_bounds.cpp test/test_restrict.cpp)
target_link_libraries(tests PRIVATE ${LIBS} gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
