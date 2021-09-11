# This CMake file is for the main project
# that includes the compiler for nyla

cmake_minimum_required (VERSION 3.8)

# Some of these warnings on MSVC are rather annoying so ive disabled them
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd\"26812\"")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd\"26495\"")
endif()

set(CMAKE_CXX_STANDARD 14)

#importing LLVM
find_package(LLVM REQUIRED CONFIG)

# Including LLVM source
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})

# Add source to this project's executable.
add_library (nyla    "compiler.h" "compiler.cpp" "log.h" "log.cpp" "utils.h" "types_ext.h" "utils.cpp" "source.h" "source.cpp" "lexer.h" "tokens.h" "lexer.cpp" "tokens.cpp" "words.h" "words.cpp" "parser.h" "parser.cpp" "ast.h" "ast.cpp" "sym_table.h" "modifiers.h" "modifiers.cpp" "sym_table.cpp" "type.h" "type.cpp" "analysis.h" "analysis.cpp" "llvm_gen.h" "llvm_gen.cpp" "code_gen.h" "code_gen.cpp"  "file_location.h")
target_include_directories (nyla PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (nyla PUBLIC ${LLVM_INCLUDE_DIRS})

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
