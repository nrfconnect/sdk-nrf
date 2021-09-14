#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(rest_services_wlan.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WLAN_SERVICE_HERE rest_here_wlan.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK rest_skyhook_wlan.c)
