#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sd_card)

target_sources(app
	PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/sd_card.c
	src/main.c
	)

target_include_directories(app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/
  ${ZEPHYR_NRF_MODULE_DIR}/modules/fs/fatfs/include/
)
