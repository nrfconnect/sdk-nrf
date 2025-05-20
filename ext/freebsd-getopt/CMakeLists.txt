#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# CMakeLists.txt - Getopt Library for Zephyr

zephyr_include_directories(.)
zephyr_library()

zephyr_library_sources(
  getopt.c
  getopt_long.c
)
