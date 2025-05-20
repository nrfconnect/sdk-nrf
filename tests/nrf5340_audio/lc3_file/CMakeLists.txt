#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_lc3_file)

# lc3_file source must be added manually as kconfigs and CMakeLists in nRF5340 audio application
# is not available from here.
target_sources(app
	PRIVATE
	src/main.c
	${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/lc3_file.c
	${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/sd_card/sd_card_fake.c
	${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/sd_card/lc3_file_data.c
	)

target_compile_definitions(app PRIVATE CONFIG_MODULE_SD_CARD_LC3_FILE_LOG_LEVEL=3)
target_include_directories(app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src
	${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes)
