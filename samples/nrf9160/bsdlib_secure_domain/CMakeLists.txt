# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(hello_world)

target_sources(app PRIVATE src/main.c)

# Code relocation
zephyr_code_relocate(src/main.c SRAM2_TEXT)
zephyr_code_relocate(src/main.c SRAM_DATA)
zephyr_code_relocate(src/main.c SRAM2_BSS)
