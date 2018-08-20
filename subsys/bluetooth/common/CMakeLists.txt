#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_link_libraries(nrf_bluetooth)

zephyr_library_sources_ifdef(CONFIG_NRF_BT_SVC_COMMON svc_common.c)
