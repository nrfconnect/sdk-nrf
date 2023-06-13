#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ipso_colour_sensor.c)
