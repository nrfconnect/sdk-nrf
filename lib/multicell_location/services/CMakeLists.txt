#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(.)

zephyr_library_sources(location_service.c)

zephyr_library_sources_ifdef(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD nrf_cloud_integration.c)
zephyr_library_sources_ifdef(CONFIG_MULTICELL_LOCATION_SERVICE_HERE here_integration.c)
zephyr_library_sources_ifdef(CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK skyhook_integration.c)
zephyr_library_sources_ifdef(CONFIG_MULTICELL_LOCATION_SERVICE_POLTE polte_integration.c)
