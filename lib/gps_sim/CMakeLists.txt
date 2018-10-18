#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_GPS_SIM)
    zephyr_library()
    zephyr_library_sources(gps_sim.c)
endif()
