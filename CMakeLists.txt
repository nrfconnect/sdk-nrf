#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

include(../fw-nrfconnect-zephyr/CMakeLists.txt)
zephyr_include_directories(include)
add_subdirectory(lib)

add_subdirectory(drivers)add_subdirectory(drivers)

