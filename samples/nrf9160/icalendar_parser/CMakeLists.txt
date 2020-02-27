cmake_minimum_required(VERSION 3.13.1)

include(../../../cmake/boilerplate.cmake NO_POLICY_SCOPE)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(ical_subscriber)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
