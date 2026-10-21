/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * RPC event expectations for L2 association transitions (must expand at test top level).
 */

#ifndef RPC_TEST_LINK_STATE_EXPECT_H_
#define RPC_TEST_LINK_STATE_EXPECT_H_

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#define EXPECT_ADDRS_CHANGED_PAD                                                                   \
	mock_dect_rpc_tr_expect_add(                                                               \
		RPC_EVT(DECT_RPC_CMD_IF_ADDRS_CHANGED, CBOR_UINT_SMALL(0)), NO_RSP)

#if defined(CONFIG_DECT_NR_RPC_SHELL)
/* IF_ADDRS_CHANGED and SHELL_LINE interleave during associate/release; pad with wildcards. */
#define EXPECT_LINK_STATE_POST_LINK_EVENTS()                                                       \
	do {                                                                                       \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
		MOCK_DECT_RPC_EXPECT_ANY_SEND;                                                     \
	} while (0)
#else
#define EXPECT_LINK_STATE_POST_LINK_EVENTS()                                                       \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD;                                                                  \
	EXPECT_ADDRS_CHANGED_PAD
#endif

#define EXPECT_LINK_STATE_TRANSITION(carrier_byte, dormant_byte)                                   \
	mock_dect_rpc_tr_expect_add(                                                               \
		RPC_EVT(DECT_RPC_CMD_IF_LINK_STATE, (carrier_byte), (dormant_byte)), NO_RSP);     \
	EXPECT_LINK_STATE_POST_LINK_EVENTS()

#endif /* RPC_TEST_LINK_STATE_EXPECT_H_ */
