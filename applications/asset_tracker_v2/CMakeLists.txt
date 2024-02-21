#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(asset_tracker_v2)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE include/${BOARD}/)
zephyr_include_directories_ifdef(CONFIG_MEMFAULT configuration/memfault)

# Application directories
add_subdirectory(src/modules)
add_subdirectory(src/events)

add_subdirectory_ifdef(CONFIG_CLOUD_MODULE src/cloud)
add_subdirectory_ifdef(CONFIG_SENSOR_MODULE src/ext_sensors)
add_subdirectory_ifdef(CONFIG_WATCHDOG_APPLICATION src/watchdog)

# Include nRF modem library header file for PC builds.
# These are used throughout the application in type definitions.
if (CONFIG_BOARD_QEMU_X86 OR CONFIG_BOARD_NATIVE_POSIX)
	target_include_directories(app PRIVATE ${NRFXLIB_DIR}/nrf_modem/include/)

	# Make the folder that contains the certificates global so that it can be located by the
	# nRF Cloud library. For the other clouds integrations, the certificates are handled by the
	# MQTT helper library, see CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES.
	if (CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
		zephyr_include_directories(src/cloud-certs)
	endif()

	target_compile_options(app PRIVATE
		-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10)
endif()

if(CONFIG_ASSET_TRACKER_V2_LTO)
	target_compile_options(app PRIVATE "-flto")
endif()
