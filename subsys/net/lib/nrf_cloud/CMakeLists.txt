#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(
	src/nrf_cloud.c
	src/nrf_cloud_codec.c
	src/nrf_cloud_fsm.c
	src/nrf_cloud_transport.c
	src/nrf_cloud_sanity.c
)
zephyr_library_sources_ifdef(
	CONFIG_NRF_CLOUD_AGPS
	src/nrf_cloud_agps.c
	src/nrf_cloud_agps_utils.c)
zephyr_include_directories(./include)
