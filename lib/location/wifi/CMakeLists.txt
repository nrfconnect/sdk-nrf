#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(wifi_service.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD wifi_nrf_cloud_rest.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE wifi_here_rest.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK wifi_skyhook_rest.c)
