/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

/* Macros for constructing CBOR data items.
 * CBOR encoding reference: RFC 8949 - https://www.rfc-editor.org/rfc/rfc8949.html
 */

#define CBOR_FALSE 0xf4
#define CBOR_TRUE  0xf5
#define CBOR_NULL  0xf6

#define CBOR_UINT64(value) 0x1B, BT_BYTES_LIST_LE64(BSWAP_64(value))
#define CBOR_UINT32(value) 0x1A, BT_BYTES_LIST_LE32(BSWAP_32(value))
#define CBOR_UINT16(value) 0x19, BT_BYTES_LIST_LE16(BSWAP_16(value))
/* for value bigger than 0x17 */
#define CBOR_UINT8(value)  0x18, (value)

/* Macros for constructing nRF RPC packets for the OpenThread command group. */

#define RPC_PKT(bytes...)                                                                          \
	(mock_nrf_rpc_pkt_t)                                                                       \
	{                                                                                          \
		.data = (uint8_t[]){bytes}, .len = sizeof((uint8_t[]){bytes}),                     \
	}

#define RPC_INIT_REQ                                                                               \
	RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'r', 'p', 'c', '_', 'u', 't', 'i', 'l', 's')
#define RPC_INIT_RSP                                                                               \
	RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'r', 'p', 'c', '_', 'u', 't', 'i', 'l', 's')
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define RPC_RSP(...)	  RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define NO_RSP		  RPC_PKT()

#define STRING_TO_TUPLE(str)                                                                       \
	str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9], str[10],   \
		str[11]
