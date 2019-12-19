#
# Copyright (c) 2018-2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_GPS_SIM gps_sim)
add_subdirectory_ifdef(CONFIG_NRF9160_GPS nrf9160_gps)
