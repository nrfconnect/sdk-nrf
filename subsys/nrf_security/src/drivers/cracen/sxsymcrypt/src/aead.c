/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/aead.h"
#include "../include/sxsymcrypt/aes.h"
#include "../include/sxsymcrypt/keyref.h"
#include "../include/sxsymcrypt/cmmask.h"
#include <cracen/statuscodes.h>
#include "../include/sxsymcrypt/memdiff.h"
#include "keyrefdefs.h"
#include "aeaddefs.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "cmaes.h"
#include <cracen/prng_pool.h>

/** Mode Register value for context loading */
#define AES_AEAD_MODEID_CTX_LOAD (1u << 4)
/** Mode Register value for context saving */
#define AES_AEAD_MODEID_CTX_SAVE (1u << 5)

/** Size of AEAD block size, in bytes */
#define AEAD_BLOCK_SZ	      (16)
/** Size of AEAD GCM and CCM context saving state, in bytes */
#define AES_AEAD_CTX_STATE_SZ (32)
/** Size of AEAD lenAlenC, in bytes */
#define AEAD_LENA_LENC_SZ     (16)

static int lenAlenC_aesgcm_ba411(size_t aadsz, size_t datasz, uint8_t *out);
static int lenAlenC_nop(size_t aadsz, size_t datasz, uint8_t *out);

static const uint8_t zeros[SX_CCM_MAX_TAG_SZ] = {0};

static const struct sx_aead_cmdma_tags ba411aeadtags = {
	.cfg = DMATAG_BA411 | DMATAG_CONFIG(0),
	.iv_or_state = DMATAG_BA411 | DMATAG_CONFIG(0x28),
	.key = DMATAG_BA411 | DMATAG_CONFIG(0x08),
	.aad = DMATAG_BA411 | DMATAG_DATATYPE_HEADER,
	.tag = DMATAG_BA411,
	.data = DMATAG_BA411};

#define BA411_MODEID_GCM 6
static const struct sx_aead_cmdma_cfg ba411gcmcfg = {.decr = CM_CFG_DECRYPT,
						     .mode = BA411_MODEID_GCM,
						     .dmatags = &ba411aeadtags,
						     .verifier = NULL,
						     .lenAlenC = lenAlenC_aesgcm_ba411,
						     .ctxsave = AES_AEAD_MODEID_CTX_SAVE,
						     .ctxload = AES_AEAD_MODEID_CTX_LOAD,
						     .granularity = AEAD_BLOCK_SZ,
						     .statesz = AES_AEAD_CTX_STATE_SZ,
						     .inputminsz = 0};

#define BA411_MODEID_CCM 5
static const struct sx_aead_cmdma_cfg ba411ccmcfg = {.decr = CM_CFG_DECRYPT,
						     .mode = BA411_MODEID_CCM,
						     .dmatags = &ba411aeadtags,
						     .verifier = zeros,
						     .lenAlenC = lenAlenC_nop,
						     .ctxsave = AES_AEAD_MODEID_CTX_SAVE,
						     .ctxload = AES_AEAD_MODEID_CTX_LOAD,
						     .granularity = AEAD_BLOCK_SZ,
						     .statesz = AES_AEAD_CTX_STATE_SZ,
						     .inputminsz = 0};

static int lenAlenC_nop(size_t aadsz, size_t datasz, uint8_t *out)
{
	(void)aadsz;
	(void)datasz;
	(void)out;

	return 0;
}

static int lenAlenC_aesgcm_ba411(size_t aadsz, size_t datasz, uint8_t *out)
{
	uint32_t i = 0;

	aadsz = aadsz << 3;
	datasz = datasz << 3;
	for (i = 0; i < 8; i++) {
		out[7 - i] = aadsz & 0xFF;
		aadsz >>= 8;
	}
	out += 8;
	for (i = 0; i < 8; i++) {
		out[7 - i] = datasz & 0xFF;
		datasz >>= 8;
	}

	return 1;
}

int sx_aead_free(struct sxaead *aead_ctx)
{
	int sx_err = SX_OK;

	if (aead_ctx->key->clean_key) {
		sx_err = aead_ctx->key->clean_key(aead_ctx->key->user_data);
	}
	sx_cmdma_release_hw(&aead_ctx->dma);
	return sx_err;
}

int sx_aead_hw_reserve(struct sxaead *aead_ctx)
{
	int err = SX_OK;
	uint32_t prng_value;

	if (aead_ctx->has_countermeasures) {
		err = cracen_prng_value_from_pool(&prng_value);
		if (err != SX_OK) {
			return err;
		}
	}

	sx_hw_reserve(&aead_ctx->dma);

	if (aead_ctx->has_countermeasures) {
		err = sx_cm_load_mask(prng_value);
		if (err != SX_OK) {
			goto exit;
		}
	}

	if (aead_ctx->key->prepare_key) {
		err = aead_ctx->key->prepare_key(aead_ctx->key->user_data);
	}

exit:
	if (err != SX_OK) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), err);
	}

	return err;
}

static int sx_aead_create_aesgcm(struct sxaead *aead_ctx, const struct sxkeyref *key,
				 const uint8_t *iv, size_t tagsz)
{
	uint32_t keyszfld = 0;
	int err;

	if (KEYREF_IS_INVALID(key)) {
		return SX_ERR_INVALID_KEYREF;
	}

	if (KEYREF_IS_USR(key)) {
		keyszfld = sx_aes_keysz(key->sz);
		if (keyszfld == ~0u) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	/* has countermeasures and the key need to be set before callling sx_aead_hw_reserve */
	aead_ctx->has_countermeasures = true;
	aead_ctx->key = key;
	err = sx_aead_hw_reserve(aead_ctx);
	if (err != SX_OK) {
		return err;
	}

	aead_ctx->cfg = &ba411gcmcfg;
	keyszfld = 0;

	sx_cmdma_newcmd(&aead_ctx->dma, aead_ctx->descs,
			CMDMA_AEAD_MODE_SET(aead_ctx->cfg->mode) | KEYREF_AES_HWKEY_CONF(key->cfg) |
				keyszfld,
			aead_ctx->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(aead_ctx->dma, key->key, key->sz, aead_ctx->cfg->dmatags->key);
	}
	ADD_CFGDESC(aead_ctx->dma, iv, SX_GCM_IV_SZ, aead_ctx->cfg->dmatags->iv_or_state);
	aead_ctx->totalaadsz = 0;
	aead_ctx->discardaadsz = 0;
	aead_ctx->datainsz = 0;
	aead_ctx->dataintotalsz = 0;
	aead_ctx->tagsz = tagsz;
	aead_ctx->expectedtag = aead_ctx->cfg->verifier;

	return SX_OK;
}

int sx_aead_create_aesgcm_enc(struct sxaead *aead_ctx, const struct sxkeyref *key,
			      const uint8_t *iv, size_t tagsz)
{
	int status;

	status = sx_aead_create_aesgcm(aead_ctx, key, iv, tagsz);
	if (status) {
		return status;
	}

	return SX_OK;
}

int sx_aead_create_aesgcm_dec(struct sxaead *aead_ctx, const struct sxkeyref *key,
			      const uint8_t *iv, size_t tagsz)
{
	int status;

	status = sx_aead_create_aesgcm(aead_ctx, key, iv, tagsz);
	if (status) {
		return status;
	}

	aead_ctx->dma.dmamem.cfg |= aead_ctx->cfg->decr;

	return SX_OK;
}

static int sx_aead_create_aesccm(struct sxaead *aead_ctx, const struct sxkeyref *key,
				 const uint8_t *nonce, size_t noncesz, size_t tagsz, size_t aadsz,
				 size_t datasz, const uint32_t dir)
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
	if ((tagsz & 1) || (tagsz < 4) || (tagsz > 16)) {
		return SX_ERR_INVALID_TAG_SIZE;
	}

	if (!sx_aead_aesccm_nonce_size_is_valid(noncesz)) {
		return SX_ERR_INVALID_NONCE_SIZE;
	}

	/* datasz must ensure  0 <= datasz < 2^(8L) */
	uint8_t l = 15 - noncesz;

	if ((l < 8U) && (datasz >= (1ULL << (l * 8)))) {
		/* message too long to encode the size in the CCM header */
		return SX_ERR_TOO_BIG;
	}

	/* has countermeasures and the key need to be set before callling sx_aead_hw_reserve */
	aead_ctx->has_countermeasures = true;
	aead_ctx->key = key;
	err = sx_aead_hw_reserve(aead_ctx);
	if (err != SX_OK) {
		return err;
	}

	aead_ctx->cfg = &ba411ccmcfg;
	sx_cmdma_newcmd(&aead_ctx->dma, aead_ctx->descs,
			CMDMA_AEAD_MODE_SET(aead_ctx->cfg->mode) | KEYREF_AES_HWKEY_CONF(key->cfg) |
				dir,
			aead_ctx->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(aead_ctx->dma, key->key, key->sz, aead_ctx->cfg->dmatags->key);
	}

	aead_ctx->totalaadsz = 0;
	aead_ctx->discardaadsz = 0;
	aead_ctx->datainsz = 0;
	aead_ctx->dataintotalsz = 0;
	aead_ctx->tagsz = tagsz;
	/* For CCM decryption, BA411 engine will compute the output tag as
	 * tagInputed ^ tagComputed. If inputed tag and computed tag are
	 * identical, the outputted tag will be an array of zeros with tagsz
	 * length. For encryption, expectedtag will be set to NULL by
	 * sx_aead_crypt() to disable verification.
	 */
	aead_ctx->expectedtag = aead_ctx->cfg->verifier;

	return SX_OK;
}

int sx_aead_create_aesccm_enc(struct sxaead *aead_ctx, const struct sxkeyref *key,
			      const uint8_t *nonce, size_t noncesz, size_t tagsz, size_t aadsz,
			      size_t datasz)
{
	return sx_aead_create_aesccm(aead_ctx, key, nonce, noncesz, tagsz, aadsz, datasz, 0);
}

int sx_aead_create_aesccm_dec(struct sxaead *aead_ctx, const struct sxkeyref *key,
			      const uint8_t *nonce, size_t noncesz, size_t tagsz, size_t aadsz,
			      size_t datasz)
{
	return sx_aead_create_aesccm(aead_ctx, key, nonce, noncesz, tagsz, aadsz, datasz,
				     ba411ccmcfg.decr);
}

int sx_aead_feed_aad(struct sxaead *aead_ctx, const uint8_t *aad, size_t aadsz)
{
	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (aadsz >= DMA_MAX_SZ) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), SX_ERR_TOO_BIG);
	}
	if (aead_ctx->dataintotalsz) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), SX_ERR_FEED_AFTER_DATA);
	}

	aead_ctx->totalaadsz += aadsz;
	aead_ctx->discardaadsz += aadsz;

	ADD_INDESCA(aead_ctx->dma, aad, aadsz, aead_ctx->cfg->dmatags->aad, 0xf);

	return SX_OK;
}

static void sx_aead_discard_aad(struct sxaead *aead_ctx)
{
	if (aead_ctx->discardaadsz) {
		ADD_DISCARDDESC(aead_ctx->dma, ALIGN_SZA(aead_ctx->discardaadsz, 0xf));
		aead_ctx->discardaadsz = 0;
	}
}

int sx_aead_crypt(struct sxaead *aead_ctx, const uint8_t *datain, size_t datainsz, uint8_t *dataout)
{
	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (datainsz >= DMA_MAX_SZ) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), SX_ERR_TOO_BIG);
	}

	sx_aead_discard_aad(aead_ctx);

	if (datainsz) {
		ADD_INDESCA(aead_ctx->dma, datain, datainsz, aead_ctx->cfg->dmatags->data, 0xf);
		aead_ctx->dataintotalsz += datainsz;
		aead_ctx->datainsz = datainsz;
		ADD_OUTDESCA(aead_ctx->dma, dataout, datainsz, 0xf);
	}
	return SX_OK;
}

static int sx_aead_run(struct sxaead *aead_ctx)
{
	sx_cmdma_start(&aead_ctx->dma, sizeof(aead_ctx->descs) + sizeof(aead_ctx->extramem),
		       aead_ctx->descs);

	return SX_OK;
}

int sx_aead_produce_tag(struct sxaead *aead_ctx, uint8_t *tagout)
{
	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (aead_ctx->cfg->mode == BA411_MODEID_CCM) {
		if ((aead_ctx->dma.dmamem.cfg & aead_ctx->cfg->ctxload) &&
		    (aead_ctx->datainsz == 0) && (aead_ctx->discardaadsz == 0)) {
			return sx_handle_nested_error(sx_aead_free(aead_ctx),
						      SX_ERR_INPUT_BUFFER_TOO_SMALL);
		}
	}
	if ((aead_ctx->dataintotalsz + aead_ctx->totalaadsz) < aead_ctx->cfg->inputminsz) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), SX_ERR_INCOMPATIBLE_HW);
	}

	if (aead_ctx->cfg->lenAlenC(aead_ctx->totalaadsz, aead_ctx->dataintotalsz,
				    &aead_ctx->extramem[0])) {
		ADD_INDESC_PRIV(aead_ctx->dma, OFFSET_EXTRAMEM(aead_ctx), AEAD_LENA_LENC_SZ,
				aead_ctx->cfg->dmatags->data);
	}

	sx_aead_discard_aad(aead_ctx);

	ADD_OUTDESCA(aead_ctx->dma, tagout, aead_ctx->tagsz, 0xf);

	aead_ctx->expectedtag = NULL;

	return sx_aead_run(aead_ctx);
}

int sx_aead_verify_tag(struct sxaead *aead_ctx, const uint8_t *tagin)
{
	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (aead_ctx->cfg->mode == BA411_MODEID_CCM) {
		if ((aead_ctx->dma.dmamem.cfg & aead_ctx->cfg->ctxload) &&
		    (aead_ctx->datainsz == 0) && (aead_ctx->discardaadsz == 0)) {
			return sx_handle_nested_error(sx_aead_free(aead_ctx),
						      SX_ERR_INPUT_BUFFER_TOO_SMALL);
		}
	}
	if ((aead_ctx->dataintotalsz + aead_ctx->totalaadsz) < aead_ctx->cfg->inputminsz) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), SX_ERR_INCOMPATIBLE_HW);
	}

	if (aead_ctx->cfg->lenAlenC(aead_ctx->totalaadsz, aead_ctx->dataintotalsz,
				    &aead_ctx->extramem[0])) {
		ADD_INDESC_PRIV(aead_ctx->dma, OFFSET_EXTRAMEM(aead_ctx), AEAD_LENA_LENC_SZ,
				aead_ctx->cfg->dmatags->data);
		aead_ctx->expectedtag = tagin;
	} else {
		if (DMATAG_DATATYPE_REFERENCE ==
		    (aead_ctx->cfg->dmatags->tag & DMATAG_DATATYPE_REFERENCE)) {
			UPDATE_LASTDESC_TAG(aead_ctx->dma, DMATAG_LAST);
		}

		ADD_INDESCA(aead_ctx->dma, tagin, aead_ctx->tagsz, aead_ctx->cfg->dmatags->tag,
			    0xf);
	}

	sx_aead_discard_aad(aead_ctx);

	ADD_OUTDESC_PRIV(aead_ctx->dma, OFFSET_EXTRAMEM(aead_ctx), aead_ctx->tagsz, 0xf);

	return sx_aead_run(aead_ctx);
}

int sx_aead_resume_state(struct sxaead *aead_ctx)
{
	int err;

	if (aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	err = sx_aead_hw_reserve(aead_ctx);
	if (err != SX_OK) {
		return err;
	}

	aead_ctx->dma.dmamem.cfg &= ~(aead_ctx->cfg->ctxsave);
	sx_cmdma_newcmd(&aead_ctx->dma, aead_ctx->descs,
			aead_ctx->dma.dmamem.cfg | aead_ctx->cfg->ctxload,
			aead_ctx->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(aead_ctx->key)) {
		ADD_CFGDESC(aead_ctx->dma, aead_ctx->key->key, aead_ctx->key->sz,
			    aead_ctx->cfg->dmatags->key);
	}
	ADD_INDESC_PRIV(
		aead_ctx->dma,
		(OFFSET_EXTRAMEM(aead_ctx) + sizeof(aead_ctx->extramem) - aead_ctx->cfg->statesz),
		aead_ctx->cfg->statesz, aead_ctx->cfg->dmatags->iv_or_state);

	aead_ctx->datainsz = 0;
	aead_ctx->discardaadsz = 0;

	return SX_OK;
}

int sx_aead_save_state(struct sxaead *aead_ctx)
{
	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (aead_ctx->cfg->statesz == 0) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx),
					      SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED);
	}

	sx_aead_discard_aad(aead_ctx);

	ADD_OUTDESC_PRIV(
		aead_ctx->dma,
		(OFFSET_EXTRAMEM(aead_ctx) + sizeof(aead_ctx->extramem) - aead_ctx->cfg->statesz),
		aead_ctx->cfg->statesz, 0x0F);

	aead_ctx->dma.dmamem.cfg |= aead_ctx->cfg->ctxsave;

	sx_cmdma_start(&aead_ctx->dma, sizeof(aead_ctx->descs) + sizeof(aead_ctx->extramem),
		       aead_ctx->descs);

	return SX_OK;
}

int sx_aead_status(struct sxaead *aead_ctx)
{
	int status;

	if (!aead_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	status = sx_cmdma_check();
	if (status == SX_ERR_HW_PROCESSING) {
		return status;
	}
	if (status) {
		return sx_handle_nested_error(sx_aead_free(aead_ctx), status);
	}

#if CONFIG_DCACHE
	sys_cache_data_invd_range((void *)&aead_ctx->extramem, sizeof(aead_ctx->extramem));
#endif

	if (!(aead_ctx->dma.dmamem.cfg & aead_ctx->cfg->ctxsave) && aead_ctx->expectedtag != NULL) {
		status = sx_memdiff(aead_ctx->expectedtag, (const uint8_t *)aead_ctx->extramem,
				    aead_ctx->tagsz)
				 ? SX_ERR_INVALID_TAG
				 : SX_OK;
	}

	return sx_handle_nested_error(sx_aead_free(aead_ctx), status);
}

int sx_aead_wait(struct sxaead *aead_ctx)
{
	int status = SX_ERR_HW_PROCESSING;

	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_aead_status(aead_ctx);
	}

	return status;
}
