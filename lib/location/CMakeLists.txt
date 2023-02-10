#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(location.c)
zephyr_library_sources(location_core.c)
zephyr_library_sources(location_utils.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_GNSS method_gnss.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_CELLULAR method_cellular.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI method_wifi.c)

if(CONFIG_LOCATION_METHOD_CELLULAR OR CONFIG_LOCATION_METHOD_WIFI)
add_subdirectory(cloud_service)
endif()
