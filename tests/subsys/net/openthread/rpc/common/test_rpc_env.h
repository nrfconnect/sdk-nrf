/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

/* Macros for constructing CBOR data items. */

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

#define RPC_INIT_REQ      RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'o', 't')
#define RPC_INIT_RSP      RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'o', 't')
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define RPC_RSP(...)      RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define NO_RSP            RPC_PKT()

/* Dummy test data */

#define LSFY_IDENTITY(n, _)  n + 1
#define INT_SEQUENCE(length) LISTIFY(length, LSFY_IDENTITY, (,))

#define LSFY_CHAR(n, _)	     'a'
#define STR_SEQUENCE(length) LISTIFY(length, LSFY_CHAR, (,))

#define ADDR_1                                                                                     \
	0xfe, 0x80, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa,  \
		0x01
#define ADDR_2                                                                                     \
	0xfe, 0x80, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa, 0xbb, 0xaa,  \
		0x02
#define PORT_1	  0xff01
#define PORT_2	  0xff02
#define HOP_LIMIT 64
#define DNS_NAME                                                                                   \
	STR_SEQUENCE(63), '.', STR_SEQUENCE(63), '.', STR_SEQUENCE(63), '.', STR_SEQUENCE(63)

#define MADDR_FF02_1  0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01
#define EXT_ADDR      0x48, INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)
#define NWK_NAME      0x70, INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE)
#define EXT_PAN_ID    0x48, INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)
#define STEERING_DATA 0x50, INT_SEQUENCE(OT_STEERING_DATA_MAX_LENGTH)
#define TLVS          0x58, 0xFE, INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)
#define NWK_KEY       0x50, INT_SEQUENCE(OT_NETWORK_KEY_SIZE)
#define LOCAL_PREFIX  0x48, INT_SEQUENCE(OT_IP6_PREFIX_SIZE)
#define PSKC          0x50, INT_SEQUENCE(OT_PSKC_MAX_SIZE)
#define TIMESTAMP     CBOR_UINT64(0x0123456789abcdef), CBOR_UINT16(0x1234), CBOR_FALSE
#define SECURITY_POLICY                                                                            \
	CBOR_UINT16(0x9876), CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE,  \
		CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, 0x07
#define DATASET_COMPONENTS                                                                         \
	CBOR_FALSE, CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE,           \
		CBOR_TRUE, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE, CBOR_TRUE
#define DATASET                                                                                    \
	TIMESTAMP, TIMESTAMP, NWK_KEY, NWK_NAME, EXT_PAN_ID, LOCAL_PREFIX,                         \
		CBOR_UINT32(0x12345678), CBOR_UINT16(0xabcd), CBOR_UINT16(0xef67), PSKC,           \
		SECURITY_POLICY, CBOR_UINT32(0xfedcba98), DATASET_COMPONENTS
#define CBOR_MSG_INFO                                                                              \
	0x50, ADDR_1, 0x50, ADDR_2, CBOR_UINT16(PORT_1), CBOR_UINT16(PORT_2), CBOR_UINT8(64), 3,   \
		CBOR_TRUE, CBOR_TRUE, CBOR_TRUE
#define CBOR_DNS_NAME 0x78, 0xff, DNS_NAME
#define CBOR_ADDR1 0x50, ADDR_1

#define CBOR_SOC_ADDR CBOR_ADDR1, CBOR_UINT16(1024)
