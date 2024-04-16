/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "../include/sxsymcrypt/hash.h"
#include "../include/sxsymcrypt/hmac.h"
#include <cracen/statuscodes.h>
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "hashdefs.h"
#include "macdefs.h"
#include "keyrefdefs.h"

#define BA413_HMAC_CONF		       (1 << 8)
/** BA413-HASH Config register -> KeySel[3:0] = [31:28] */
#define KEYREF_BA413_HWKEY_CONF(index) (((index)&0xF) << 28)

static const struct sx_mac_cmdma_tags ba413tags = {.cfg = DMATAG_BA413 | DMATAG_CONFIG(0),
						   .key = DMATAG_BA413 | DMATAG_DATATYPE(2) |
							  DMATAG_LAST,
						   .data = DMATAG_BA413 | DMATAG_DATATYPE(0)};

static const struct sx_mac_cmdma_cfg ba413cfg = {
	.cmdma_mask = CMDMA_BA413_BUS_MSK,
	.statesz = 0,
	.dmatags = &ba413tags,
};

static int sx_hash_create_hmac_ba413(struct sxmac *c, const struct sxhashalg *algo,
				     struct sxkeyref *keyref)
{

	if (KEYREF_IS_INVALID(keyref)) {
		return SX_ERR_INVALID_KEYREF;
	}

	/* SM3 is not supported by CRACEN in nRF devices */
	if (algo->cfgword & REG_BA413_CAPS_ALGO_SM3) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	sx_hw_reserve(&c->dma);

	c->cfg = &ba413cfg;
	sx_cmdma_newcmd(&c->dma, c->allindescs,
			algo->cfgword | BA413_HMAC_CONF | KEYREF_BA413_HWKEY_CONF(keyref->cfg),
			c->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(keyref)) {
		if (keyref->sz) {
			ADD_INDESCA(c->dma, keyref->key, keyref->sz, c->cfg->dmatags->key,
				    CMDMA_BA413_BUS_MSK);
		} else {
			ADD_EMPTY_INDESC(c->dma, HASH_INVALID_BYTES, c->cfg->dmatags->key);
		}
	}

	c->dma.out = c->dma.dmamem.outdescs;
	c->cntindescs = 2;
	c->feedsz = 0;
	c->macsz = algo->digestsz;

	return SX_OK;
}

int sx_mac_create_hmac_sha2_256(struct sxmac *c, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(c, &sxhashalg_sha2_256, keyref);
}

int sx_mac_create_hmac_sha2_384(struct sxmac *c, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(c, &sxhashalg_sha2_384, keyref);
}

int sx_mac_create_hmac_sha2_512(struct sxmac *c, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(c, &sxhashalg_sha2_512, keyref);
}

int sx_mac_create_hmac_sha1(struct sxmac *c, struct sxkeyref *keyref)
{
	/* sha1 hmac and a hardware key in conjunction is not supported */
	if (!KEYREF_IS_USR(keyref)) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	return sx_hash_create_hmac_ba413(c, &sxhashalg_sha1, keyref);
}

int sx_mac_create_hmac_sha2_224(struct sxmac *c, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(c, &sxhashalg_sha2_224, keyref);
}
