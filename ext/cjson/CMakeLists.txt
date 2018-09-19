#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# CMakeLists.txt - cJSON Library for Zephyr

zephyr_include_directories(.)
zephyr_library()
zephyr_library_sources(
  cJSON.c
  cJSON_os.c
)
