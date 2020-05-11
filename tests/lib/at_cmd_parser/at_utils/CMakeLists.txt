cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(at_cmd_parser)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
