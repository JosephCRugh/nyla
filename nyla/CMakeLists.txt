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

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB nyla_sources
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Add source to this project's executable.
add_executable (nyla ${nyla_sources})

# Moving over resources
add_custom_target(copy_resources_target
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/nyla/resources
)
add_dependencies(nyla copy_resources_target)

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

message(STATUS "Libraries being linked: ${llvm_libs}")

#Linking the LLVM Libraries
target_link_libraries(nyla ${llvm_libs})
