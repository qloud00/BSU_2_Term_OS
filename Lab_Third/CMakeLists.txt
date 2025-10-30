cmake_minimum_required(VERSION 3.20)
project(MarkerRandomArraySTLThreads)

set(CMAKE_CXX_STANDARD 23)

add_executable(MarkerThreads
        main.cpp
        header.h
        functions.cpp
)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        9706f75b8f91c52a3840cf5d878a7f37ea10ef00
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(MarkerArrayTests
        tests/tests.cpp
        header.h
        functions.cpp
)

target_link_libraries(MarkerArrayTests GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(MarkerArrayTests)