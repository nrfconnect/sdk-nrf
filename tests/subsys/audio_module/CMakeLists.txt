#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Audio module")

target_sources(app PRIVATE
	src/main.c
	src/fakes.c
	src/audio_module_test_common.c
	src/bad_param_test.c
	src/functional_test.c
)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/audio_module)
