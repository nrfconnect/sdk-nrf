/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

#define DATA_SIZE  100

/* CBOR constants */

#define CBOR_FALSE 0xf4
#define CBOR_TRUE  0xf5
#define CBOR_NULL  0xf6

#define LSFY_IDENTITY(n, _) n + 1
#define INT_SEQUENCE(length)  LISTIFY(length, LSFY_IDENTITY, (,))

/* Macros for constructing nRF RPC packets for the OpenThread command group. */

#define RPC_PKT(bytes...)                                                                          \
	(mock_nrf_rpc_pkt_t)                                                                       \
	{                                                                                          \
		.data = (uint8_t[]){bytes}, .len = sizeof((uint8_t[]){bytes}),                     \
	}

#define RPC_INIT_REQ      RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'n', 'f', 'c')
#define RPC_INIT_RSP      RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'n', 'f', 'c')
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define RPC_RSP(...)      RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define NO_RSP            RPC_PKT()

/* Other test data */

#define CBOR_UINT32(value) 0x1A, BT_BYTES_LIST_LE32(BSWAP_32(value))
#define CBOR_UINT16(value) 0x19, BT_BYTES_LIST_LE16(BSWAP_16(value))
/* for value bigger than 0x17 */
#define CBOR_UINT8(value)  0x18, (value)
#define NFC_DATA           0x58, 0x64, INT_SEQUENCE(DATA_SIZE)
