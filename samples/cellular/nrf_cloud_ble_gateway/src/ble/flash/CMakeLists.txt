#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(include)
target_sources_ifdef(
	CONFIG_FLASH_TEST
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/flash_test.c
	)
