#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(rest_services_wifi.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD rest_nrf_cloud_wifi.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE rest_here_wifi.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK rest_skyhook_wifi.c)
