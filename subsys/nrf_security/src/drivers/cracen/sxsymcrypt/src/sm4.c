/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/sm4.h"
#include "../include/sxsymcrypt/aead.h"
#include "../include/sxsymcrypt/mac.h"
#include "../include/sxsymcrypt/keyref.h"
#include <cracen/statuscodes.h>
#include "blkcipherdefs.h"
#include "aeaddefs.h"
#include "keyrefdefs.h"
#include "macdefs.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "cmaes.h"

/* Size of the key used in SM4 */
#define SX_BLKCIPHER_SM4_KEYSZ 16

#define CMDMA_BA419_BUS_MSK   (0x0F)
#define SM4_CMAC_MODEID_BA419 8
#define SM4_CMAC_BLOCK_SZ     16
#define SM4_CMAC_STATE_SZ     16
#define SM4_CMAC_MAC_SZ	      16

/** Mode Register value for context loading */
#define SM4_MODEID_CTX_LOAD (1u << 4)
/** Mode Register value for context saving */
#define SM4_MODEID_CTX_SAVE (1u << 5)

/** SM4 block size, in bytes */
#define SM4_BLOCK_SZ	      (16)
/** SM4 GCM and CCM context saving state size, in bytes */
#define SM4_AEAD_CTX_STATE_SZ (32)

static int lenAlenC_nop(size_t aadsz, size_t datasz, uint8_t *out);
static int lenAlenC_sm4gcm_ba419(size_t aadsz, size_t datasz, uint8_t *out);

static const struct sx_blkcipher_cmdma_tags ba419tags = {.cfg = DMATAG_BA419 | DMATAG_CONFIG(0),
							 .key = DMATAG_BA419 | DMATAG_CONFIG(0x08),
							 .key2 = DMATAG_BA419 | DMATAG_CONFIG(0x48),
							 .iv_or_state =
								 DMATAG_BA419 | DMATAG_CONFIG(0x28),
							 .data = DMATAG_BA419};

static const struct sx_blkcipher_cmdma_cfg ba419cfg = {
	.encr = 0,
	.decr = CM_CFG_DECRYPT,
	.dmatags = &ba419tags,
};

static const struct sx_aead_cmdma_tags ba419aeadtags = {
	.cfg = DMATAG_BA419 | DMATAG_CONFIG(0),
	.iv_or_state = DMATAG_BA419 | DMATAG_CONFIG(0x28),
	.key = DMATAG_BA419 | DMATAG_CONFIG(0x08),
	.aad = DMATAG_BA419 | DMATAG_DATATYPE_HEADER,
	.tag = 0,
	.data = DMATAG_BA419};

#define SM4_MODEID_GCM 6
static const struct sx_aead_cmdma_cfg ba419gcmcfg = {.encr = 0,
						     .decr = CM_CFG_DECRYPT,
						     .mode = SM4_MODEID_GCM,
						     .dmatags = &ba419aeadtags,
						     .verifier = NULL,
						     .lenAlenC = lenAlenC_sm4gcm_ba419,
						     .ctxsave = SM4_MODEID_CTX_SAVE,
						     .ctxload = SM4_MODEID_CTX_LOAD,
						     .granularity = SM4_BLOCK_SZ,
						     .ctxsz = SM4_AEAD_CTX_STATE_SZ};

#define SM4_MODEID_CCM 5
const struct sx_aead_cmdma_cfg ba419ccmcfg = {.encr = 0,
					      .decr = CM_CFG_DECRYPT,
					      .mode = SM4_MODEID_CCM,
					      .dmatags = &ba419aeadtags,
					      .verifier = NULL,
					      .lenAlenC = lenAlenC_nop,
					      .ctxsave = SM4_MODEID_CTX_SAVE,
					      .ctxload = SM4_MODEID_CTX_LOAD,
					      .granularity = SM4_BLOCK_SZ,
					      .ctxsz = SM4_AEAD_CTX_STATE_SZ};

static const struct sx_mac_cmdma_tags ba419tags_cmac = {.cfg = DMATAG_BA419 | DMATAG_CONFIG(0),
							.state = DMATAG_BA419 | DMATAG_CONFIG(0x28),
							.key = DMATAG_BA419 | DMATAG_CONFIG(0x08),
							.data = DMATAG_BA419};

static const struct sx_mac_cmdma_cfg ba419cfg_cmac = {
	.cmdma_mask = CMDMA_BA419_BUS_MSK,
	.granularity = SM4_CMAC_BLOCK_SZ,
	.blocksz = SM4_CMAC_BLOCK_SZ,
	.statesz = SM4_CMAC_STATE_SZ,
	.savestate = SM4_MODEID_CTX_SAVE,
	.loadstate = SM4_MODEID_CTX_LOAD,
	.dmatags = &ba419tags_cmac,
};

int lenAlenC_nop(size_t aadsz, size_t datasz, uint8_t *out)
{
	(void)aadsz;
	(void)datasz;
	(void)out;

	return 0;
}

static int lenAlenC_sm4gcm_ba419(size_t aadsz, size_t datasz, uint8_t *out)
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

static int sx_blkcipher_newcmd(struct sxblkcipher *c, const struct sxkeyref *key,
			       const uint32_t mode, const uint32_t dir)
{
	memcpy(&c->key, key, sizeof(c->key));
	sx_hw_reserve(&c->dma);

	c->cfg = &ba419cfg;
	sx_cmdma_newcmd(&c->dma, c->allindescs, CMDMA_BLKCIPHER_MODE_SET(mode) | dir,
			c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
	}

	c->mode = mode;

	return SX_OK;
}

static int sx_blkcipher_create_sm4_ba419(struct sxblkcipher *c, const struct sxkeyref *key,
					 const char *iv, const uint32_t mode, const uint32_t dir)
{
	int r;

	if (KEYREF_IS_INVALID(key) || !KEYREF_IS_USR(key)) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key)) {
		if (key->sz != SX_BLKCIPHER_SM4_KEYSZ) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	r = sx_blkcipher_newcmd(c, key, mode, dir);
	if (r) {
		return r;
	}

	if (iv != NULL) {
		ADD_CFGDESC(c->dma, iv, 16, c->cfg->dmatags->iv_or_state);
	}

	return SX_OK;
}

int sx_blkcipher_create_sm4ctr_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CTR, ba419cfg.encr);
}

int sx_blkcipher_create_sm4ctr_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CTR, ba419cfg.decr);
}

int sx_blkcipher_create_sm4ecb_enc(struct sxblkcipher *c, const struct sxkeyref *key)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, NULL, BLKCIPHER_MODEID_ECB, ba419cfg.encr);
}

int sx_blkcipher_create_sm4ecb_dec(struct sxblkcipher *c, const struct sxkeyref *key)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, NULL, BLKCIPHER_MODEID_ECB, ba419cfg.decr);
}

int sx_blkcipher_create_sm4cbc_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CBC, ba419cfg.encr);
}

int sx_blkcipher_create_sm4cbc_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CBC, ba419cfg.decr);
}

int sx_blkcipher_create_sm4cfb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CFB, ba419cfg.encr);
}

int sx_blkcipher_create_sm4cfb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_CFB, ba419cfg.decr);
}

int sx_blkcipher_create_sm4ofb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_OFB, ba419cfg.encr);
}

int sx_blkcipher_create_sm4ofb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv)
{
	c->inminsz = 16;
	c->granularity = 16;
	return sx_blkcipher_create_sm4_ba419(c, key, iv, BLKCIPHER_MODEID_OFB, ba419cfg.decr);
}

int sx_aead_create_sm4gcm(struct sxaead *c, const struct sxkeyref *key, const char *iv,
			  const int dir)
{
	if (KEYREF_IS_INVALID(key) || !KEYREF_IS_USR(key)) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key)) {
		if (key->sz != SX_BLKCIPHER_SM4_KEYSZ) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	sx_hw_reserve(&c->dma);
	c->cfg = &ba419gcmcfg;

	sx_cmdma_newcmd(&c->dma, c->allindescs, CMDMA_AEAD_MODE_SET(c->cfg->mode) | dir,
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
	c->tagsz = SX_GCM_TAG_SZ;
	c->expectedtag = c->cfg->verifier;
	c->key = key;
	c->dma.out = c->dma.dmamem.outdescs;

	return SX_OK;
}

int sx_aead_create_sm4gcm_enc(struct sxaead *c, const struct sxkeyref *key, const char *iv)
{
	return sx_aead_create_sm4gcm(c, key, iv, ba419gcmcfg.encr);
}

int sx_aead_create_sm4gcm_dec(struct sxaead *c, const struct sxkeyref *key, const char *iv)
{
	return sx_aead_create_sm4gcm(c, key, iv, ba419gcmcfg.decr);
}

static int sx_aead_create_sm4ccm(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
				 size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz,
				 const uint32_t dir)
{
	if (KEYREF_IS_INVALID(key) || !KEYREF_IS_USR(key)) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key)) {
		if (key->sz != SX_BLKCIPHER_SM4_KEYSZ) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}
	if ((tagsz & 1) || (tagsz < 4) || (tagsz > 16)) {
		return SX_ERR_INVALID_TAG_SIZE;
	}
	if ((noncesz < 7) || (noncesz > 13)) {
		return SX_ERR_INVALID_NONCE_SIZE;
	}

	/* datasz must ensure  0 <= datasz < 2^(8L) */
	uint8_t l = 15 - noncesz;

	if ((l < 8U) && (datasz >= (1ULL << (l * 8)))) {
		/* message too long to encode the size in the CCM header */
		return SX_ERR_TOO_BIG;
	}

	sx_hw_reserve(&c->dma);

	c->cfg = &ba419ccmcfg;
	sx_cmdma_newcmd(&c->dma, c->allindescs, CMDMA_AEAD_MODE_SET(c->cfg->mode) | dir,
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
	c->key = key;
	c->dma.out = c->dma.dmamem.outdescs;
	c->expectedtag = c->cfg->verifier;

	return SX_OK;
}

int sx_aead_create_sm4ccm_enc(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz)
{
	return sx_aead_create_sm4ccm(c, key, nonce, noncesz, tagsz, aadsz, datasz,
				     ba419ccmcfg.encr);
}

int sx_aead_create_sm4ccm_dec(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz)
{
	return sx_aead_create_sm4ccm(c, key, nonce, noncesz, tagsz, aadsz, datasz,
				     ba419ccmcfg.decr);
}

static int sx_blkcipher_create_sm4xts(struct sxblkcipher *c, const struct sxkeyref *key1,
				      const struct sxkeyref *key2, const char *iv,
				      const uint32_t dir)
{
	int r;

	if (KEYREF_IS_INVALID(key1) || KEYREF_IS_INVALID(key2)) {
		return SX_ERR_INVALID_KEYREF;
	}
	/* XTS does not support HW keys, therefore both keys must be user
	 * provided keys
	 */
	if (!(KEYREF_IS_USR(key1) && KEYREF_IS_USR(key2))) {
		return SX_ERR_HW_KEY_NOT_SUPPORTED;
	}
	if ((key1->sz != key2->sz) || (key1->sz != SX_BLKCIPHER_SM4_KEYSZ)) {
		return SX_ERR_INVALID_KEY_SZ;
	}

	r = sx_blkcipher_newcmd(c, key1, BLKCIPHER_MODEID_XTS, dir);
	if (r) {
		return r;
	}
	ADD_CFGDESC(c->dma, key2->key, key2->sz, c->cfg->dmatags->key2);
	ADD_CFGDESC(c->dma, iv, 16, c->cfg->dmatags->iv_or_state);

	c->inminsz = 16;
	c->granularity = 1;

	return SX_OK;
}

int sx_blkcipher_create_sm4xts_enc(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv)
{
	return sx_blkcipher_create_sm4xts(c, key1, key2, iv, ba419cfg.encr);
}

int sx_blkcipher_create_sm4xts_dec(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv)
{
	return sx_blkcipher_create_sm4xts(c, key1, key2, iv, ba419cfg.decr);
}

static int sx_cmac_create_sm4_ba419(struct sxmac *c, const struct sxkeyref *key)
{
	if (KEYREF_IS_INVALID(key) || !KEYREF_IS_USR(key)) {
		return SX_ERR_INVALID_KEYREF;
	}
	if (KEYREF_IS_USR(key)) {
		if (key->sz != SX_BLKCIPHER_SM4_KEYSZ) {
			return SX_ERR_INVALID_KEY_SZ;
		}
	}

	sx_hw_reserve(&c->dma);

	c->cfg = &ba419cfg_cmac;
	c->cntindescs = 1;
	sx_cmdma_newcmd(&c->dma, c->allindescs, CMDMA_CMAC_MODE_SET(SM4_CMAC_MODEID_BA419),
			c->cfg->dmatags->cfg);
	if (KEYREF_IS_USR(key)) {
		ADD_CFGDESC(c->dma, key->key, key->sz, c->cfg->dmatags->key);
		c->cntindescs++;
	}
	c->dma.out = c->dma.dmamem.outdescs;
	c->feedsz = 0;
	c->macsz = SM4_CMAC_MAC_SZ;
	c->dma.out = c->dma.dmamem.outdescs;
	c->key = key;

	return SX_OK;
}

int sx_mac_create_sm4cmac(struct sxmac *c, const struct sxkeyref *key)
{
	return sx_cmac_create_sm4_ba419(c, key);
}
