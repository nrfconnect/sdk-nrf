#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/app_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/cloud_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/gps_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/modem_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/data_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/sensor_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/ui_module_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/util_module_event.c
)
