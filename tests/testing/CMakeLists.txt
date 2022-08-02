cmake_minimum_required(VERSION 3.22)

# Make sure our C++ standard is sane
add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-std=c99>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++2a>
)

# Set up the 'app' project for the testing
add_executable(app)
project(app)

# Set up gtest with its boilerplate
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Make sure our app links with gtest
target_link_libraries(
    app
    PRIVATE
        gtest_main
)

include(GoogleTest)

gtest_discover_tests(app)

enable_testing()

# Set up the tests that we'll run
target_include_directories(
    app
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
)

target_sources(
    app
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/basic.cpp"
)
