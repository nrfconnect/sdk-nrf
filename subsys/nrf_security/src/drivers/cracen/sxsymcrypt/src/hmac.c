/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/hmac.h>
#include <cracen/statuscodes.h>
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "macdefs.h"
#include "keyrefdefs.h"

#define BA413_HMAC_CONF		       (1 << 8)
/** BA413-HASH Config register -> KeySel[3:0] = [31:28] */
#define KEYREF_BA413_HWKEY_CONF(index) (((index) & 0xF) << 28)
/** BA418-HASH Config register hmac */
#define BA418_HMAC_ENABLED	       (1 << 29)
/** BA418-HASH Config register Padding*/
#define BA418_HW_PADDING	       (1 << 5)
/** BA418-HASH Config register -> KeySel[4:0] = [28:24] */
#define KEYREF_BA418_HWKEY_CONF(index) (((index) & 0x1F) << 24)

static const struct sx_mac_cmdma_tags ba413tags = {.cfg = DMATAG_BA413 | DMATAG_CONFIG(0),
						   .key = DMATAG_BA413 | DMATAG_DATATYPE(2) |
							  DMATAG_LAST,
						   .data = DMATAG_BA413 | DMATAG_DATATYPE(0)};

static const struct sx_mac_cmdma_cfg ba413cfg = {
	.cmdma_mask = CMDMA_BA413_BUS_MSK,
	.statesz = 0,
	.dmatags = &ba413tags,
};

static const struct sx_mac_cmdma_tags ba418tags = {.cfg = DMATAG_BA418 | DMATAG_CONFIG(0),
						   .key = DMATAG_BA418 | DMATAG_DATATYPE(2) |
							  DMATAG_LAST,
						   .data = DMATAG_BA418 | DMATAG_DATATYPE(0)};

static struct sx_mac_cmdma_cfg ba418cfg = {
	.cmdma_mask = CMDMA_BA418_BUS_MSK,
	.statesz = 0,
	.dmatags = &ba418tags,
};

static int sx_hash_create_hmac_ba413(struct sxmac *mac_ctx, const struct sxhashalg *algo,
				     struct sxkeyref *keyref)
{

	if (KEYREF_IS_INVALID(keyref)) {
		return SX_ERR_INVALID_KEYREF;
	}

	/* SM3 is not supported by CRACEN in nRF devices */
	if (algo->cfgword & REG_BA413_CAPS_ALGO_SM3) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	mac_ctx->key = keyref;
	sx_hw_reserve(&mac_ctx->dma);

	mac_ctx->cfg = &ba413cfg;
	sx_cmdma_newcmd(&mac_ctx->dma, mac_ctx->descs,
			algo->cfgword | BA413_HMAC_CONF | KEYREF_BA413_HWKEY_CONF(keyref->cfg),
			mac_ctx->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(keyref)) {
		if (keyref->sz) {
			ADD_INDESCA(mac_ctx->dma, keyref->key, keyref->sz,
				    mac_ctx->cfg->dmatags->key, CMDMA_BA413_BUS_MSK);
		} else {
			ADD_EMPTY_INDESC(mac_ctx->dma, HASH_INVALID_BYTES,
					 mac_ctx->cfg->dmatags->key);
		}
	}

	mac_ctx->cntindescs = 2;
	mac_ctx->feedsz = 0;
	mac_ctx->macsz = algo->digestsz;

	return SX_OK;
}

int sx_mac_create_hmac_sha2_256(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(mac_ctx, &sxhashalg_sha2_256, keyref);
}

int sx_mac_create_hmac_sha2_384(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(mac_ctx, &sxhashalg_sha2_384, keyref);
}

int sx_mac_create_hmac_sha2_512(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(mac_ctx, &sxhashalg_sha2_512, keyref);
}

int sx_mac_create_hmac_sha1(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	/* sha1 hmac and a hardware key in conjunction is not supported */
	if (!KEYREF_IS_USR(keyref)) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	return sx_hash_create_hmac_ba413(mac_ctx, &sxhashalg_sha1, keyref);
}

int sx_mac_create_hmac_sha2_224(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba413(mac_ctx, &sxhashalg_sha2_224, keyref);
}

static int sx_hash_create_hmac_ba418(struct sxmac *mac_ctx, const struct sxhashalg *algo,
				     struct sxkeyref *keyref)
{
	if (KEYREF_IS_INVALID(keyref)) {
		return SX_ERR_INVALID_KEYREF;
	}

	mac_ctx->key = keyref;
	sx_hw_reserve(&mac_ctx->dma);

	mac_ctx->cfg = &ba418cfg;

	sx_cmdma_newcmd(&mac_ctx->dma, mac_ctx->descs,
			algo->cfgword | BA418_HMAC_ENABLED | KEYREF_BA418_HWKEY_CONF(keyref->cfg) |
				BA418_HW_PADDING,
			mac_ctx->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(keyref)) {
		if (keyref->sz) {
			ADD_INDESCA(mac_ctx->dma, keyref->key, keyref->sz,
				    mac_ctx->cfg->dmatags->key, CMDMA_BA418_BUS_MSK);
		} else {
			ADD_EMPTY_INDESC(mac_ctx->dma, HASH_INVALID_BYTES,
					 mac_ctx->cfg->dmatags->key);
		}
	}

	mac_ctx->cntindescs = 2;
	mac_ctx->feedsz = 0;
	mac_ctx->macsz = algo->digestsz;

	return SX_OK;
}

int sx_mac_create_hmac_sha3_224(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba418(mac_ctx, &sxhashalg_sha3_224, keyref);
}

int sx_mac_create_hmac_sha3_256(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba418(mac_ctx, &sxhashalg_sha3_256, keyref);
}

int sx_mac_create_hmac_sha3_384(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba418(mac_ctx, &sxhashalg_sha3_384, keyref);
}

int sx_mac_create_hmac_sha3_512(struct sxmac *mac_ctx, struct sxkeyref *keyref)
{
	return sx_hash_create_hmac_ba418(mac_ctx, &sxhashalg_sha3_512, keyref);
}
