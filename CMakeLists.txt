cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_BUILD_TESTS "Build tests" OFF)

set(WARNINGS -Wall -Wextra -Wshadow -Wconversion -Wpedantic)

add_subdirectory(src)

if(ENABLE_BUILD_TESTS)
    add_subdirectory(test)
endif()
