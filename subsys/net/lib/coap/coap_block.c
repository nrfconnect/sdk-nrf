/*
 * Copyright (c) 2015 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_block);

#include <stdbool.h>
#include <errno.h>

#include <net/coap_block.h>

#include "coap.h"

/** Block size base exponent. 4 means a base block size of 2^4 = 16 bytes. */
#define BLOCK_SIZE_BASE_CONSTANT 4

#define BLOCK_SIZE_16            0 /**< Block size of 2^(4+0) = 16 bytes. */
#define BLOCK_SIZE_32            1 /**< Block size of 2^(4+1) = 32 bytes. */
#define BLOCK_SIZE_64            2 /**< Block size of 2^(4+2) = 64 bytes. */
#define BLOCK_SIZE_128           3 /**< Block size of 2^(4+3) = 128 bytes. */
#define BLOCK_SIZE_256           4 /**< Block size of 2^(4+4) = 256 bytes. */
#define BLOCK_SIZE_512           5 /**< Block size of 2^(4+5) = 512 bytes. */
#define BLOCK_SIZE_1024          6 /**< Block size of 2^(4+6) = 1024 bytes. */
#define BLOCK_SIZE_2048_RESERVED 7 /**< Reserved. */

#define BLOCK_MORE_BIT_UNSET 0 /**< Value when more flag is set. */
#define BLOCK_MORE_BIT_SET   1 /**< Value when more flag is not set. */

#define BLOCK_SIZE_POS     0 /**< Bit offset to the size. */
#define BLOCK_MORE_BIT_POS 3 /**< Bit offset to the more bit. */
#define BLOCK_NUMBER_POS   4 /**< Bit offset to the block number. */

#define BLOCK_SIZE_MASK     0x7                       /**< Block size mask. */
#define BLOCK_MORE_BIT_MASK (1 << BLOCK_MORE_BIT_POS) /**< More bit mask. */
#define BLOCK_NUMBER_MASK   (0xFFFFF << 4)            /**< Block number mask. */

#define BLOCK_SIZE_MAX     0x7                /**< Maximum block size number. */
#define BLOCK_MORE_BIT_MAX BLOCK_MORE_BIT_SET /**< Maximum more bit value. */

/** Maximum block number. 20 bits max value is (1 << 20) - 1. */
#define BLOCK_NUMBER_MAX   0xFFFFF

static u32_t block_opt_encode(u8_t more, u16_t size, u32_t number,
			      u32_t *encoded)
{
	if ((number > BLOCK_NUMBER_MAX) || (more > BLOCK_MORE_BIT_MAX)) {
		return EINVAL;
	}

	u32_t val = 0;

	switch (size) {
	case 16:
		val = BLOCK_SIZE_16;
		break;

	case 32:
		val = BLOCK_SIZE_32;
		break;

	case 64:
		val = BLOCK_SIZE_64;
		break;

	case 128:
		val = BLOCK_SIZE_128;
		break;

	case 256:
		val = BLOCK_SIZE_256;
		break;

	case 512:
		val = BLOCK_SIZE_512;
		break;

	case 1024:
		val = BLOCK_SIZE_1024;
		break;

	case 2048:
	/* Falltrough. */

	default:
		/* Break omitted. */
		return EINVAL;
	}

	/* Value has been initialized. */
	val |= (more) << BLOCK_MORE_BIT_POS;
	val |= (number) << BLOCK_NUMBER_POS;

	*encoded = val;
	return 0;
}

static u32_t block_opt_decode(u32_t encoded, u8_t  *more, u16_t *size,
			      u32_t *number)
{
	if ((encoded & BLOCK_SIZE_MASK) == BLOCK_SIZE_2048_RESERVED) {
		return EINVAL;
	}

	if ((encoded >> BLOCK_NUMBER_POS) > BLOCK_NUMBER_MAX) {
		return EINVAL;
	}

	*size = (1 << ((BLOCK_SIZE_BASE_CONSTANT +
			(encoded & BLOCK_SIZE_MASK))));
	*more = (encoded & BLOCK_MORE_BIT_MASK) >> BLOCK_MORE_BIT_POS;
	*number = (encoded & BLOCK_NUMBER_MASK) >> BLOCK_NUMBER_POS;

	return 0;
}

u32_t coap_block_opt_block1_encode(u32_t *encoded, coap_block_opt_block1_t *opt)
{
	NULL_PARAM_CHECK(encoded);
	NULL_PARAM_CHECK(opt);

	return block_opt_encode(opt->more, opt->size, opt->number, encoded);
}

u32_t coap_block_opt_block1_decode(coap_block_opt_block1_t *opt, u32_t encoded)
{
	NULL_PARAM_CHECK(opt);

	return block_opt_decode(encoded, &opt->more, &opt->size, &opt->number);
}

u32_t coap_block_opt_block2_encode(u32_t *encoded, coap_block_opt_block2_t *opt)
{
	NULL_PARAM_CHECK(encoded);
	NULL_PARAM_CHECK(opt);

	return block_opt_encode(opt->more, opt->size, opt->number, encoded);
}

u32_t coap_block_opt_block2_decode(coap_block_opt_block2_t *opt, u32_t encoded)
{
	NULL_PARAM_CHECK(opt);

	return block_opt_decode(encoded, &opt->more, &opt->size, &opt->number);
}
