cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(qss/src)
add_subdirectory(src)

set(WARNINGS "-Wall -Wextra -Wshadow -Wconversion -Wpedantic")
add_compile_options(WARNINGS "-O3")
