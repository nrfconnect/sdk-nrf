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

static int lenAlenC_aesgcm_ba411(size_t aadsz, size_t datasz, uint8_t *out);
static int lenAlenC_nop(size_t aadsz, size_t datasz, uint8_t *out);
extern const struct sx_aead_cmdma_cfg ba419ccmcfg;

static const char zeros[SX_CCM_MAX_TAG_SZ] = {0};

static const struct sx_aead_cmdma_tags ba411aeadtags = {
	.cfg = DMATAG_BA411 | DMATAG_CONFIG(0),
	.iv_or_state = DMATAG_BA411 | DMATAG_CONFIG(0x28),
	.key = DMATAG_BA411 | DMATAG_CONFIG(0x08),
	.aad = DMATAG_BA411 | DMATAG_DATATYPE_HEADER,
	.tag = 0,
	.data = DMATAG_BA411};

#define BA411_MODEID_GCM 6
static const struct sx_aead_cmdma_cfg ba411gcmcfg = {.encr = 0,
						     .decr = CM_CFG_DECRYPT,
						     .mode = BA411_MODEID_GCM,
						     .dmatags = &ba411aeadtags,
						     .verifier = NULL,
						     .lenAlenC = lenAlenC_aesgcm_ba411,
						     .ctxsave = AES_AEAD_MODEID_CTX_SAVE,
						     .ctxload = AES_AEAD_MODEID_CTX_LOAD,
						     .granularity = AEAD_BLOCK_SZ,
						     .ctxsz = AES_AEAD_CTX_STATE_SZ};

#define BA411_MODEID_CCM 5
static const struct sx_aead_cmdma_cfg ba411ccmcfg = {.encr = 0,
						     .decr = CM_CFG_DECRYPT,
						     .mode = BA411_MODEID_CCM,
						     .dmatags = &ba411aeadtags,
						     .verifier = zeros,
						     .lenAlenC = lenAlenC_nop,
						     .ctxsave = AES_AEAD_MODEID_CTX_SAVE,
						     .ctxload = AES_AEAD_MODEID_CTX_LOAD,
						     .granularity = AEAD_BLOCK_SZ,
						     .ctxsz = AES_AEAD_CTX_STATE_SZ};

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

void sx_aead_free(struct sxaead *c)
{
	if (c->key->clean_key) {
		c->key->clean_key(c->key->user_data);
	}
	sx_cmdma_release_hw(&c->dma);
}

static int sx_aead_hw_reserve(struct sxaead *c)
{
	int err = SX_OK;
	uint32_t prng_value;

	if (c->aes_countermeasures == BA411_AES_COUNTERMEASURES_ENABLE) {
		err = cracen_prng_value_from_pool(&prng_value);
		if (err != SX_OK) {
			return err;
		}
	}

	sx_hw_reserve(&c->dma);

	if (c->aes_countermeasures == BA411_AES_COUNTERMEASURES_ENABLE) {
		err = sx_cm_load_mask(prng_value);
		if (err != SX_OK) {
			goto exit;
		}
	}

	if (c->key->prepare_key) {
		err = c->key->prepare_key(c->key->user_data);
	}

exit:
	if (err != SX_OK) {
		sx_aead_free(c);
	}

	return err;
}

static int sx_aead_create_aesgcm(struct sxaead *c, const struct sxkeyref *key, const char *iv,
				 size_t tagsz)
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

	c->key = key;
	err = sx_aead_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->cfg = &ba411gcmcfg;
	keyszfld = 0;

	sx_cmdma_newcmd(&c->dma, c->allindescs,
			CMDMA_AEAD_MODE_SET(c->cfg->mode) | KEYREF_BA411E_HWKEY_CONF(key->cfg) |
				keyszfld,
			c->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
	}
	ADD_CFGDESC(c->dma, iv, SX_GCM_IV_SZ, c->cfg->dmatags->iv_or_state);
	c->totalaadsz = 0;
	c->discardaadsz = 0;
	c->datainsz = 0;
	c->dataintotalsz = 0;
	c->is_in_ctx = false;
	c->tagsz = tagsz;
	c->expectedtag = c->cfg->verifier;
	c->dma.out = c->dma.dmamem.outdescs;

	return SX_OK;
}

int sx_aead_create_aesgcm_enc(struct sxaead *c, const struct sxkeyref *key, const char *iv,
			      size_t tagsz)
{
	int r;

	c->aes_countermeasures = BA411_AES_COUNTERMEASURES_ENABLE;
	r = sx_aead_create_aesgcm(c, key, iv, tagsz);
	if (r) {
		return r;
	}

	c->dma.dmamem.cfg |= c->cfg->encr;

	return SX_OK;
}

int sx_aead_create_aesgcm_dec(struct sxaead *c, const struct sxkeyref *key, const char *iv,
			      size_t tagsz)
{
	int r;

	c->aes_countermeasures = BA411_AES_COUNTERMEASURES_ENABLE;
	r = sx_aead_create_aesgcm(c, key, iv, tagsz);
	if (r) {
		return r;
	}

	c->dma.dmamem.cfg |= c->cfg->decr;

	return SX_OK;
}

static int sx_aead_create_aesccm(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
				 size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz,
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

	c->key = key;
	err = sx_aead_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->cfg = &ba411ccmcfg;
	sx_cmdma_newcmd(&c->dma, c->allindescs,
			CMDMA_AEAD_MODE_SET(c->cfg->mode) | KEYREF_BA411E_HWKEY_CONF(key->cfg) |
				dir,
			c->cfg->dmatags->cfg);

	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
	}

	c->totalaadsz = 0;
	c->discardaadsz = 0;
	c->datainsz = 0;
	c->dataintotalsz = 0;
	c->is_in_ctx = false;
	c->tagsz = tagsz;
	c->dma.out = c->dma.dmamem.outdescs;
	/* For CCM decryption, BA411 engine will compute the output tag as
	 * tagInputed ^ tagComputed. If inputed tag and computed tag are
	 * identical, the outputted tag will be an array of zeros with tagsz
	 * length. For encryption, expectedtag will be set to NULL by
	 * sx_aead_crypt() to disable verification.
	 */
	c->expectedtag = c->cfg->verifier;

	return SX_OK;
}

int sx_aead_create_aesccm_enc(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz)
{
	c->aes_countermeasures = BA411_AES_COUNTERMEASURES_ENABLE;
	return sx_aead_create_aesccm(c, key, nonce, noncesz, tagsz, aadsz, datasz,
				     ba411ccmcfg.encr);
}

int sx_aead_create_aesccm_dec(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz)
{
	c->aes_countermeasures = BA411_AES_COUNTERMEASURES_ENABLE;
	return sx_aead_create_aesccm(c, key, nonce, noncesz, tagsz, aadsz, datasz,
				     ba411ccmcfg.decr);
}

int sx_aead_feed_aad(struct sxaead *c, const char *aad, size_t aadsz)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (aadsz >= DMA_MAX_SZ) {
		sx_aead_free(c);
		return SX_ERR_TOO_BIG;
	}
	if (c->dataintotalsz) {
		sx_aead_free(c);
		return SX_ERR_FEED_AFTER_DATA;
	}

	c->totalaadsz += aadsz;
	c->discardaadsz += aadsz;

	ADD_INDESC(c->dma, aad, aadsz, c->cfg->dmatags->aad);

	return SX_OK;
}

static void sx_aead_discard_aad(struct sxaead *c)
{
	if (c->discardaadsz) {
		ADD_DISCARDDESC(c->dma.out, ALIGN_SZ(c->discardaadsz));
		c->discardaadsz = 0;
	}
}

int sx_aead_crypt(struct sxaead *c, const char *datain, size_t datainsz, char *dataout)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (datainsz >= DMA_MAX_SZ) {
		sx_aead_free(c);
		return SX_ERR_TOO_BIG;
	}

	if (datainsz) {
		ADD_INDESC(c->dma, datain, datainsz, c->cfg->dmatags->data);
		c->dataintotalsz += datainsz;
		c->datainsz = datainsz;
	}

	sx_aead_discard_aad(c);

	ADD_OUTDESC(c->dma.out, dataout, datainsz);

	return SX_OK;
}

static int sx_aead_run(struct sxaead *c)
{
	sx_cmdma_finalize_descs(c->allindescs, c->dma.d - 1);
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, c->dma.out - 1);

	sx_cmdma_start(&c->dma, sizeof(c->allindescs) + sizeof(c->extramem), c->allindescs);

	return SX_OK;
}

int sx_aead_produce_tag(struct sxaead *c, char *tagout)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (c->cfg->mode == BA411_MODEID_CCM) {
		if ((c->is_in_ctx) && (c->datainsz == 0) && (c->discardaadsz == 0)) {
			sx_aead_free(c);
			return SX_ERR_INPUT_BUFFER_TOO_SMALL;
		}
	}

	if (c->cfg->lenAlenC(c->totalaadsz, c->dataintotalsz, &c->extramem[0])) {
		ADD_INDESC_PRIV(c->dma, OFFSET_EXTRAMEM(c), 16, c->cfg->dmatags->data);
	}

	sx_aead_discard_aad(c);

	ADD_OUTDESC(c->dma.out, tagout, c->tagsz);

	c->expectedtag = NULL;
	c->is_in_ctx = false;

	return sx_aead_run(c);
}

int sx_aead_verify_tag(struct sxaead *c, const char *tagin)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (c->cfg->mode == BA411_MODEID_CCM) {
		if ((c->is_in_ctx) && (c->datainsz == 0) && (c->discardaadsz == 0)) {
			sx_aead_free(c);
			return SX_ERR_INPUT_BUFFER_TOO_SMALL;
		}
	}

	if (c->cfg->lenAlenC(c->totalaadsz, c->dataintotalsz, &c->extramem[0])) {
		ADD_INDESC_PRIV(c->dma, OFFSET_EXTRAMEM(c), 16, c->cfg->dmatags->data);
		c->expectedtag = tagin;
	} else if (c->cfg == &ba419ccmcfg) {
		c->expectedtag = tagin;
	} else {
		ADD_INDESC(c->dma, tagin, c->tagsz, c->cfg->dmatags->data);
	}

	sx_aead_discard_aad(c);

	if (c->cfg->dmatags->tag) {
		/* The hardware will output a 4 bytes tag. It will be all 0 if
		 * authentication with the tag succeeded. Set tagsz to the size
		 * of that "computed validation tag" for the check by
		 * sx_aead_status().
		 */
		c->tagsz = 4;
	}
	ADD_OUTDESC_PRIV(c->dma, c->dma.out, OFFSET_EXTRAMEM(c), c->tagsz, 0xf);
	c->is_in_ctx = false;

	return sx_aead_run(c);
}

int sx_aead_resume_state(struct sxaead *c)
{
	int err;

	if (c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	err = sx_aead_hw_reserve(c);
	if (err != SX_OK) {
		return err;
	}

	c->dma.dmamem.cfg &= ~(c->cfg->ctxsave);
	sx_cmdma_newcmd(&c->dma, c->allindescs, c->dma.dmamem.cfg | c->cfg->ctxload,
			c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(c->key)) {
		ADD_CFGDESC(c->dma, c->key->key, c->key->sz, c->cfg->dmatags->key);
	}
	ADD_INDESC_PRIV(c->dma, (OFFSET_EXTRAMEM(c) + sizeof(c->extramem) - c->cfg->ctxsz),
			c->cfg->ctxsz, c->cfg->dmatags->iv_or_state);

	c->datainsz = 0;
	c->dma.out = c->dma.dmamem.outdescs;
	c->discardaadsz = 0;

	return SX_OK;
}

int sx_aead_save_state(struct sxaead *c)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	sx_aead_discard_aad(c);

	ADD_OUTDESC_PRIV(c->dma, c->dma.out,
			 (OFFSET_EXTRAMEM(c) + sizeof(c->extramem) - c->cfg->ctxsz), c->cfg->ctxsz,
			 0x0F);

	c->dma.dmamem.cfg |= c->cfg->ctxsave;
	c->is_in_ctx = true;

	sx_cmdma_finalize_descs(c->allindescs, c->dma.d - 1);
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, c->dma.out - 1);

	sx_cmdma_start(&c->dma, sizeof(c->allindescs) + sizeof(c->extramem), c->allindescs);

	return SX_OK;
}

int sx_aead_status(struct sxaead *c)
{
	int r;

	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	r = sx_cmdma_check();
	if (r == SX_ERR_HW_PROCESSING) {
		return r;
	}

	if ((!r) && (c->expectedtag != NULL) && (!c->is_in_ctx)) {
		r = sx_memdiff(c->expectedtag, (const char *)c->extramem, c->tagsz)
			    ? SX_ERR_INVALID_TAG
			    : SX_OK;
	}

	sx_aead_free(c);

	return r;
}

int sx_aead_wait(struct sxaead *c)
{
	int r = SX_ERR_HW_PROCESSING;

	while (r == SX_ERR_HW_PROCESSING) {
		r = sx_aead_status(c);
	}

	return r;
}
