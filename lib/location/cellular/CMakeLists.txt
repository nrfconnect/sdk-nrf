#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(cellular_service.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_SERVICE_NRF_CLOUD cellular_nrf_cloud.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_SERVICE_HERE cellular_here_rest.c)
