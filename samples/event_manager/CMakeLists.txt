#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("Event Manager sample")

# Include application event headers
zephyr_library_include_directories(src/events)

# Application sources
# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
		src/events/ack_event.c
		src/events/config_event.c
		src/events/control_event.c
		src/events/measurement_event.c
)

target_sources(app PRIVATE
		src/modules/controller.c
		src/modules/sensor_simulated.c
		src/modules/stats.c
)
# NORDIC SDK APP END
