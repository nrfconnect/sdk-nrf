/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * nRF RPC mock transport for DECT NR+ RPC unit tests only (see mock_dect_rpc_transport.c).
 */

#ifndef MOCK_DECT_RPC_TRANSPORT_H_
#define MOCK_DECT_RPC_TRANSPORT_H_

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct nrf_rpc_tr mock_dect_rpc_tr;

typedef struct mock_dect_rpc_pkt {
	const uint8_t *data;
	size_t len;
} mock_dect_rpc_pkt_t;

void mock_dect_rpc_tr_expect_add(mock_dect_rpc_pkt_t expect, mock_dect_rpc_pkt_t response);

#define MOCK_DECT_RPC_EXPECT_ANY_SEND                                                          \
	mock_dect_rpc_tr_expect_add((mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 },               \
				    (mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 })

void mock_dect_rpc_tr_expect_done(void);
void mock_dect_rpc_tr_expect_reset(void);
size_t mock_dect_rpc_tr_sent_count(void);
void mock_dect_rpc_tr_receive(mock_dect_rpc_pkt_t packet);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_DECT_RPC_TRANSPORT_H_ */
