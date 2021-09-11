# Executable for the project's compiler

project ("driver")

cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable (driver "driver.cpp")

#Setting the name of the executable that is generated to be nylac
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "nylac")

target_link_libraries(driver LINK_PUBLIC nyla)