/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef L2_WIFI_CONNECT_H__
#define L2_WIFI_CONNECT_H__

#include <zephyr/net/conn_mgr_connectivity_impl.h>

#define L2_CONN_WLAN0_CTX_TYPE void *
CONN_MGR_CONN_DECLARE_PUBLIC(L2_CONN_WLAN0);

#endif /* L2_WIFI_CONNECT_H__ */
