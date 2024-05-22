include_directories(${CMAKE_SOURCE_DIR}/qss/src)
add_executable(main main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(main PRIVATE Threads::Threads)
target_compile_options(main PRIVATE ${WARNINGS} -O3 -march=${CUSTOM_MARCH} -msse3)

add_executable(stat only_stat.cpp)
target_compile_options(stat PRIVATE ${WARNINGS} -O3)
