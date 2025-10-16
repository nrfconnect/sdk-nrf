/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/internal.h>
#include <cracen/statuscodes.h>
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "macdefs.h"

#define BA413_HMAC_CONF (1 << 8)
#define HASH_HW_PAD	(1 << 9)
#define HASH_FINAL	(1 << 10)
#define EXPORT_STATE	(~HASH_HW_PAD & ~HASH_FINAL)

static const struct sx_digesttags ba413tags = {
	.cfg = DMATAG_BA413 | DMATAG_CONFIG(0),
	.initialstate = DMATAG_BA413 | DMATAG_DATATYPE(1) | DMATAG_LAST,
	.keycfg = DMATAG_BA413 | DMATAG_DATATYPE(2) | DMATAG_LAST,
	.data = DMATAG_BA413 | DMATAG_DATATYPE(0)};

#define HASH_INVALID_BYTES  4 /* number of invalid bytes when empty message, required by HW */
#define CMDMA_BA413_MODE(x) ((x) | (HASH_HW_PAD | HASH_FINAL))

static int sx_hash_create_ba413(struct sxhash *hash_ctx, size_t csz);

const struct sxhashalg sxhashalg_sha1 = {
	CMDMA_BA413_MODE(0x02), HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SHA1,
	SX_HASH_BLOCKSZ_SHA1,	20,	     72,	 sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_224 = {
	CMDMA_BA413_MODE(0x04),	  HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SHA2_224,
	SX_HASH_BLOCKSZ_SHA2_224, 32,	       72,	   sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_256 = {
	CMDMA_BA413_MODE(0x08),	  HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SHA2_256,
	SX_HASH_BLOCKSZ_SHA2_256, 32,	       72,	   sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_384 = {
	CMDMA_BA413_MODE(0x10),	  HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SHA2_384,
	SX_HASH_BLOCKSZ_SHA2_384, 64,	       144,	   sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_512 = {
	CMDMA_BA413_MODE(0x20),	  HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SHA2_512,
	SX_HASH_BLOCKSZ_SHA2_512, 64,	       144,	   sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sm3 = {
	CMDMA_BA413_MODE(0x40), HASH_HW_PAD, HASH_FINAL, SX_HASH_DIGESTSZ_SM3,
	SX_HASH_BLOCKSZ_SM3,	32,	     72,	 sx_hash_create_ba413};

#define CMDMA_BA413_BUS_MSK 3

void sx_hash_free(struct sxhash *hash_ctx)
{
	sx_cmdma_release_hw(&hash_ctx->dma);
}

static void sx_hash_pad(struct sxhash *hash_ctx)
{
	uint8_t *padding = hash_ctx->extramem;
	size_t padsz;
	size_t total_feed_size = hash_ctx->totalfeedsz;

	/* as per SHA standard, write the full hashed message size in
	 * big endian form starting with 0x80 and as many 0 bytes as
	 * needed to have full block size.
	 */
	size_t length_field_sz = (hash_ctx->algo->digestsz >= 48) ? 16 : 8;

	padsz = hash_ctx->algo->blocksz - (hash_ctx->totalfeedsz & (hash_ctx->algo->blocksz - 1));
	if (padsz < (length_field_sz + 1)) {
		padsz += hash_ctx->algo->blocksz;
	}

	padding[padsz - 1] = (total_feed_size & 0x1f) << 3; /* multiply by 8 for bits */
	total_feed_size >>= 5;
	for (size_t i = padsz - 2; i > 0; i--) {
		padding[i] = total_feed_size & 0xFF;
		total_feed_size >>= 8;
	}
	padding[0] = 0x80;

	ADD_INDESC_PRIV_RAW(hash_ctx->dma, OFFSET_EXTRAMEM(hash_ctx), padsz,
			    hash_ctx->dmatags->data);
	hash_ctx->feedsz += padsz;
	/* Padding is done only when sx_hash_resume_state() has been called and
	 * sx_hash_resume_state() already reserved room for this extra
	 * descriptor.
	 */
}

void sx_ba413_digest(struct sxhash *hash_ctx, uint8_t *digest)
{
	if (hash_ctx->totalfeedsz == 0) {
		/* If we get here it means an empty message hash will be
		 * generated. 4 dummy bytes will be added to hash, these bytes
		 * will be dropped by the hash engine.
		 */
		ADD_EMPTY_INDESC(hash_ctx->dma, HASH_INVALID_BYTES, hash_ctx->dmatags->data);
	}

	if (!(hash_ctx->dma.dmamem.cfg & HASH_HW_PAD)) {
		sx_hash_pad(hash_ctx);
	}
	SET_LAST_DESC_IGN(hash_ctx->dma, hash_ctx->feedsz, CMDMA_BA413_BUS_MSK);

	ADD_OUTDESCA(hash_ctx->dma, digest, hash_ctx->algo->digestsz, CMDMA_BA413_BUS_MSK);

	if (!(hash_ctx->dma.dmamem.cfg & HASH_FINAL) &&
	    (hash_ctx->algo->statesz - hash_ctx->algo->digestsz)) {
		ADD_DISCARDDESC(hash_ctx->dma,
				(hash_ctx->algo->statesz - hash_ctx->algo->digestsz));
	}
}

static int sx_hash_create_ba413(struct sxhash *hash_ctx, size_t csz)
{
	if (csz < sizeof(*hash_ctx)) {
		return SX_ERR_ALLOCATION_TOO_SMALL;
	}

	/* SM3 is not supported by CRACEN in nRF devices */
	if (hash_ctx->algo->cfgword & REG_BA413_CAPS_ALGO_SM3) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	sx_hw_reserve(&hash_ctx->dma);

	hash_ctx->dmatags = &ba413tags;
	sx_cmdma_newcmd(&hash_ctx->dma, hash_ctx->descs, hash_ctx->algo->cfgword,
			hash_ctx->dmatags->cfg);
	hash_ctx->feedsz = 0;
	hash_ctx->totalfeedsz = 0;
	hash_ctx->digest = sx_ba413_digest;

	hash_ctx->cntindescs = 1;

	return SX_OK;
}

int sx_hash_create(struct sxhash *hash_ctx, const struct sxhashalg *alg, size_t csz)
{
	hash_ctx->algo = alg;

	return alg->reservehw(hash_ctx, csz);
}

int sx_hash_resume_state(struct sxhash *hash_ctx)
{
	if (!hash_ctx->algo || hash_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	size_t totalfeedsz = hash_ctx->totalfeedsz;
	int status = hash_ctx->algo->reservehw(hash_ctx, sizeof(*hash_ctx));

	if (status) {
		return status;
	}
	hash_ctx->totalfeedsz = totalfeedsz;
	ADD_INDESC_PRIV(
		hash_ctx->dma,
		(OFFSET_EXTRAMEM(hash_ctx) + sizeof(hash_ctx->extramem) - hash_ctx->algo->statesz),
		hash_ctx->algo->statesz, hash_ctx->dmatags->initialstate);
	hash_ctx->cntindescs += 2; /* ensure that we have a free descriptor for padding */

	return SX_OK;
}

int sx_hash_feed(struct sxhash *hash_ctx, const uint8_t *msg, size_t sz)
{
	if (!hash_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (sz == 0) {
		return SX_OK;
	}

	if (hash_ctx->cntindescs >= (ARRAY_SIZE(hash_ctx->descs))) {
		sx_hash_free(hash_ctx);
		return SX_ERR_FEED_COUNT_EXCEEDED;
	}

	if (sz >= DMA_MAX_SZ) {
		sx_hash_free(hash_ctx);
		return SX_ERR_TOO_BIG;
	}

	ADD_RAW_INDESC(hash_ctx->dma, msg, sz, hash_ctx->dmatags->data);
	hash_ctx->feedsz += sz;
	hash_ctx->totalfeedsz += sz;
	hash_ctx->cntindescs++;

	return SX_OK;
}

static int start_hash_hw(struct sxhash *hash_ctx)
{
	sx_cmdma_start(&hash_ctx->dma, sizeof(hash_ctx->descs) + sizeof(hash_ctx->extramem),
		       hash_ctx->descs);

	return 0;
}

int sx_hash_save_state(struct sxhash *hash_ctx)
{
	if (!hash_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (hash_ctx->totalfeedsz % hash_ctx->algo->blocksz) {
		sx_hash_free(hash_ctx);
		return SX_ERR_WRONG_SIZE_GRANULARITY;
	}
	if ((hash_ctx->algo->statesz + hash_ctx->algo->maxpadsz) > sizeof(hash_ctx->extramem)) {
		sx_hash_free(hash_ctx);
		return SX_ERR_ALLOCATION_TOO_SMALL;
	}

	hash_ctx->dma.dmamem.cfg ^= hash_ctx->algo->exportcfg;
	hash_ctx->dma.dmamem.cfg ^= hash_ctx->algo->resumecfg;
	ADD_OUTDESC_PRIV(
		hash_ctx->dma,
		(OFFSET_EXTRAMEM(hash_ctx) + sizeof(hash_ctx->extramem) - hash_ctx->algo->statesz),
		hash_ctx->algo->statesz, CMDMA_BA413_BUS_MSK);

	return start_hash_hw(hash_ctx);
}

int sx_hash_digest(struct sxhash *hash_ctx, uint8_t *digest)
{
	if (!hash_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (hash_ctx->totalfeedsz != hash_ctx->feedsz) {
		hash_ctx->dma.dmamem.cfg ^= hash_ctx->algo->resumecfg;
	}
	hash_ctx->digest(hash_ctx, digest);

	return start_hash_hw(hash_ctx);
}

int sx_hash_status(struct sxhash *hash_ctx)
{
	if (!hash_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	int status;

	status = sx_cmdma_check();
	if (status == SX_ERR_HW_PROCESSING) {
		return status;
	}

#if CONFIG_DCACHE
	sys_cache_data_invd_range((void *)&hash_ctx->extramem, sizeof(hash_ctx->extramem));
#endif

	sx_hash_free(hash_ctx);

	return status;
}

int sx_hash_wait(struct sxhash *hash_ctx)
{
	int status = SX_ERR_HW_PROCESSING;

	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_hash_status(hash_ctx);
	}

	return status;
}

size_t sx_hash_get_alg_digestsz(const struct sxhashalg *alg)
{
	return alg->digestsz;
}

size_t sx_hash_get_alg_blocksz(const struct sxhashalg *alg)
{
	return alg->blocksz;
}

size_t sx_hash_get_digestsz(struct sxhash *hash_ctx)
{
	return hash_ctx->algo->digestsz;
}

size_t sx_hash_get_blocksz(struct sxhash *hash_ctx)
{
	return hash_ctx->algo->blocksz;
}
