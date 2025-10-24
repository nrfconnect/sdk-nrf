/** Block cipher common defines and structures used by AES, SM4, TDES.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BLKCIPHERDEFS_HEADER_FILE
#define BLKCIPHERDEFS_HEADER_FILE

#include <stddef.h>

#define BLKCIPHER_MODEID_ECB	 0
#define BLKCIPHER_MODEID_CBC	 1
#define BLKCIPHER_MODEID_CTR	 2
#define BLKCIPHER_MODEID_XTS	 7
#define BLKCIPHER_MODEID_CHACH20 8

#define BLKCIPHER_BLOCK_SZ (16)

/* BA411E-AES and BA419-SM4 Config register -> ModeOfOperation [16:8] */
#define CMDMA_BLKCIPHER_MODE_SET(modeid) (1 << (8 + (modeid)))

struct sx_blkcipher_cmdma_tags {
	uint32_t cfg;
	uint32_t key;
	uint32_t key2;
	uint32_t iv_or_state;
	uint32_t data;
};

struct sx_blkcipher_cmdma_cfg {
	const struct sx_blkcipher_cmdma_tags *dmatags;
	uint8_t decr;
	uint8_t ctxsave;
	uint8_t ctxload;
	uint8_t mode;
	uint8_t inminsz;
	uint8_t granularity;
	uint8_t blocksz;
	uint8_t statesz;

};

#define OFFSET_EXTRAMEM(c) (sizeof((c)->dma.dmamem) + sizeof((c)->descs))

#endif
