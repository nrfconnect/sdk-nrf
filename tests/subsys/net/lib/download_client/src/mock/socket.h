/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <zephyr/kernel.h>
#include <zephyr/net/offloaded_netdev.h>

extern struct mock_socket_iface_data mock_socket_iface_data;
extern struct offloaded_if_api mock_if_api;

int mock_nrf_modem_lib_socket_offload_init(const struct device *arg);
bool mock_socket_is_supported(int family, int type, int proto);
int mock_socket_create(int family, int type, int proto);

#endif /* _SOCKET_H_ */
