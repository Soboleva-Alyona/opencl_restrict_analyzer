cmake_minimum_required(VERSION 3.12)
project(opencl_restrict_analyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti)

find_package(Clang REQUIRED CONFIG)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")

include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})
set(CLANG_LIBRARIES clangFrontend clangTooling clang)

find_package(Z3 REQUIRED)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

add_executable(opencl_restrict_analyzer main.cpp lib/analyzer/core/analyzer_parameters.h lib/analyzer/frontend/analyzer.cpp lib/analyzer/frontend/analyzer.h lib/analyzer/core/ast_consumer.cpp lib/analyzer/core/ast_consumer.h lib/analyzer/core/ast_visitor.cpp lib/analyzer/core/ast_visitor.h lib/analyzer/core/block.cpp lib/analyzer/core/block.h lib/analyzer/core/analyzer_context.h lib/analyzer/checkers/restrict_checker.cpp lib/analyzer/checkers/restrict_checker.h lib/analyzer/core/abstract_checker.cpp lib/analyzer/core/abstract_checker.h lib/analyzer/core/analyzer_context.cpp lib/analyzer/checkers/address_checker.cpp lib/analyzer/checkers/address_checker.h lib/analyzer/core/violation.h lib/analyzer/core/optional_value.cpp lib/analyzer/core/optional_value.h lib/analyzer/core/memory_access_type.h lib/analyzer/c_api.h lib/analyzer/c_api.cpp lib/analyzer/core/pseudocl.h lib/analyzer/core/pseudocl.cpp lib/analyzer/core/analyzer_options.h)

target_link_libraries(opencl_restrict_analyzer PRIVATE ${OpenCL_LIBRARIES} ${CLANG_LIBRARIES} ${Z3_LIBRARIES})
