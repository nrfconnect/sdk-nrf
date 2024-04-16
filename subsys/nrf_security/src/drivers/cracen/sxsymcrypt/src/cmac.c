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
#include <cracen/prng_pool.h>

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

static int sx_cmac_create_aes_ba411(struct sxmac *c, const struct sxkeyref *key);

static int sx_cmac_create_aes_ba411(struct sxmac *c, const struct sxkeyref *key)
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

	c->key = key;
	err = sx_mac_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->cfg = &ba411cfg;
	c->cntindescs = 1;
	sx_cmdma_newcmd(&c->dma, c->allindescs,
			CMDMA_CMAC_MODE_SET(CMAC_MODEID_AES) | KEYREF_BA411E_HWKEY_CONF(key->cfg),
			c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
		c->cntindescs++;
	}
	c->dma.out = c->dma.dmamem.outdescs;
	c->feedsz = 0;
	c->macsz = CMAC_MAC_SZ;
	c->dma.out = c->dma.dmamem.outdescs;

	return SX_OK;
}

int sx_mac_create_aescmac(struct sxmac *c, const struct sxkeyref *key)
{
	return sx_cmac_create_aes_ba411(c, key);
}
