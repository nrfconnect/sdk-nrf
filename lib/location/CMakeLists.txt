#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(location.c)
zephyr_library_sources(loc_core.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_GNSS method_gnss.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_CELLULAR method_cellular.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WLAN method_wlan.c)
add_subdirectory_ifdef(CONFIG_LOCATION_METHOD_WLAN rest)
