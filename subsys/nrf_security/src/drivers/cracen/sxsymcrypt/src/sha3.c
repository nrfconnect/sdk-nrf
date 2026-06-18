/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <cracen/statuscodes.h>
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"

static const struct sx_digesttags ba418tags = {.cfg = DMATAG_BA418 | DMATAG_CONFIG(0),
					       .initialstate = DMATAG_BA418 | DMATAG_DATATYPE(1) |
							       DMATAG_LAST,
					       .data = DMATAG_BA418 | DMATAG_DATATYPE(0)};

/**
 * SHA3/SHAKE padding according to standard sha3 padding scheme described
 * in https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf
 * (section 5.1 and B.2).
 */
static size_t fips202_pad(uint8_t prefix, uint8_t suffix, size_t capacity, size_t msgsz,
			  uint8_t *padding)
{
	size_t r = (1600 / 8) - capacity;
	size_t q;
	size_t i = 0;

	/* The number of bytes to be appended, denoted q, is in {1, 2, ..., r}.
	 */
	q = r - (msgsz % r);

	*padding = prefix;
	for (i = 1; i < q; i++) {
		padding++;
		*padding = 0;
	}
	*padding |= suffix;
	return q;
}

/** Byte to be added at the beginning of the padding in SHA3 mode */
#define SHA3_MODE_PREFIX 0x06

/** Byte to be added at the end of the padding in SHA3 mode */
#define SHA3_MODE_SUFFIX 0x80

/** Byte to be added at the beginning of the padding for SHAKE. */
#define SHAKE_MODE_PREFIX 0x1F

/** Byte to be added at the end of the padding for SHAKE. */
#define SHAKE_MODE_SUFFIX 0x80

/** Bit position of the output-length field in the BA418 cfgword. */
#define SHA3_OUTLEN_SHIFT 8

/*
 * Maximum bytes we ever ask the BA418 to discard via a single DMA descriptor.
 * The DMA size field is 24 bits (DMA_SZ_MASK), so 16 MiB-1 is the hard limit;
 * cap at 64 KiB so one descriptor covers any realistic squeeze chain.
 */
#define XOF_MAX_DISCARD_PER_DESC   (1u << 16)

#define SHA3_SAVE_CONTEXT	   (1 << 6)
#define SHA3_SHAKE_ENABLE	   (1 << 4)
#define SHA3_MODE(x)		   ((x) << 0)
#define SHA3_MODE_SHAKE(x, outlen) ((x) | SHA3_SHAKE_ENABLE | ((outlen) << SHA3_OUTLEN_SHIFT))
#define SHA3_SW_PAD		   0

static void shake_var_digest(struct sxhash *hash_ctx, size_t skip, uint8_t *out, size_t out_len)
{
	uint8_t *padding = (uint8_t *)&hash_ctx->extramem;
	size_t capacity = hash_ctx->algo->statesz - hash_ctx->algo->blocksz;
	size_t total_out = skip + out_len;
	size_t padsz;

	padsz = fips202_pad(SHAKE_MODE_PREFIX, SHAKE_MODE_SUFFIX, capacity,
			    hash_ctx->feedsz, padding);

	ADD_INDESC_PRIV_RAW(hash_ctx->dma, OFFSET_EXTRAMEM(hash_ctx), padsz,
			    hash_ctx->dmatags->data);

	hash_ctx->dma.dmamem.cfg |= ((uint32_t)total_out << SHA3_OUTLEN_SHIFT);

	/* Add DMA DISCARD descriptors to skip the already-emitted prefix. */
	while (skip > 0) {
		size_t discard_bytes = (skip > XOF_MAX_DISCARD_PER_DESC) ?
					XOF_MAX_DISCARD_PER_DESC :
					skip;

		ADD_DISCARDDESC(hash_ctx->dma, discard_bytes);
		skip -= discard_bytes;
	}

	ADD_OUTDESCA(hash_ctx->dma, out, out_len, CMDMA_BA418_BUS_MSK);
}

static void shake_digest(struct sxhash *hash_ctx, size_t skip, uint8_t *digest, size_t digest_sz)
{
	(void)skip;
	(void)digest_sz;

	uint8_t *padding = (uint8_t *)&hash_ctx->extramem;
	int padsz;
	size_t capacity = hash_ctx->algo->statesz - hash_ctx->algo->blocksz;

	padsz = fips202_pad(SHAKE_MODE_PREFIX, SHAKE_MODE_SUFFIX, capacity, hash_ctx->feedsz,
			    padding);

	/* Use ADD_INDESC_PRIV_RAW instead of ADD_INDESC_PRIV.
	 * BA418 hardware cannot work with ADD_INDESC_PRIV as BA418 does not
	 * support byte ignore flags.
	 */
	ADD_INDESC_PRIV_RAW(hash_ctx->dma, OFFSET_EXTRAMEM(hash_ctx), padsz,
			    hash_ctx->dmatags->data);

	ADD_OUTDESCA(hash_ctx->dma, digest, hash_ctx->algo->digestsz, CMDMA_BA413_BUS_MSK);
}

static void sha3_digest(struct sxhash *hash_ctx, size_t skip, uint8_t *digest, size_t digest_sz)
{
	(void)skip;
	(void)digest_sz;

	uint8_t *padding = (uint8_t *)&hash_ctx->extramem;
	int padsz;

	padsz = fips202_pad(SHA3_MODE_PREFIX, SHA3_MODE_SUFFIX, 2 * hash_ctx->algo->digestsz,
			    hash_ctx->feedsz, padding);
	/* Use ADD_INDESC_PRIV_RAW instead of ADD_INDESC_PRIV.
	 * BA418 hardware cannot work with ADD_INDESC_PRIV as BA418 does not
	 * support byte ignore flags.
	 */
	ADD_INDESC_PRIV_RAW(hash_ctx->dma, OFFSET_EXTRAMEM(hash_ctx), padsz,
			    hash_ctx->dmatags->data);

	ADD_OUTDESCA(hash_ctx->dma, digest, hash_ctx->algo->digestsz, CMDMA_BA413_BUS_MSK);
}

static int sx_hash_create_ba418(struct sxhash *hash_ctx, size_t csz)
{
	if ((csz < sizeof(*hash_ctx)) || (hash_ctx->algo->maxpadsz > sizeof(hash_ctx->extramem))) {
		return SX_ERR_ALLOCATION_TOO_SMALL;
	}

	hash_ctx->dmatags = &ba418tags;
	sx_cmdma_newcmd(&hash_ctx->dma, hash_ctx->descs, hash_ctx->algo->cfgword,
			hash_ctx->dmatags->cfg);
	hash_ctx->digest = sha3_digest;
	hash_ctx->feedsz = 0;
	hash_ctx->totalfeedsz = 0;

	hash_ctx->cntindescs = 2; /* reserve 1 extra descriptor for padding */

	return SX_OK;
}

static int sx_hash_create_ba418_shake(struct sxhash *hash_ctx, size_t csz)
{
	int status = sx_hash_create_ba418(hash_ctx, csz);

	if (status != SX_OK) {
		return status;
	}

	hash_ctx->digest = shake_digest;

	return status;
}

static int sx_hash_create_ba418_shake_xof(struct sxhash *hash_ctx, size_t csz)
{
	int status = sx_hash_create_ba418(hash_ctx, csz);

	if (status != SX_OK) {
		return status;
	}

	hash_ctx->digest = shake_var_digest;

	return status;
}

const struct sxhashalg sxhashalg_sha3_224 = {
	SHA3_MODE(6), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, SX_HASH_DIGESTSZ_SHA3_224,
	SX_HASH_BLOCKSZ_SHA3_224, 200, 144, sx_hash_create_ba418
};

const struct sxhashalg sxhashalg_sha3_256 = {
	SHA3_MODE(7), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, SX_HASH_DIGESTSZ_SHA3_256,
	SX_HASH_BLOCKSZ_SHA3_256, 200, 136, sx_hash_create_ba418
};

const struct sxhashalg sxhashalg_sha3_384 = {
	SHA3_MODE(11), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, SX_HASH_DIGESTSZ_SHA3_384,
	SX_HASH_BLOCKSZ_SHA3_384, 200, 104, sx_hash_create_ba418
};

const struct sxhashalg sxhashalg_sha3_512 = {
	SHA3_MODE(15), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, SX_HASH_DIGESTSZ_SHA3_512,
	SX_HASH_BLOCKSZ_SHA3_512, 200, 72, sx_hash_create_ba418
};

const struct sxhashalg sxhashalg_shake256_114 = {
	SHA3_MODE_SHAKE(7, 114), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, 114,
	SX_HASH_BLOCKSZ_SHA3_256, 200, 136, sx_hash_create_ba418_shake
};

const struct sxhashalg sxhashalg_shake256_64 = {
	SHA3_MODE_SHAKE(7, 64), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, 64,
	SX_HASH_BLOCKSZ_SHA3_256, 200, 136, sx_hash_create_ba418_shake
};

/** Variable-output SHAKE-128 XOF. */
const struct sxhashalg sxhashalg_shake128 = {
	SHA3_MODE_SHAKE(3, 0), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, 0,
	SX_HASH_BLOCKSZ_SHAKE_128, 200, 168, sx_hash_create_ba418_shake_xof
};

/** Variable-output SHAKE-256 XOF. */
const struct sxhashalg sxhashalg_shake256 = {
	SHA3_MODE_SHAKE(7, 0), SHA3_SW_PAD, SHA3_SAVE_CONTEXT, 0,
	SX_HASH_BLOCKSZ_SHA3_256, 200, 136, sx_hash_create_ba418_shake_xof
};

bool sx_is_shake_alg(const struct sxhashalg *hashalg)
{
	return ((hashalg->cfgword ^ SHA3_MODE_SHAKE(7, 0)) == 0 ||
		(hashalg->cfgword ^ SHA3_MODE_SHAKE(3, 0)) == 0);
}
