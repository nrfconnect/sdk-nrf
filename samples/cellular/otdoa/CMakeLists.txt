#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(otdoa)

target_compile_options(app PRIVATE -flto -fdata-sections -ffunction-sections)
target_link_options(app PRIVATE --gc-sections)

# Prep the TLS certificate for including
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})
generate_inc_file_for_target(
  app
  cert/hellaphy.pem
  ${CMAKE_CURRENT_BINARY_DIR}/hellaphy.pem.inc
)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/otdoa_sample_app.c)
target_sources(app PRIVATE src/otdoa_gpio.c)

target_include_directories(app PRIVATE ./include)

target_link_libraries(app PUBLIC m)

# GCC 11 by default emits DWARF version 5 which cannot be parsed by
# pyelftools. Can be removed once pyelftools supports v5.
add_link_options(-gdwarf-4)
