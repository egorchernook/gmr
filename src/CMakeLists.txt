add_executable(main main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(main PRIVATE Threads::Threads)

add_executable(stat only_stat.cpp)
