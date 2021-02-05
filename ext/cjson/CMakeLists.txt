#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# CMakeLists.txt - cJSON Library for Zephyr

zephyr_include_directories(.)
zephyr_library()
zephyr_library_sources(
  cJSON.c
  cJSON_os.c
)
