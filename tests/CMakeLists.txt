# This CMake file is for a project
# that performs test on the nyla compiler

project ("tests")

cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable (tests "nyla_tests.cpp" "test_suite.h")

target_link_libraries(tests LINK_PUBLIC nyla)

# Moving over resources
add_custom_target(copy_resources_target
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/tests/resources
)
add_dependencies(tests copy_resources_target)