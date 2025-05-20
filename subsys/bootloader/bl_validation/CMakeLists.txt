#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/bl_validation_magic.cmake)
zephyr_library()
zephyr_library_sources(bl_validation.c)
