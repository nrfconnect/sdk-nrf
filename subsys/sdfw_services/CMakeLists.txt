#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(os)

zephyr_sources(ssf_client.c)
zephyr_sources(ssf_client_notif.c)
zephyr_sources(os/ssf_client_zephyr.c)

add_subdirectory(zcbor)
add_subdirectory(services)
add_subdirectory(transport)
