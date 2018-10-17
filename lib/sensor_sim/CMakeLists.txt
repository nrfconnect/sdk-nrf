#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_SENSOR_SIM)
    zephyr_library()
    zephyr_library_sources(sensor_sim.c)
    zephyr_library_include_directories(.)
endif()
