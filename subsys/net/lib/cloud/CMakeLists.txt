#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
	cloud.c
)
zephyr_include_directories(./include)

zephyr_linker_sources(SECTIONS custom-sections.ld)
