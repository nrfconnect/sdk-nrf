/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

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

#define RPC_INIT_REQ      RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'o', 't')
#define RPC_INIT_RSP      RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'o', 't')
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define RPC_RSP(...)      RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define NO_RSP            RPC_PKT()

/* Other test data */

#define CBOR_UINT32(value) 0x1A, BT_BYTES_LIST_LE32(BSWAP_32(value))
#define CBOR_UINT16(value) 0x19, BT_BYTES_LIST_LE16(BSWAP_16(value))
/* for value bigger than 0x17 */
#define CBOR_UINT8(value)  0x18, (value)

#define MADDR_FF02_1  0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01
#define EXT_ADDR      0x48, INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)
#define NWK_NAME      0x70, INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE)
#define EXT_PAN_ID    0x48, INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)
#define STEERING_DATA 0x50, INT_SEQUENCE(OT_STEERING_DATA_MAX_LENGTH)
#define TLVS          0x58, 0xFE, INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)
