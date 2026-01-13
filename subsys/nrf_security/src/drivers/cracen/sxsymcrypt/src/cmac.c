/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/cmac.h"
#include "../include/sxsymcrypt/keyref.h"
#include "../include/sxsymcrypt/cmmask.h"
#include "../include/sxsymcrypt/mac.h"
#include <cracen/statuscodes.h>
#include "keyrefdefs.h"
#include "macdefs.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "cmaes.h"

#define CMDMA_BA411_BUS_MSK (0x0F)
#define CMAC_MODEID_AES	    8
#define CMAC_BLOCK_SZ	    16
#define CMAC_STATE_SZ	    16
#define CMAC_MAC_SZ	    16

/** Mode Register value for context loading */
#define CMAC_MODEID_CTX_LOAD (1u << 4)
/** Mode Register value for context saving */
#define CMAC_MODEID_CTX_SAVE (1u << 5)

static const struct sx_mac_cmdma_tags ba411tags = {.cfg = DMATAG_BA411 | DMATAG_CONFIG(0),
						   .state = DMATAG_BA411 | DMATAG_CONFIG(0x28),
						   .key = DMATAG_BA411 | DMATAG_CONFIG(0x08),
						   .data = DMATAG_BA411};

static const struct sx_mac_cmdma_cfg ba411cfg = {
	.cmdma_mask = CMDMA_BA411_BUS_MSK,
	.granularity = CMAC_BLOCK_SZ,
	.blocksz = CMAC_BLOCK_SZ,
	.savestate = CMAC_MODEID_CTX_SAVE,
	.statesz = CMAC_STATE_SZ,
	.loadstate = CMAC_MODEID_CTX_LOAD,
	.dmatags = &ba411tags,
};

static int sx_cmac_create_aes_ba411(struct sxmac *mac_ctx, const struct sxkeyref *key);

static int sx_cmac_create_aes_ba411(struct sxmac *mac_ctx, const struct sxkeyref *key)
{
	int err;

	if (KEYREF_IS_INVALID(key)) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key)) {
		if (sx_aes_keysz((key)->sz) == ~0u) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	mac_ctx->key = key;

	/* Key preparation - CM setup is handled by CRACENPSA layer via sx_hw_reserve() */
	if (key->prepare_key) {
		err = key->prepare_key(key->user_data);
		if (err != SX_OK) {
			return err;
		}
	}

	mac_ctx->cfg = &ba411cfg;
	mac_ctx->cntindescs = 1;
	sx_cmdma_newcmd(&mac_ctx->dma, mac_ctx->descs,
			CMDMA_CMAC_MODE_SET(CMAC_MODEID_AES) | KEYREF_AES_HWKEY_CONF(key->cfg),
			mac_ctx->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(mac_ctx->dma, key->key, key->sz, mac_ctx->cfg->dmatags->key);
		mac_ctx->cntindescs++;
	}
	mac_ctx->feedsz = 0;
	mac_ctx->macsz = CMAC_MAC_SZ;

	return SX_OK;
}

int sx_mac_create_aescmac(struct sxmac *mac_ctx, const struct sxkeyref *key)
{
	return sx_cmac_create_aes_ba411(mac_ctx, key);
}
