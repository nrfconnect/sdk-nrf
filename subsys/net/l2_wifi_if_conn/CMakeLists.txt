#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(l2_wifi_conn.c)

zephyr_library_include_directories(
    ${ZEPHYR_BASE}/include
    ${ZEPHYR_NRF_MODULE_DIR}/include
    ${ZEPHYR_HOSTAP_MODULE_DIR}
    ${ZEPHYR_BASE}/subsys/net/conn_mgr
    )
