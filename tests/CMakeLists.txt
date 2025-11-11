#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

add_subdirectory_ifdef(CONFIG_UNITY unity)
add_subdirectory(mocks)
