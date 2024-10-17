/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/blkcipher.h"
#include "../include/sxsymcrypt/keyref.h"
#include "../include/sxsymcrypt/cmmask.h"
#include <cracen/statuscodes.h>
#include "blkcipherdefs.h"
#include "keyrefdefs.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "cmaes.h"
#include <stdint.h>
#include <cracen/prng_pool.h>

/** Mode Register value for context loading */
#define BLKCIPHER_MODEID_CTX_LOAD (1u << 4)
/** Mode Register value for context saving */
#define BLKCIPHER_MODEID_CTX_SAVE (1u << 5)

static const struct sx_blkcipher_cmdma_tags ba411tags = {.cfg = DMATAG_BA411 | DMATAG_CONFIG(0),
							 .iv_or_state =
								 DMATAG_BA411 | DMATAG_CONFIG(0x28),
							 .key = DMATAG_BA411 | DMATAG_CONFIG(0x08),
							 .key2 = DMATAG_BA411 | DMATAG_CONFIG(0x48),
							 .data = DMATAG_BA411};

static const struct sx_blkcipher_cmdma_cfg ba411cfg = {
	.encr = 0,
	.decr = CM_CFG_DECRYPT,
	.dmatags = &ba411tags,
};

void sx_blkcipher_free(struct sxblkcipher *c)
{
	if (c->key.clean_key) {
		c->key.clean_key(c->key.user_data);
	}
	sx_cmdma_release_hw(&c->dma);
}

static uint32_t get_blkcipher_ctx_load(uint32_t mode)
{
	if (mode == BLKCIPHER_MODEID_CHACH20) {
		return BA417_MODEID_CTX_LOAD;
	} else {
		return BLKCIPHER_MODEID_CTX_LOAD;
	}
}

static uint32_t get_blkcipher_ctx_save(uint32_t mode)
{
	if (mode == BLKCIPHER_MODEID_CHACH20) {
		return BA417_MODEID_CTX_SAVE;
	} else {
		return BLKCIPHER_MODEID_CTX_SAVE;
	}
}

static int sx_blkcipher_hw_reserve(struct sxblkcipher *c)
{
	int err = SX_OK;

	uint32_t prng_value;

	err = cracen_prng_value_from_pool(&prng_value);
	if (err != SX_OK) {
		return err;
	}

	sx_hw_reserve(&c->dma);

	err = sx_cm_load_mask(prng_value);
	if (err != SX_OK) {
		goto exit;
	}

	if (c->key.prepare_key) {
		err = c->key.prepare_key(c->key.user_data);
	}

exit:
	if (err != SX_OK) {
		sx_blkcipher_free(c);
	}

	return SX_OK;
}

static int sx_blkcipher_create_aesxts(struct sxblkcipher *c, const struct sxkeyref *key1,
				      const struct sxkeyref *key2, const char *iv)
{
	uint32_t keyszfld = 0;
	uint32_t mode = 0;
	int err;

	if (KEYREF_IS_INVALID(key1) || KEYREF_IS_INVALID(key2)) {
		return SX_ERR_INVALID_KEYREF;
	}
	/* BA411 XTS does support HW keys */
	if ((KEYREF_IS_USR(key1) != KEYREF_IS_USR(key2)) ||
	    (!KEYREF_IS_USR(key1) && (key2->cfg != (key1->cfg + 1)))) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key1)) {
		keyszfld = sx_aes_keysz(key1->sz);
		if ((keyszfld == ~0u) || (key1->sz != key2->sz)) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	memcpy(&c->key, key1, sizeof(c->key));
	err = sx_blkcipher_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->cfg = &ba411cfg;
	mode = CMDMA_BLKCIPHER_MODE_SET(BLKCIPHER_MODEID_XTS);
	keyszfld = 0;

	sx_cmdma_newcmd(&c->dma, c->allindescs,
			KEYREF_BA411E_HWKEY_CONF(key1->cfg) | mode | keyszfld,
			c->cfg->dmatags->cfg);
	c->inminsz = 16;
	c->granularity = 1;
	if (KEYREF_IS_USR(key1)) {
		ADD_CFGDESC(c->dma, key1->key, key1->sz, c->cfg->dmatags->key);
		ADD_CFGDESC(c->dma, key2->key, key2->sz, c->cfg->dmatags->key2);
	}
	ADD_CFGDESC(c->dma, iv, 16, c->cfg->dmatags->iv_or_state);

	c->mode = BLKCIPHER_MODEID_XTS;

	return SX_OK;
}

int sx_blkcipher_create_aesxts_enc(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv)
{
	int r;

	r = sx_blkcipher_create_aesxts(c, key1, key2, iv);
	if (r) {
		return r;
	}

	c->dma.dmamem.cfg |= c->cfg->encr;

	return SX_OK;
}

int sx_blkcipher_create_aesxts_dec(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv)
{
	int r;

	r = sx_blkcipher_create_aesxts(c, key1, key2, iv);
	if (r) {
		return r;
	}

	c->dma.dmamem.cfg |= c->cfg->decr;

	return SX_OK;
}

static int sx_blkcipher_create_aes_ba411(struct sxblkcipher *c, const struct sxkeyref *key,
					 const char *iv, const uint32_t mode, const uint32_t dir)
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

	memcpy(&c->key, key, sizeof(c->key));
	c->cfg = &ba411cfg;
	c->mode = mode;

	err = sx_blkcipher_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	sx_cmdma_newcmd(&c->dma, c->allindescs,
			CMDMA_BLKCIPHER_MODE_SET(mode) | KEYREF_BA411E_HWKEY_CONF(key->cfg) | dir,
			c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
	}
	if (iv != NULL) {
		ADD_CFGDESC(c->dma, iv, 16, c->cfg->dmatags->iv_or_state);
	}

	return SX_OK;
}

int sx_blkcipher_create_aesctr_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 1;
	c->granularity = 1;
	return sx_blkcipher_create_aes_ba411(c, key, iv, BLKCIPHER_MODEID_CTR, ba411cfg.encr);
}

int sx_blkcipher_create_aesctr_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 1;
	c->granularity = 1;
	return sx_blkcipher_create_aes_ba411(c, key, iv, BLKCIPHER_MODEID_CTR, ba411cfg.decr);
}

int sx_blkcipher_create_aesecb_enc(struct sxblkcipher *c, const struct sxkeyref *key
				   )
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_aes_ba411(c, key, NULL, BLKCIPHER_MODEID_ECB, ba411cfg.encr);
}

int sx_blkcipher_create_aesecb_dec(struct sxblkcipher *c, const struct sxkeyref *key)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_aes_ba411(c, key, NULL, BLKCIPHER_MODEID_ECB, ba411cfg.decr);
}

int sx_blkcipher_create_aescbc_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_aes_ba411(c, key, iv, BLKCIPHER_MODEID_CBC, ba411cfg.encr);
}

int sx_blkcipher_create_aescbc_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_aes_ba411(c, key, iv, BLKCIPHER_MODEID_CBC, ba411cfg.decr);
}

int sx_blkcipher_crypt(struct sxblkcipher *c, const char *datain, size_t sz, char *dataout)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (sz >= DMA_MAX_SZ) {
		sx_blkcipher_free(c);
		return SX_ERR_TOO_BIG;
	}

	c->dma.out = c->dma.dmamem.outdescs;

	ADD_INDESC(c->dma, datain, sz, c->cfg->dmatags->data);
	ADD_OUTDESC(c->dma.out, dataout, sz);

	return SX_OK;
}

int sx_blkcipher_run(struct sxblkcipher *c)
{
	uint32_t sz;

	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	/* at this moment we have only one descriptor that holds the message to
	 * be processed, therefore, it is the last one
	 */
	sz = INDESC_SZ(c->dma.d - 1);

	if (sz < c->inminsz) {
		sx_blkcipher_free(c);
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}
	if (sz % c->granularity) {
		sx_blkcipher_free(c);
		return SX_ERR_WRONG_SIZE_GRANULARITY;
	}

	if (c->dma.dmamem.cfg & get_blkcipher_ctx_save(c->mode)) {
		c->dma.dmamem.cfg &= ~(get_blkcipher_ctx_save(c->mode));
	}

	sx_cmdma_finalize_descs(c->allindescs, c->dma.d - 1);
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, c->dma.out - 1);

	sx_cmdma_start(&c->dma, sizeof(c->allindescs) + sizeof(c->extramem), c->allindescs);

	return SX_OK;
}

int sx_blkcipher_resume_state(struct sxblkcipher *c)
{
	int err;

	if (c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (c->mode == BLKCIPHER_MODEID_ECB) {
		return SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED;
	}

	err = sx_blkcipher_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->dma.dmamem.cfg &= ~(get_blkcipher_ctx_save(c->mode));
	sx_cmdma_newcmd(&c->dma, c->allindescs, c->dma.dmamem.cfg, c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(&c->key)) {
		ADD_CFGDESC(c->dma, c->key.key, c->key.sz, c->cfg->dmatags->key);
	}
	/* Context will be transferred in the same place as the IV. However,
	 * we cannot use same approach as for IV because context is stored in c
	 * and needs to be added using ADD_INDESC_PRIV()
	 */
	ADD_INDESC_PRIV(c->dma, OFFSET_EXTRAMEM(c), 16, c->cfg->dmatags->iv_or_state);

	c->dma.dmamem.cfg |= get_blkcipher_ctx_load(c->mode);

	return SX_OK;
}

int sx_blkcipher_save_state(struct sxblkcipher *c)
{
	uint32_t sz;

	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (c->mode == BLKCIPHER_MODEID_ECB) {
		sx_blkcipher_free(c);
		return SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED;
	}

	/* at this moment we have only one descriptor that holds the message to
	 * be processed, therefore, it is the last one
	 */
	sz = INDESC_SZ(c->dma.d - 1);

	if (sz < BLKCIPHER_BLOCK_SZ) {
		sx_blkcipher_free(c);
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}
	if (sz % BLKCIPHER_BLOCK_SZ) {
		sx_blkcipher_free(c);
		return SX_ERR_WRONG_SIZE_GRANULARITY;
	}

	c->dma.dmamem.cfg |= get_blkcipher_ctx_save(c->mode);

	ADD_OUTDESC_PRIV(c->dma, c->dma.out, OFFSET_EXTRAMEM(c), 16, 0x0F);

	sx_cmdma_finalize_descs(c->allindescs, c->dma.d - 1);
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, c->dma.out - 1);

	sx_cmdma_start(&c->dma, sizeof(c->allindescs) + sizeof(c->extramem), c->allindescs);

	return SX_OK;
}

int sx_blkcipher_status(struct sxblkcipher *c)
{
	int r;

	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	r = sx_cmdma_check();
	if (r == SX_ERR_HW_PROCESSING) {
		return r;
	}

#if CONFIG_DCACHE
	sys_cache_data_invd_range((void *)&c->extramem, sizeof(c->extramem));
#endif

	sx_blkcipher_free(c);

	return r;
}

int sx_blkcipher_wait(struct sxblkcipher *c)
{
	int r = SX_ERR_HW_PROCESSING;

	while (r == SX_ERR_HW_PROCESSING) {
		r = sx_blkcipher_status(c);
	}

	return r;
}

int sx_blkcipher_ecb_simple(uint8_t *key, size_t key_size, uint8_t *input, size_t input_size,
			    uint8_t *output, size_t output_size)
{
	int r = SX_ERR_HW_PROCESSING;

	uint32_t cmd = CMDMA_BLKCIPHER_MODE_SET(BLKCIPHER_MODEID_ECB);
	struct sxdesc in_descs[3] = {};

	in_descs[0].addr = (char *)&cmd;
	in_descs[0].sz = DMA_REALIGN | sizeof(cmd);
	in_descs[0].dmatag = ba411tags.cfg;
	in_descs[0].next = &in_descs[1];

	in_descs[1].addr = (char *)key;
	in_descs[1].sz = DMA_REALIGN | key_size;
	in_descs[1].dmatag = ba411tags.key;
	in_descs[1].next = &in_descs[2];

	in_descs[2].addr = (char *)input;
	in_descs[2].sz = DMA_REALIGN | input_size;
	in_descs[2].dmatag = DMATAG_LAST | ba411tags.data;
	in_descs[2].next = (void *)1;

	struct sxdesc out_desc = {};

	out_desc.addr = output;
	out_desc.sz = DMA_REALIGN | output_size;
	out_desc.next = (void *)1;
	out_desc.dmatag = DMATAG_LAST;

	sx_hw_reserve(NULL);

#if CONFIG_DCACHE
	sys_cache_data_flush_range(in_descs, sizeof(in_descs));
	sys_cache_data_flush_range(&out_desc, sizeof(out_desc));
	sys_cache_data_flush_range(input, sizeof(input));
	sys_cache_data_flush_range(output, output_size);
#endif

	sx_wrreg_addr(REG_FETCH_ADDR, in_descs);
	sx_wrreg_addr(REG_PUSH_ADDR, &out_desc);
	sx_wrreg(REG_CONFIG, REG_CONFIG_SG);
	sx_wrreg(REG_START, REG_START_ALL);

	while (r == SX_ERR_HW_PROCESSING) {
		r = sx_cmdma_check();
	}

	sx_cmdma_release_hw(NULL);

#if CONFIG_DCACHE
	sys_cache_data_invd_range(output, output_size);
#endif

	return r;
}
