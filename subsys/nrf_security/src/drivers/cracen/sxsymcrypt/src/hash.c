/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "../include/sxsymcrypt/hash.h"
#include <cracen/statuscodes.h>
#include "../include/sxsymcrypt/internal.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "hashdefs.h"

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

static int sx_hash_create_ba413(struct sxhash *c, size_t csz);

const struct sxhashalg sxhashalg_sha1 = {
	CMDMA_BA413_MODE(0x02), HASH_HW_PAD, HASH_FINAL, 20, 64, 20, 72, sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_224 = {
	CMDMA_BA413_MODE(0x04), HASH_HW_PAD, HASH_FINAL, 28, 64, 32, 72, sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_256 = {
	CMDMA_BA413_MODE(0x08), HASH_HW_PAD, HASH_FINAL, 32, 64, 32, 72, sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_384 = {
	CMDMA_BA413_MODE(0x10), HASH_HW_PAD, HASH_FINAL, 48, 128, 64, 80, sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sha2_512 = {
	CMDMA_BA413_MODE(0x20), HASH_HW_PAD, HASH_FINAL, 64, 128, 64, 144, sx_hash_create_ba413};
const struct sxhashalg sxhashalg_sm3 = {
	CMDMA_BA413_MODE(0x40), HASH_HW_PAD, HASH_FINAL, 32, 64, 32, 72, sx_hash_create_ba413};

#define CMDMA_BA413_BUS_MSK 3

void sx_hash_free(struct sxhash *c)
{
	sx_cmdma_release_hw(&c->dma);
}

static void sx_hash_pad(struct sxhash *c)
{
	unsigned char *padding = (unsigned char *)c->extramem;
	size_t padsz;
	size_t v = c->totalsz;

	/* as per SHA standard, write the full hashed message size in
	 * big endian form starting with 0x80 and as many 0 bytes as
	 * needed to have full block size.
	 */
	size_t length_field_sz = (c->algo->digestsz >= 48) ? 16 : 8;

	padsz = c->algo->blocksz - (c->totalsz & (c->algo->blocksz - 1));
	if (padsz < (length_field_sz + 1)) {
		padsz += c->algo->blocksz;
	}

	padding[padsz - 1] = (v & 0x1f) << 3; /* multiply by 8 for bits */
	v >>= 5;
	for (size_t i = padsz - 2; i > 0; i--) {
		padding[i] = v & 0xFF;
		v >>= 8;
	}
	padding[0] = 0x80;

	ADD_INDESC_PRIV_RAW(c->dma, OFFSET_EXTRAMEM(c), padsz, c->dmatags->data);
	c->feedsz += padsz;
	/* Padding is done only when sx_hash_resume_state() has been called and
	 * sx_hash_resume_state() already reserved room for this extra
	 * descriptor.
	 */
}

void sx_ba413_digest(struct sxhash *c, char *digest)
{
	if (c->totalsz == 0) {
		/* If we get here it means an empty message hash will be
		 * generated. 4 dummy bytes will be added to hash, these bytes
		 * will be dropped by the hash engine.
		 */
		ADD_EMPTY_INDESC(c->dma, HASH_INVALID_BYTES, c->dmatags->data);
	}

	if (!(c->dma.dmamem.cfg & HASH_HW_PAD)) {
		sx_hash_pad(c);
	}
	SET_LAST_DESC_IGN(c->dma.d - 1, c->feedsz, CMDMA_BA413_BUS_MSK);

	struct sxdesc *out = c->dma.dmamem.outdescs;

	ADD_OUTDESCA(out, digest, c->algo->digestsz, CMDMA_BA413_BUS_MSK);

	if (!(c->dma.dmamem.cfg & HASH_FINAL) && (c->algo->statesz - c->algo->digestsz)) {
		ADD_DISCARDDESC(out, (c->algo->statesz - c->algo->digestsz));
	}
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, out - 1);
}

static int sx_hash_create_ba413(struct sxhash *c, size_t csz)
{
	if (csz < sizeof(*c)) {
		return SX_ERR_ALLOCATION_TOO_SMALL;
	}

	/* SM3 is not supported by CRACEN in nRF devices */
	if (c->algo->cfgword & REG_BA413_CAPS_ALGO_SM3) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	sx_hw_reserve(&c->dma);

	c->dmatags = &ba413tags;
	sx_cmdma_newcmd(&c->dma, c->allindescs, c->algo->cfgword, c->dmatags->cfg);
	c->feedsz = 0;
	c->totalsz = 0;
	c->digest = sx_ba413_digest;

	c->cntindescs = 1;

	return SX_OK;
}

int sx_hash_create_sha256(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sha2_256;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create_sha384(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sha2_384;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create_sha512(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sha2_512;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create_sha1(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sha1;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create_sha224(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sha2_224;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create_sm3(struct sxhash *c, size_t csz)
{
	c->algo = &sxhashalg_sm3;

	return sx_hash_create_ba413(c, csz);
}

int sx_hash_create(struct sxhash *c, const struct sxhashalg *alg, size_t csz)
{
	c->algo = alg;

	return alg->reservehw(c, csz);
}

int sx_hash_resume_state(struct sxhash *c)
{
	if (!c->algo || c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	size_t totalsz = c->totalsz;
	int r = c->algo->reservehw(c, sizeof(*c));

	if (r) {
		return r;
	}
	c->totalsz = totalsz;
	ADD_INDESC_PRIV(c->dma, (OFFSET_EXTRAMEM(c) + sizeof(c->extramem) - c->algo->statesz),
			c->algo->statesz, c->dmatags->initialstate);
	c->cntindescs += 2; /* ensure that we have a free descriptor for padding */

	return SX_OK;
}

int sx_hash_feed(struct sxhash *c, const char *msg, size_t sz)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (sz == 0) {
		return SX_OK;
	}

	if (c->cntindescs >= (ARRAY_SIZE(c->allindescs))) {
		sx_hash_free(c);
		return SX_ERR_FEED_COUNT_EXCEEDED;
	}

	if (sz >= DMA_MAX_SZ) {
		sx_hash_free(c);
		return SX_ERR_TOO_BIG;
	}

	ADD_RAW_INDESC(c->dma, msg, sz, c->dmatags->data);
	c->feedsz += sz;
	c->totalsz += sz;
	c->cntindescs++;

	return SX_OK;
}

static int start_hash_hw(struct sxhash *c)
{
	sx_cmdma_finalize_descs(c->allindescs, c->dma.d - 1);
	sx_cmdma_start(&c->dma, sizeof(c->allindescs) + sizeof(c->extramem), c->allindescs);

	return 0;
}

int sx_hash_save_state(struct sxhash *c)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (c->totalsz % c->algo->blocksz) {
		sx_hash_free(c);
		return SX_ERR_WRONG_SIZE_GRANULARITY;
	}
	if ((c->algo->statesz + c->algo->maxpadsz) > sizeof(c->extramem)) {
		sx_hash_free(c);
		return SX_ERR_ALLOCATION_TOO_SMALL;
	}

	struct sxdesc *out = c->dma.dmamem.outdescs;

	c->dma.dmamem.cfg ^= c->algo->exportcfg;
	c->dma.dmamem.cfg ^= c->algo->resumecfg;
	ADD_OUTDESC_PRIV(c->dma, out, (OFFSET_EXTRAMEM(c) + sizeof(c->extramem) - c->algo->statesz),
			 c->algo->statesz, CMDMA_BA413_BUS_MSK);
	sx_cmdma_finalize_descs(c->dma.dmamem.outdescs, out - 1);

	return start_hash_hw(c);
}

int sx_hash_digest(struct sxhash *c, char *digest)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (c->totalsz != c->feedsz) {
		c->dma.dmamem.cfg ^= c->algo->resumecfg;
	}
	c->digest(c, digest);

	return start_hash_hw(c);
}

int sx_hash_status(struct sxhash *c)
{
	if (!c->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	int r;

	r = sx_cmdma_check();
	if (r == SX_ERR_HW_PROCESSING) {
		return r;
	}

	sx_hash_free(c);

	return r;
}

int sx_hash_wait(struct sxhash *c)
{
	int r = SX_ERR_HW_PROCESSING;

	while (r == SX_ERR_HW_PROCESSING) {
		r = sx_hash_status(c);
	}

	return r;
}

size_t sx_hash_get_alg_digestsz(const struct sxhashalg *alg)
{
	return alg->digestsz;
}

size_t sx_hash_get_alg_blocksz(const struct sxhashalg *alg)
{
	return alg->blocksz;
}

size_t sx_hash_get_digestsz(struct sxhash *c)
{
	return c->algo->digestsz;
}

size_t sx_hash_get_blocksz(struct sxhash *c)
{
	return c->algo->blocksz;
}
