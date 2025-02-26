#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bootloader)

zephyr_library_sources(src/main.c)

if(CONFIG_SOC_SERIES_NRF54LX)
	set(OUTPUT_FILE_PATH ${APPLICATION_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr)
	set(B0_SIZE ${CONFIG_PM_PARTITION_SIZE_B0_IMAGE})


	add_custom_command(OUTPUT ${OUTPUT_FILE_PATH}/b0_lock.hex
		 COMMAND
		 ${PYTHON_EXECUTABLE}
		 ${ZEPHYR_NRF_MODULE_DIR}/scripts/reglock.py
		 -o ${OUTPUT_FILE_PATH}/b0_lock.hex
		 -s ${B0_SIZE}
		 DEPENDS
		 ${OUTPUT_FILE_PATH}/zephyr.hex
	)

	add_custom_command(OUTPUT ${OUTPUT_FILE_PATH}/merged.hex
		 COMMAND
		 ${PYTHON_EXECUTABLE}
		 ${ZEPHYR_BASE}/scripts/build/mergehex.py
		 -o ${OUTPUT_FILE_PATH}/zephyr.hex
		 ${OUTPUT_FILE_PATH}/b0_lock.hex
		 ${OUTPUT_FILE_PATH}/zephyr.hex
		 DEPENDS
		 ${OUTPUT_FILE_PATH}/b0_lock.hex
		 ${OUTPUT_FILE_PATH}/zephyr.hex
	)

	add_custom_target(merged_hex
		ALL DEPENDS
		${OUTPUT_FILE_PATH}/merged.hex
		${OUTPUT_FILE_PATH}/b0_lock.hex
	)

	add_custom_target(b0_lock_hex
		ALL DEPENDS
		${OUTPUT_FILE_PATH}/b0_lock.hex
	)

endif()
