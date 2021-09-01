/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Protocol related definitions */

#ifndef PTT_PROTO_H__
#define PTT_PROTO_H__

#include <stdbool.h>
#include <stdint.h>

#include "ptt_types.h"

/* first byte of a packet preamble */
#define PTT_PREAMBLE_1ST (0xDEu)
/* second byte of a packet preamble */
#define PTT_PREAMBLE_2ND (0xDEu)
/* third byte of a packet preamble */
#define PTT_PREAMBLE_3D (0x00u)
/* length of a packet preamble */
#define PTT_PREAMBLE_LEN (3u)

/* length of command id field */
#define PTT_CMD_CODE_LEN (1u)

/* index in a packet where command id is placed */
#define PTT_CMD_CODE_START (PTT_PREAMBLE_LEN)
/* index in a packet where payload started */
#define PTT_PAYLOAD_START (PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN)

#define PTT_PAYLOAD_LEN_SET_CHANNEL 4
#define PTT_PAYLOAD_LEN_SET_POWER 1
#define PTT_PAYLOAD_LEN_STREAM 2
#define PTT_PAYLOAD_LEN_GET_POWER 1
#define PTT_PAYLOAD_LEN_REPORT 16
#define PTT_PAYLOAD_LEN_GET_HARDWARE 1
#define PTT_PAYLOAD_LEN_GET_SOFTWARE 1
#define PTT_PAYLOAD_LEN_SET_ANTENNA 1
#define PTT_PAYLOAD_LEN_GET_ANTENNA 1

/** OTA commands codes */
enum ptt_cmd {
	PTT_CMD_PING = 0x00, /**< ping command */
	PTT_CMD_ACK = 0x01, /**< ack command */
	PTT_CMD_SET_CHANNEL = 0x02, /**< set DUT radio channel command */
	PTT_CMD_SET_POWER = 0x05, /**< set DUT radio power command */
	PTT_CMD_GET_POWER = 0x06, /**< send get power request to DUT command */
	PTT_CMD_GET_POWER_RESPONSE = 0x07, /**< get power command response */
	PTT_CMD_STREAM = 0x09, /**< start modulated waveform stream */
	PTT_CMD_START_RX_TEST = 0x0A, /**< start statistics incrementing */
	PTT_CMD_END_RX_TEST = 0x0B, /**< send report request to DUT command */
	PTT_CMD_REPORT = 0x0C, /**< test statistic report */
	PTT_CMD_GET_HARDWARE_VERSION = 0x11, /**< send hardware version request to DUT */
	PTT_CMD_GET_HARDWARE_VERSION_RESPONSE = 0x12, /**< get hardware version response */
	PTT_CMD_GET_SOFTWARE_VERSION = 0x13, /**< send software version request to DUT */
	PTT_CMD_GET_SOFTWARE_VERSION_RESPONSE = 0x14, /**< get software version response */
	PTT_CMD_GET_RX_ANTENNA = 0x20, /**< send antenna request to DUT command */
	PTT_CMD_GET_TX_ANTENNA = 0x21, /**< send antenna request to DUT command */
	PTT_CMD_GET_LAST_BEST_RX_ANTENNA = 0x22, /**< send antenna request to DUT command */
	PTT_CMD_GET_ANTENNA_RESPONSE = 0x23, /**< get antenna command response */
	PTT_CMD_SET_ANTENNA = 0x24, /**< set DUT antenna command */
	PTT_CMD_SET_TX_ANTENNA = 0x25, /**< set DUT antenna command */
	PTT_CMD_SET_RX_ANTENNA = 0x26, /**< set DUT antenna command */
	PTT_CMD_CHANGE_MODE = 0xF0, /**< change device mode command */
};

/** @brief Check a packet for valid protocol header
 *
 *  @param pkt - pointer to packet data
 *  @param len - length of packet
 *
 *  @return true - packet header is valid
 *  @return false - packet header isn't valid
 */
bool ptt_proto_check_packet(const uint8_t *pkt, ptt_pkt_len_t len);

/** @brief Fill a packet with protocol header and provided command
 *
 *  @param pkt - pointer to packet data, must have enough space to reside the header and command
 *  @param cmd - command id
 *  @param pkt_max_size - available space in the packet
 *
 *  @return ptt_pkt_len_t - length of header with command
 */
ptt_pkt_len_t ptt_proto_construct_header(uint8_t *pkt, enum ptt_cmd cmd,
					 ptt_pkt_len_t pkt_max_size);

/* helper functions to convert between protocol big-endian format and host endianness */
void ptt_htobe16(uint8_t *src, uint8_t *dst);
void ptt_htobe32(uint8_t *src, uint8_t *dst);
void ptt_betoh16(uint8_t *src, uint8_t *dst);
void ptt_betoh32(uint8_t *src, uint8_t *dst);

uint16_t ptt_htobe16_val(uint8_t *src);
uint32_t ptt_htobe32_val(uint8_t *src);
uint16_t ptt_betoh16_val(uint8_t *src);
uint32_t ptt_betoh32_val(uint8_t *src);

void ptt_htole16(uint8_t *src, uint8_t *dst);

#endif /* PTT_PROTO_H__ */
