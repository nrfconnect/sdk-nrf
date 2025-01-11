#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
target_sources_ifdef(
	CONFIG_FLASH_TEST
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/flash_test.c
	)
