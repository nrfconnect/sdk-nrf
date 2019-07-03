#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(
	cloud.c
)
zephyr_include_directories(./include)

zephyr_linker_sources(SECTIONS custom-sections.ld)
