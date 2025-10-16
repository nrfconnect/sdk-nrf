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

/** AES block cipher context saving state size, in bytes */
#define AES_BLKCIPHER_STATE_SZ (16)

static const struct sx_blkcipher_cmdma_tags ba411tags = {.cfg = DMATAG_BA411 | DMATAG_CONFIG(0),
							 .iv_or_state =
								 DMATAG_BA411 | DMATAG_CONFIG(0x28),
							 .key = DMATAG_BA411 | DMATAG_CONFIG(0x08),
							 .key2 = DMATAG_BA411 | DMATAG_CONFIG(0x48),
							 .data = DMATAG_BA411};

static const struct sx_blkcipher_cmdma_cfg ba411ecbcfg = {
	.decr = CM_CFG_DECRYPT,
	.dmatags = &ba411tags,
	.statesz = 0,
	.mode = BLKCIPHER_MODEID_ECB,
	.inminsz = 16,
	.granularity = 16,
	.blocksz = BLKCIPHER_BLOCK_SZ,
};

static const struct sx_blkcipher_cmdma_cfg ba411cbccfg = {
	.decr = CM_CFG_DECRYPT,
	.ctxsave = BLKCIPHER_MODEID_CTX_SAVE,
	.ctxload = BLKCIPHER_MODEID_CTX_LOAD,
	.dmatags = &ba411tags,
	.statesz = AES_BLKCIPHER_STATE_SZ,
	.mode = BLKCIPHER_MODEID_CBC,
	.inminsz = 16,
	.granularity = 16,
	.blocksz = BLKCIPHER_BLOCK_SZ,
};

static const struct sx_blkcipher_cmdma_cfg ba411ctrcfg = {
	.decr = CM_CFG_DECRYPT,
	.ctxsave = BLKCIPHER_MODEID_CTX_SAVE,
	.ctxload = BLKCIPHER_MODEID_CTX_LOAD,
	.dmatags = &ba411tags,
	.statesz = AES_BLKCIPHER_STATE_SZ,
	.mode = BLKCIPHER_MODEID_CTR,
	.inminsz = 1,
	.granularity = 1,
	.blocksz = BLKCIPHER_BLOCK_SZ,
};

static const struct sx_blkcipher_cmdma_cfg ba411xtscfg = {
	.decr = CM_CFG_DECRYPT,
	.ctxsave = BLKCIPHER_MODEID_CTX_SAVE,
	.ctxload = BLKCIPHER_MODEID_CTX_LOAD,
	.dmatags = &ba411tags,
	.statesz = AES_BLKCIPHER_STATE_SZ,
	.mode = BLKCIPHER_MODEID_XTS,
	.inminsz = 16,
	.granularity = 1,
	.blocksz = BLKCIPHER_BLOCK_SZ,
};

int sx_blkcipher_free(struct sxblkcipher *cipher_ctx)
{
	int sx_err = SX_OK;
	if (cipher_ctx->key && cipher_ctx->key->clean_key) {
		sx_err = cipher_ctx->key->clean_key(cipher_ctx->key->user_data);
	}
	sx_cmdma_release_hw(&cipher_ctx->dma);
	return sx_err;
}

static int sx_blkcipher_hw_reserve(struct sxblkcipher *cipher_ctx)
{
	int err = SX_OK;

	uint32_t prng_value;

	err = cracen_prng_value_from_pool(&prng_value);
	if (err != SX_OK) {
		return err;
	}

	sx_hw_reserve(&cipher_ctx->dma);

	err = sx_cm_load_mask(prng_value);
	if (err != SX_OK) {
		goto exit;
	}

	if (cipher_ctx->key && cipher_ctx->key->prepare_key) {
		err = cipher_ctx->key->prepare_key(cipher_ctx->key->user_data);
	}

exit:
	if (err != SX_OK) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx), err);
	}

	return SX_OK;
}

static int sx_blkcipher_create_aesxts(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key1,
				      const struct sxkeyref *key2, const uint8_t *iv)
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

	cipher_ctx->key = key1;
	err = sx_blkcipher_hw_reserve(cipher_ctx);
	if (err != SX_OK) {
		return err;
	}

	cipher_ctx->cfg = &ba411xtscfg;
	mode = CMDMA_BLKCIPHER_MODE_SET(BLKCIPHER_MODEID_XTS);
	keyszfld = 0;

	sx_cmdma_newcmd(&cipher_ctx->dma, cipher_ctx->descs,
			KEYREF_AES_HWKEY_CONF(key1->cfg) | mode | keyszfld,
			cipher_ctx->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key1)) {
		ADD_CFGDESC(cipher_ctx->dma, key1->key, key1->sz, cipher_ctx->cfg->dmatags->key);
		ADD_CFGDESC(cipher_ctx->dma, key2->key, key2->sz, cipher_ctx->cfg->dmatags->key2);
	}
	ADD_CFGDESC(cipher_ctx->dma, iv, 16, cipher_ctx->cfg->dmatags->iv_or_state);

	return SX_OK;
}

int sx_blkcipher_create_aesxts_enc(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const uint8_t *iv)
{
	int status;

	status = sx_blkcipher_create_aesxts(cipher_ctx, key1, key2, iv);
	if (status) {
		return status;
	}

	cipher_ctx->dma.dmamem.cfg |= CM_CFG_ENCRYPT;

	return SX_OK;
}

int sx_blkcipher_create_aesxts_dec(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const uint8_t *iv)
{
	int status;

	status = sx_blkcipher_create_aesxts(cipher_ctx, key1, key2, iv);
	if (status) {
		return status;
	}

	cipher_ctx->dma.dmamem.cfg |= cipher_ctx->cfg->decr;

	return SX_OK;
}

static int sx_blkcipher_create_aes_ba411(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key,
					 const uint8_t *iv,
					 const struct sx_blkcipher_cmdma_cfg *cfg,
					 const uint32_t dir)
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

	cipher_ctx->key = key;
	cipher_ctx->cfg = cfg;
	cipher_ctx->textsz = 0;

	err = sx_blkcipher_hw_reserve(cipher_ctx);
	if (err != SX_OK) {
		return err;
	}

	sx_cmdma_newcmd(&cipher_ctx->dma, cipher_ctx->descs,
			CMDMA_BLKCIPHER_MODE_SET(cfg->mode) | KEYREF_AES_HWKEY_CONF(key->cfg) | dir,
			cipher_ctx->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(cipher_ctx->dma, key->key, key->sz, cipher_ctx->cfg->dmatags->key);
	}
	if (iv != NULL) {
		ADD_CFGDESC(cipher_ctx->dma, iv, 16, cipher_ctx->cfg->dmatags->iv_or_state);
	}

	return SX_OK;
}

int sx_blkcipher_create_aesctr_enc(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key,
				   const uint8_t *iv)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, iv, &ba411ctrcfg, CM_CFG_ENCRYPT);
}

int sx_blkcipher_create_aesctr_dec(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key,
				   const uint8_t *iv)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, iv, &ba411ctrcfg, ba411ctrcfg.decr);
}

int sx_blkcipher_create_aesecb_enc(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, NULL, &ba411ecbcfg, CM_CFG_ENCRYPT);
}

int sx_blkcipher_create_aesecb_dec(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, NULL, &ba411ecbcfg, ba411ecbcfg.decr);
}

int sx_blkcipher_create_aescbc_enc(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key,
				   const uint8_t *iv)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, iv, &ba411cbccfg, CM_CFG_ENCRYPT);
}

int sx_blkcipher_create_aescbc_dec(struct sxblkcipher *cipher_ctx, const struct sxkeyref *key,
				   const uint8_t *iv)
{
	return sx_blkcipher_create_aes_ba411(cipher_ctx, key, iv, &ba411cbccfg, ba411cbccfg.decr);
}

int sx_blkcipher_crypt(struct sxblkcipher *cipher_ctx, const uint8_t *datain, size_t sz,
		       uint8_t *dataout)
{
	if (!cipher_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (sz >= DMA_MAX_SZ) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx), SX_ERR_TOO_BIG);
	}

	cipher_ctx->textsz += sz;
	ADD_INDESCA(cipher_ctx->dma, datain, sz, cipher_ctx->cfg->dmatags->data, 0xf);
	ADD_OUTDESCA(cipher_ctx->dma, dataout, sz, 0xf);

	return SX_OK;
}

int sx_blkcipher_run(struct sxblkcipher *cipher_ctx)
{
	if (!cipher_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (cipher_ctx->textsz < cipher_ctx->cfg->inminsz) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx),
					      SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}
	if (cipher_ctx->textsz % cipher_ctx->cfg->granularity) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx),
					      SX_ERR_WRONG_SIZE_GRANULARITY);
	}

	if (cipher_ctx->dma.dmamem.cfg & cipher_ctx->cfg->ctxsave) {
		cipher_ctx->dma.dmamem.cfg &= ~cipher_ctx->cfg->ctxsave;
	}

	sx_cmdma_start(&cipher_ctx->dma, sizeof(cipher_ctx->descs) + sizeof(cipher_ctx->extramem),
		       cipher_ctx->descs);

	return SX_OK;
}

int sx_blkcipher_resume_state(struct sxblkcipher *cipher_ctx)
{
	int err;

	if (cipher_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (cipher_ctx->cfg->statesz == 0) {
		return SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED;
	}

	err = sx_blkcipher_hw_reserve(cipher_ctx);
	if (err != SX_OK) {
		return err;
	}

	cipher_ctx->dma.dmamem.cfg &= ~(cipher_ctx->cfg->ctxsave);
	sx_cmdma_newcmd(&cipher_ctx->dma, cipher_ctx->descs, cipher_ctx->dma.dmamem.cfg,
			cipher_ctx->cfg->dmatags->cfg);

	if (cipher_ctx->key && KEYREF_IS_USR(cipher_ctx->key)) {
		ADD_CFGDESC(cipher_ctx->dma, cipher_ctx->key->key, cipher_ctx->key->sz,
			    cipher_ctx->cfg->dmatags->key);
	}
	/* Context will be transferred in the same place as the IV. However,
	 * we cannot use same approach as for IV because context is stored in cipher_ctx
	 * and needs to be added using ADD_INDESC_PRIV()
	 */
	ADD_INDESC_PRIV(cipher_ctx->dma, OFFSET_EXTRAMEM(cipher_ctx), 16,
			cipher_ctx->cfg->dmatags->iv_or_state);

	cipher_ctx->dma.dmamem.cfg |= cipher_ctx->cfg->ctxload;
	cipher_ctx->textsz = 0;

	return SX_OK;
}

int sx_blkcipher_save_state(struct sxblkcipher *cipher_ctx)
{
	if (!cipher_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (cipher_ctx->cfg->statesz == 0) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx),
					      SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED);
	}

	if (cipher_ctx->textsz < cipher_ctx->cfg->blocksz) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx),
					      SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}
	if (cipher_ctx->textsz & (cipher_ctx->cfg->blocksz - 1)) {
		return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx),
					      SX_ERR_WRONG_SIZE_GRANULARITY);
	}

	cipher_ctx->dma.dmamem.cfg |= cipher_ctx->cfg->ctxsave;

	ADD_OUTDESC_PRIV(cipher_ctx->dma, OFFSET_EXTRAMEM(cipher_ctx), 16, 0x0F);

	sx_cmdma_start(&cipher_ctx->dma, sizeof(cipher_ctx->descs) + sizeof(cipher_ctx->extramem),
		       cipher_ctx->descs);

	return SX_OK;
}

int sx_blkcipher_status(struct sxblkcipher *cipher_ctx)
{
	int status;

	if (!cipher_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	status = sx_cmdma_check();
	if (status == SX_ERR_HW_PROCESSING) {
		return status;
	}

#if CONFIG_DCACHE
	sys_cache_data_invd_range((void *)&cipher_ctx->extramem, sizeof(cipher_ctx->extramem));
#endif

	return sx_handle_nested_error(sx_blkcipher_free(cipher_ctx), status);
}

int sx_blkcipher_wait(struct sxblkcipher *cipher_ctx)
{
	int status = SX_ERR_HW_PROCESSING;

	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_blkcipher_status(cipher_ctx);
	}

	return status;
}

int sx_blkcipher_ecb_simple(uint8_t *key, size_t key_size, uint8_t *input, size_t input_size,
			    uint8_t *output, size_t output_size)
{
	int status = SX_ERR_HW_PROCESSING;

	uint32_t cmd = CMDMA_BLKCIPHER_MODE_SET(BLKCIPHER_MODEID_ECB);
	/* Both out_desc and in_descs are used after sx_hw_reserve which locks
	 * the symmetric mutex, so it is safe to have them as static.
	 */
	static struct sxdesc out_desc;
	static struct sxdesc in_descs[3];

	/* This guards the static variables out_desc and in_descs */
	sx_hw_reserve(NULL);

	in_descs[0].addr = (uint8_t *)&cmd;
	in_descs[0].sz = DMA_REALIGN | sizeof(cmd);
	in_descs[0].dmatag = ba411tags.cfg;
	in_descs[0].next = &in_descs[1];

	in_descs[1].addr = key;
	in_descs[1].sz = DMA_REALIGN | key_size;
	in_descs[1].dmatag = ba411tags.key;
	in_descs[1].next = &in_descs[2];

	in_descs[2].addr = input;
	in_descs[2].sz = DMA_REALIGN | input_size;
	in_descs[2].dmatag = DMATAG_LAST | ba411tags.data;
	in_descs[2].next = (void *)1;

	out_desc.addr = output;
	out_desc.sz = DMA_REALIGN | output_size;
	out_desc.next = (void *)1;
	out_desc.dmatag = DMATAG_LAST;

#if CONFIG_DCACHE
	sys_cache_data_flush_range(in_descs, sizeof(in_descs));
	sys_cache_data_flush_range(&out_desc, sizeof(out_desc));
	sys_cache_data_flush_range(input, input_size);
	sys_cache_data_flush_range(output, output_size);
#endif

	sx_wrreg_addr(REG_FETCH_ADDR, in_descs);
	sx_wrreg_addr(REG_PUSH_ADDR, &out_desc);
	sx_wrreg(REG_CONFIG, REG_CONFIG_SG);
	sx_wrreg(REG_START, REG_START_ALL);

	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_cmdma_check();
	}

	sx_cmdma_release_hw(NULL);

#if CONFIG_DCACHE
	sys_cache_data_invd_range(output, output_size);
#endif

	return status;
}
