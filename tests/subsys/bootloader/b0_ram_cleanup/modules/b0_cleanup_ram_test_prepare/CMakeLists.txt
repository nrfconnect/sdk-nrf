#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

zephyr_library()
zephyr_library_sources(src/b0_cleanup_ram_test_prepare.c)
