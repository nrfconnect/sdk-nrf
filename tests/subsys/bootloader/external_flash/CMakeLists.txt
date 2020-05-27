# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(mcuboot_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/mcuboot_prj.conf)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(external_flash_test)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE src)
