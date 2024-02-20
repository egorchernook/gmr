find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(simple_thread_pool_use simple_thread_pool_use.cpp)
target_link_libraries(simple_thread_pool_use PRIVATE Threads::Threads)
