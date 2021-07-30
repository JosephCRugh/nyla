# CMakeList.txt : CMake project for nyla, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)

# Telling CMake we are running x64.
set(BUILD_ARCH "-m64")
set(TARGET_64 ON)

#importing LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})

# Add source to this project's executable.
add_executable (nyla "nyla.cpp" "lexer.h" "reader.cpp" "reader.h" "tests.h" "lexer.cpp" "console_colors.h" "console_colors.cpp" "types_ext.h" "test_suite.h" "tests.cpp" "token.h" "name.h" "name.cpp" "parser.h" "parser.cpp" "ast.h" "type.h")

#LLVM Components provide access to the underlying libraries without having
# to specify the libraries being used.
llvm_map_components_to_libnames(llvm_libs
  Analysis
  Core
  ExecutionEngine
  InstCombine
  Object
  OrcJIT
  RuntimeDyld
  ScalarOpts
  Support
  TransformUtils
  native
  )

#Linking the LLVM Libraries
target_link_libraries(nyla ${llvm_libs})