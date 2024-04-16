/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>

#ifndef AEADDEFS_HEADER_FILE
#define AEADDEFS_HEADER_FILE

struct sx_aead_cmdma_tags {
	uint32_t cfg;
	uint32_t key;
	uint32_t iv_or_state;
	uint32_t nonce;
	uint32_t aad;
	uint32_t data;
	uint32_t tag;
};

struct sx_aead_cmdma_cfg {
	uint32_t encr;
	uint32_t decr;
	uint32_t mode;
	const struct sx_aead_cmdma_tags *dmatags;
	const char *verifier;
	int (*lenAlenC)(size_t arg0, size_t arg1, uint8_t *arg2);
	uint32_t ctxsave;
	uint32_t ctxload;
	int granularity;
	int ctxsz;
};

#define OFFSET_EXTRAMEM(c) (sizeof((c)->dma.dmamem) + sizeof((c)->allindescs))

#define BA411_MODEID_OFFSET 8

/* BA411E-AES Config register -> ModeOfOperation [16:8] */
#define CMDMA_AEAD_MODE_SET(modeid) (1 << (BA411_MODEID_OFFSET + (modeid)))

#endif
