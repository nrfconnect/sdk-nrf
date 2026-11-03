/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RPC_TEST_RSP_BUILDER_H_
#define RPC_TEST_RSP_BUILDER_H_

#include <mock_dect_rpc_transport.h>

#include <zephyr/net/net_if.h>

/** Build expected IF_GET_ADDRS RPC_RSP for the current server dect0 mirror fields. */
mock_dect_rpc_pkt_t rpc_test_expect_get_addrs_rsp(struct net_if *iface);

#endif /* RPC_TEST_RSP_BUILDER_H_ */
