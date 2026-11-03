/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Shared test helpers for the DECT NR+ RPC client/server unit tests.
 *
 * These macros build raw nRF RPC packets (header + CBOR payload) so tests can assert
 * exactly what our client/server code sends, and inject canned packets as if they came
 * from the peer. The wire header layout below is copied from nrf_rpc.c (NRF_RPC_HEADER_SIZE
 * = 5 bytes: type, id, dst, src_group_id, dst_group_id) with a single RPC group ("dect_rpc"),
 * so src_group_id/dst_group_id are always 0 once bound. This mirrors
 * tests/subsys/net/openthread/rpc/common/test_rpc_env.h, adapted for the "dect_rpc" group
 * name and DECT NR+ RPC command/event IDs.
 *
 * NOTE: this file only encodes/decodes CBOR primitives used to build test stimuli or
 * assert exact bytes sent by our own encoders (dect_rpc_if.c or dect_rpc_shell.c). It does
 * not test nRF RPC's transport or serialization internals themselves.
 */

#include <zephyr/sys/util_macro.h>

/* Basic CBOR constants used when building expected/response payloads. */
#define CBOR_FALSE 0xf4
#define CBOR_TRUE  0xf5
#define CBOR_NULL  0xf6

/* CBOR unsigned integer encodings (zcbor_uint32_put style: shortest form). */
#define CBOR_UINT_SMALL(value) (value) /* value <= 0x17 encodes as itself */
#define CBOR_UINT8(value)      0x18, (value) /* 0x18..0xff */
#define CBOR_UINT16(value)     0x19, ((value) >> 8) & 0xff, (value) & 0xff
#define CBOR_UINT32(value)                                                                        \
	0x1a, ((value) >> 24) & 0xff, ((value) >> 16) & 0xff, ((value) >> 8) & 0xff, (value) & 0xff

/* CBOR negative integer (Zephyr errno on the wire), e.g. CBOR_NINT(22) -> -EINVAL (0x35). */
#define CBOR_NINT(value) (0x20 + ((value)-1))

/* CBOR negative integer -90..-24 (one-byte tail). */
#define CBOR_NINT8(value) 0x38, ((value)-1)

/* CBOR byte string header for a buffer of length len (len <= 23 for this helper). */
#define CBOR_BSTR(len, ...) (0x40 | (len)) __VA_OPT__(,) __VA_ARGS__

/* CBOR byte string header for a buffer of length 24..255 (1-byte length follows). */
#define CBOR_BSTR8(len, ...) (0x40 | 24), (len) __VA_OPT__(,) __VA_ARGS__

/** Build a raw nRF RPC packet from a byte list. */
#define RPC_PKT(bytes...)                                                                          \
	(mock_dect_rpc_pkt_t)                                                                      \
	{                                                                                          \
		.data = (uint8_t[]){bytes}, .len = sizeof((uint8_t[]){bytes}),                     \
	}

/* Group name is "dect_rpc" (8 bytes); see subsys/net/dect/rpc/common/dect_rpc_group.c.
 * Byte layout verified against nrf_rpc.c: send_init()/header_encode(): [type=0x04, id=0x00,
 * dst=0xff(unknown), src_group_id, dst_group_id, version=0x00, name...].
 */
#define RPC_INIT_REQ                                                                               \
	RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'd', 'e', 'c', 't', '_', 'r', 'p', 'c')
#define RPC_INIT_RSP                                                                               \
	RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'd', 'e', 'c', 't', '_', 'r', 'p', 'c')

/* Command header: [type=0x80|src(0), id=cmd, dst=0xff(unknown), src_group_id=0, dst_group_id=0].
 * Verified against header_cmd_encode() in nrf_rpc.c.
 */
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, CBOR_NULL)

/* Response header: [type=0x01, id=0xff(unknown), dst=0x00, src_group_id=0, dst_group_id=0].
 * Verified against nrf_rpc_rsp() in nrf_rpc.c (dst = cmd_ctx->remote_id, which is 0 for a
 * command received with src=0, i.e. sent via RPC_CMD above).
 */
#define RPC_RSP(...) RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, CBOR_NULL)

/* Event header: [type=0x00, id=evt, dst=0xff(unknown), src_group_id=0, dst_group_id=0].
 * Verified against nrf_rpc_evt() in nrf_rpc.c.
 */
#define RPC_EVT(evt, ...) RPC_PKT(0x00, evt, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, CBOR_NULL)

/* Event ACK the client/server stack sends after handling an incoming event. */
#define RPC_EVT_ACK(evt) RPC_PKT(0x02, evt, 0xff, 0x00, 0x00)

#define NO_RSP RPC_PKT()

/*
 * DECT_RPC_CMD_IF_STATUS response sent by the server when it has no "dect0" interface
 * (get_server_dect_if() fails) -- see the `if (!server_dect_if) { ... }` fallback branch
 * dect_rpc_status_rsp_encode() with dect_rpc_status_init_empty() (server/dect_rpc_if.c).
 * Shared by client and server tests so
 * both sides are checked against the exact same wire contract:
 *   - client/src/status_suite.c: server sends this, does the client decode/print it right?
 *   - server/src/status_suite.c: with no dect0 configured, does the server send exactly this?
 */
#define STATUS_RSP_NO_DECT_IF                                                                      \
	CBOR_FALSE,             /* mdm_activated */                                                \
		CBOR_FALSE,     /* cluster_running */                                              \
		CBOR_UINT_SMALL(0), /* cluster_channel */                                          \
		CBOR_FALSE,         /* nw_beacon_running */                                        \
		CBOR_UINT_SMALL(0), /* parent_count */                                             \
		CBOR_UINT_SMALL(0), /* child_count */                                              \
		CBOR_FALSE,         /* br_global_ipv6_addr_prefix_set */                           \
		CBOR_BSTR(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                     \
		/* prefix, unused */                                                               \
		CBOR_UINT_SMALL(0), /* prefix len, unused */                                       \
		CBOR_BSTR(1, 0x00)  /* fw_version_str: single NUL byte, not an empty string */

/*
 * Minimal IF_GET_ADDRS response: 0 addrs, 0 prefixes, MTU 1280, carrier on, dormant on.
 * Matches a booted-but-unassociated mock dect0 when CONFIG_NET_L2_DECT_MTU=1280.
 */
#define GET_ADDRS_RSP_BOOTED_UNASSOCIATED                                                          \
	CBOR_UINT_SMALL(0), CBOR_UINT_SMALL(0), CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE

#define GET_ADDRS_RSP_ASSOCIATED_CARRIER                                                           \
	CBOR_UINT_SMALL(0), CBOR_UINT_SMALL(0), CBOR_UINT16(1280), CBOR_TRUE, CBOR_FALSE

/* Server RPC shell disabled (CONFIG_DECT_NR_RPC_SHELL=n): status uint 1 + error bstr. */
#define SHELL_RSP_NOT_CONFIGURED                                                                   \
	CBOR_UINT_SMALL(1),                                                                        \
		CBOR_BSTR8(38, 'D', 'E', 'C', 'T', ' ', 'L', '2', ' ', 's', 'h', 'e', 'l', \
			   'l', ' ', 'n', 'o', 't', ' ', 'c', 'o', 'n', 'f', 'i', 'g', 'u', \
			   'r', 'e', 'd', ' ', 'o', 'n', ' ', 's', 'e', 'r', 'v', 'e', 'r')
