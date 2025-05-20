#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Application Event Manager Profiler sample")

# Include application application event headers
zephyr_library_include_directories(src/events)

# Application sources
# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
		src/events/five_sec_event.c
		src/events/config_event.c
		src/events/one_sec_event.c
		src/events/burst_event.c
)

target_sources(app PRIVATE
		src/modules/module_a.c
		src/modules/module_b.c
)
# NORDIC SDK APP END
