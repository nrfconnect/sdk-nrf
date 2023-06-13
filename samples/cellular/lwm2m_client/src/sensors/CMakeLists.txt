#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_ACCELEROMETER
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/accelerometer.c)

target_sources_ifdef(CONFIG_ENVIRONMENT_SENSOR
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/env_sensor.c)

target_sources_ifdef(CONFIG_LIGHT_SENSOR
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/light_sensor.c)

target_sources_ifdef(CONFIG_GNSS
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_module.c)
