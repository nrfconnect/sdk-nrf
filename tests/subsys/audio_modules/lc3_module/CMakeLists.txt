#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Audio T2 LC3 module")

target_sources(app PRIVATE
	src/main.c
	src/fakes.c
	src/decoder_test.c
)

zephyr_include_directories(
	${NRFXLIB_DIR}/lc3/codec/inc
	${NRFXLIB_DIR}/lc3/platform/os/inc
	${NRFXLIB_DIR}/lc3/platform/os/baremetal/inc
)
