#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Point to NCS root directory.
set(NRF_DIR ${CMAKE_CURRENT_LIST_DIR})

zephyr_include_directories(include)

add_subdirectory(ext)
add_subdirectory(lib)
add_subdirectory(subsys)
add_subdirectory(drivers)
add_subdirectory(tests/unity)
