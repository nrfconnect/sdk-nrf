/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SOCKET_MOCK_H
#define SOCKET_MOCK_H

#include <stdbool.h>
#include <stdio.h>

extern struct mock_socket_iface_data mock_socket_iface_data;
extern struct net_if_api mock_if_api;

void test_set_ret_val_socket_offload_recvfrom(size_t *values, size_t len);
void test_set_ret_val_socket_offload_sendto(size_t *values, size_t len);
void test_socket_mock_teardown(void);

bool mock_socket_is_supported(int family, int type, int proto);
int mock_socket_create(int family, int type, int proto);

#endif /* SOCKET_MOCK_H */
