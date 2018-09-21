cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
	${app_sources}
	${ZEPHYR_BASE}/samples/bluetooth/gatt/bas.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
