#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/location_shell.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/location_cmd_utils.c)

if(CONFIG_LOCATION_SERVICE_EXTERNAL)
	target_sources_ifdef(CONFIG_MOSH_CLOUD_MQTT app PRIVATE
			     ${CMAKE_CURRENT_SOURCE_DIR}/location_srv_ext_nrf_cloud.c)
	target_sources_ifdef(CONFIG_MOSH_CLOUD_LWM2M app PRIVATE
			     ${CMAKE_CURRENT_SOURCE_DIR}/location_srv_ext_lwm2m.c)
endif()
