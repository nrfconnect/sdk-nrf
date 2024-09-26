#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_BATTERY_MEAS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/battery_event.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
