#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_BSD_LIBRARY bsdlib)
add_subdirectory_ifdef(CONFIG_GPS_SIM gps_sim)

if (CONFIG_DK_LIBRARY OR
    CONFIG_UI_OUT_LIBRARY)
add_subdirectory(dk_buttons_and_leds)
endif()

add_subdirectory_ifdef(CONFIG_SENSOR_SIM sensor_sim)
add_subdirectory_ifdef(CONFIG_AT_HOST_LIBRARY at_host)
add_subdirectory_ifdef(CONFIG_UI_OUT_LIBRARY ui)

