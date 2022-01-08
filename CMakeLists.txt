cmake_minimum_required(VERSION 3.12)
project(opencl_restrict_analyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti)

find_package(Clang REQUIRED CONFIG)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")

include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})
set(CLANG_LIBRARIES clangFrontend clangTooling clang)

find_package(Z3 REQUIRED)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

add_executable(opencl_restrict_analyzer main.cpp lib/analyzer/analyzer_parameters.h lib/analyzer/analyzer.cpp lib/analyzer/analyzer.h lib/analyzer/ast_consumer.cpp lib/analyzer/ast_consumer.h lib/analyzer/ast_visitor.cpp lib/analyzer/ast_visitor.h lib/analyzer/model/block.cpp lib/analyzer/model/block.h lib/analyzer/model/var.cpp lib/analyzer/model/var.h lib/analyzer/analyzer_context.h lib/analyzer/checkers/restrict_checker.cpp lib/analyzer/checkers/restrict_checker.h lib/analyzer/checkers/abstract_checker.cpp lib/analyzer/checkers/abstract_checker.h lib/analyzer/model/memory.cpp lib/analyzer/model/memory.h lib/analyzer/analyzer_context.cpp lib/analyzer/checkers/address_checker.cpp lib/analyzer/checkers/address_checker.h)

target_link_libraries(opencl_restrict_analyzer PRIVATE ${OpenCL_LIBRARIES} ${CLANG_LIBRARIES} ${Z3_LIBRARIES})
