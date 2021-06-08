#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

zephyr_library()

zephyr_library_sources(nrf_rpc_os.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_TR_RPMSG nrf_rpc_rpmsg.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_TR_RPMSG rp_ll.c)
