#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(rng.c)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
