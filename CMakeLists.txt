# More recent versions of CMake exist.
# Version 3.20 is chosen here to ensure the examples work everywhere.
cmake_minimum_required(VERSION 3.20)

project(beatbox_64889)

# More recent C++ versions of the C++ standard exist.
# Standard C++20 has all the features needed to illustrate the
# core concepts taught in 4DEV, and is widely supported.
set(CMAKE_CXX_STANDARD 20)

add_executable(bbx-64889 main.cpp)

include_directories( PRIVATE
        "."
        "libraries/imgui"
        "libraries/imgui/backend"
)
