#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_device.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_security.c)

target_sources_ifdef(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_location.c
	)
target_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_firmware.c
	)
target_sources_ifdef(CONFIG_LWM2M_CONN_MON_OBJ_SUPPORT
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_connmon.c
	)
target_sources_ifdef(CONFIG_LWM2M_IPSO_LIGHT_CONTROL
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_light_control.c
	)
target_sources_ifdef(CONFIG_LWM2M_IPSO_TEMP_SENSOR
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_temp.c
	)
target_sources_ifdef(CONFIG_UI_BUZZER
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_buzzer.c
	)
target_sources_ifdef(CONFIG_UI_BUTTON
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_button.c
	)
target_sources_ifdef(CONFIG_LWM2M_IPSO_ACCELEROMETER
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m_accelerometer.c
	)
