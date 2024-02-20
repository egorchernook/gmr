include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

set(TARGET ${PROJECT_NAME}-unit)
# set(FLAGS ${CMAKE_CXX_FLAGS_DEBUG} -O0 -fprofile-arcs -ftest-coverage)
file(GLOB_RECURSE TEST_SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_executable(${TARGET} "${TEST_SOURCE_FILES}")

target_link_libraries(
    ${TARGET}
    GTest::gtest_main
)
# target_compile_options(${TARGET} PRIVATE ${FLAGS})

add_test(gtests ${TARGET})
