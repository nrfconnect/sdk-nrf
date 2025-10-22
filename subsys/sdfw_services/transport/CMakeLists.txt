#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

add_subdirectory_ifdef(CONFIG_SSF_NRF_RPC nrf_rpc)
