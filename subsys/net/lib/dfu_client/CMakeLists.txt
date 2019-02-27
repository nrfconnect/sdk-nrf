#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(
	src/http_dfu_client.c
	src/http_client.c
)
zephyr_include_directories(./include)