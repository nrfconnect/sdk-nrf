cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(at_cmd_parser)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
