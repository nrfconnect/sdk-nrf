cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE src/main.c)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

project(bluetooth_sample_test_app)
