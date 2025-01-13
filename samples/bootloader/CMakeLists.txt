#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bootloader)

zephyr_library_sources(src/main.c)

if(CONFIG_SB_BOOTCONF_LOCK_WRITES)

	set(OUTPUT_FILE_PATH ${APPLICATION_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr)
	set(B0_SIZE ${CONFIG_PM_PARTITION_SIZE_B0_IMAGE})
	set(B0_LOCK_FILE "b0_lock.hex")
	set(MERGED_HEX_FILE "zephyr_with_lock.hex")

	add_custom_command(OUTPUT ${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
		 COMMAND
		 ${PYTHON_EXECUTABLE}
		 ${ZEPHYR_NRF_MODULE_DIR}/scripts/reglock.py
		 -o ${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
		 -s ${B0_SIZE}
		 DEPENDS
		 ${OUTPUT_FILE_PATH}/zephyr.hex
	)

	add_custom_command(OUTPUT ${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE}
		 COMMAND
		 ${PYTHON_EXECUTABLE}
		 ${ZEPHYR_BASE}/scripts/build/mergehex.py
		 -o ${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE}
		 ${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
		 ${OUTPUT_FILE_PATH}/zephyr.hex
		 DEPENDS
		 ${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
		 ${OUTPUT_FILE_PATH}/zephyr.hex
	)

	add_custom_target(b0_lock_hex
		ALL DEPENDS
		${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
	)

	add_custom_target(zephyr_with_lock_hex
		ALL DEPENDS
		${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE}
		${OUTPUT_FILE_PATH}/zephyr.hex
		${OUTPUT_FILE_PATH}/${B0_LOCK_FILE}
	)

	set(byproducts ${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE})
	set_target_properties(runners_yaml_props_target PROPERTIES "hex_file" "${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE}")
	set(BYPRODUCT_KERNEL_HEX_NAME "${OUTPUT_FILE_PATH}/${MERGED_HEX_FILE}"
		CACHE FILEPATH "B0 lock hex file" FORCE
	)
	set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})

endif()
