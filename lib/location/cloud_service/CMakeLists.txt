#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(cloud_service.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_SERVICE_NRF_CLOUD cloud_service_nrf.c)
zephyr_library_sources_ifdef(CONFIG_LOCATION_SERVICE_HERE cloud_service_here_rest.c)
