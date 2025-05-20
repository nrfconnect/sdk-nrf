/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Protocol processing */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ctrl/ptt_trace.h"

#include "ptt_proto.h"

#include "ptt_types.h"

bool ptt_proto_check_packet(const uint8_t *pkt, ptt_pkt_len_t len)
{
	bool ret = false;

	if ((pkt != NULL) && (len > PTT_PREAMBLE_LEN)) {
		if ((pkt[0] == PTT_PREAMBLE_1ST) && (pkt[1] == PTT_PREAMBLE_2ND) &&
		    (pkt[2] == PTT_PREAMBLE_3D)) {
			ret = true;
		}
	}

	return ret;
}

ptt_pkt_len_t ptt_proto_construct_header(uint8_t *pkt, enum ptt_cmd cmd, ptt_pkt_len_t pkt_max_size)
{
	ptt_pkt_len_t len = 0;

	assert(pkt_max_size >= (PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN));

	pkt[len] = PTT_PREAMBLE_1ST;
	len++;
	pkt[len] = PTT_PREAMBLE_2ND;
	len++;
	pkt[len] = PTT_PREAMBLE_3D;
	len++;
	pkt[len] = cmd;
	len++;

	assert((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN) == len);

	return len;
}

void ptt_htobe16(uint8_t *src, uint8_t *dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	dst[0] = src[1];
	dst[1] = src[0];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	dst[0] = src[0];
	dst[1] = src[1];
#else
#error "Unsupported endianness"
#endif
}

void ptt_htole16(uint8_t *src, uint8_t *dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	dst[0] = src[0];
	dst[1] = src[1];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	dst[0] = src[1];
	dst[1] = src[0];
#else
#error "Unsupported endianness"
#endif
}

void ptt_htobe32(uint8_t *src, uint8_t *dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
#else
#error "Unsupported endianness"
#endif
}

void ptt_betoh16(uint8_t *src, uint8_t *dst)
{
	ptt_htobe16(src, dst);
}

void ptt_betoh32(uint8_t *src, uint8_t *dst)
{
	ptt_htobe32(src, dst);
}

uint16_t ptt_htobe16_val(uint8_t *src)
{
	uint16_t val = 0;

	ptt_htobe16(src, (uint8_t *)(&val));

	return val;
}

uint32_t ptt_htobe32_val(uint8_t *src)
{
	uint32_t val = 0;

	ptt_htobe32(src, (uint8_t *)(&val));

	return val;
}

uint16_t ptt_betoh16_val(uint8_t *src)
{
	uint16_t val = 0;

	ptt_betoh16(src, (uint8_t *)(&val));

	return val;
}

uint32_t ptt_betoh32_val(uint8_t *src)
{
	uint32_t val = 0;

	ptt_betoh32(src, (uint8_t *)(&val));

	return val;
}
