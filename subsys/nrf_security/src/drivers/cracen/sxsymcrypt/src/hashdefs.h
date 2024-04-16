/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HASHDEFS_HEADER_FILE
#define HASHDEFS_HEADER_FILE

#include <stddef.h>

struct sx_digesttags {
	uint32_t cfg;
	uint32_t initialstate;
	uint32_t keycfg;
	uint32_t data;
};

struct sxhashalg {
	uint32_t cfgword;
	uint32_t resumecfg;
	uint32_t exportcfg;
	size_t digestsz;
	size_t blocksz;
	size_t statesz;
	size_t maxpadsz;
	int (*reservehw)(struct sxhash *c, size_t csz);
};

extern const struct sxhashalg sxhashalg_sha1;
extern const struct sxhashalg sxhashalg_sha2_224;
extern const struct sxhashalg sxhashalg_sha2_256;
extern const struct sxhashalg sxhashalg_sha2_384;
extern const struct sxhashalg sxhashalg_sha2_512;
extern const struct sxhashalg sxhashalg_sm3;

#define CMDMA_BA413_BUS_MSK 3
#define HASH_INVALID_BYTES  4 /* number of invalid bytes when empty message, required by HW */
#define OFFSET_EXTRAMEM(c)  (sizeof((c)->dma.dmamem) + sizeof((c)->allindescs))

#endif
