/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HASHDEFS_HEADER_FILE
#define HASHDEFS_HEADER_FILE

#include <stddef.h>
#include <stdint.h>
#include "internal.h"

#ifndef SX_HASH_PRIV_SZ
#define SX_HASH_PRIV_SZ 344
#endif

struct sx_digesttags {
	uint32_t cfg;
	uint32_t initialstate;
	uint32_t keycfg;
	uint32_t data;
};

/** A hash operation.
 *
 * To be used with sx_hash_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxhash {
	const struct sxhashalg *algo;
	const struct sx_digesttags *dmatags;
	uint32_t cntindescs;
	size_t totalfeedsz;
	uint32_t feedsz;
	void (*digest)(struct sxhash *c, char *digest);
	struct sx_dmactl dma;
	struct sxdesc descs[7 + SX_EXTRA_IN_DESCS];
	uint8_t extramem[SX_HASH_PRIV_SZ];
};

struct sxhashalg {
	uint32_t cfgword;
	uint32_t resumecfg;
	uint32_t exportcfg;
	size_t digestsz;
	size_t blocksz;
	size_t statesz;
	size_t maxpadsz;
	int (*reservehw)(struct sxhash *c, size_t csz);
};

#define SX_HASH_DIGESTSZ_SHA1 20
#define SX_HASH_BLOCKSZ_SHA1 64
#define SX_HASH_DIGESTSZ_SHA2_224 28
#define SX_HASH_DIGESTSZ_SHA2_256 32
#define SX_HASH_DIGESTSZ_SHA2_384 48
#define SX_HASH_DIGESTSZ_SHA2_512 64
#define SX_HASH_BLOCKSZ_SHA2_224 64
#define SX_HASH_BLOCKSZ_SHA2_256 64
#define SX_HASH_BLOCKSZ_SHA2_384 128
#define SX_HASH_BLOCKSZ_SHA2_512 128
#define SX_HASH_DIGESTSZ_SHA3_224 28
#define SX_HASH_DIGESTSZ_SHA3_256 32
#define SX_HASH_DIGESTSZ_SHA3_384 48
#define SX_HASH_DIGESTSZ_SHA3_512 64
#define SX_HASH_BLOCKSZ_SHA3_224 144
#define SX_HASH_BLOCKSZ_SHA3_256 136
#define SX_HASH_BLOCKSZ_SHA3_384 104
#define SX_HASH_BLOCKSZ_SHA3_512 72
#define SX_HASH_DIGESTSZ_SM3 32
#define SX_HASH_BLOCKSZ_SM3 64

/** Hash algorithm SHA-1 (Secure Hash Algorithm 1)
 *
 * Deprecated algorithm. NIST formally deprecated use of SHA-1 in 2011
 * and disallowed its use for digital signatures in 2013. SHA-3 or SHA-2
 * are recommended instead.
 */
extern const struct sxhashalg sxhashalg_sha1;

/** Hash algorithm SHA-2 224
 *
 * Has only 32 bit capacity against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_224;

/** Hash algorithm SHA-2 256
 *
 * Has no resistance against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_256;

/** Hash algorithm SHA-2 384
 *
 * Has 128 bit capacity against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_384;

/** Hash algorithm SHA-2 512
 *
 * Has no resistance against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_512;

/** Hash algorithm SHA-3 224 */
extern const struct sxhashalg sxhashalg_sha3_224;

/** Hash algorithm SHA-3 256 */
extern const struct sxhashalg sxhashalg_sha3_256;

/** Hash algorithm SHA-3 384 */
extern const struct sxhashalg sxhashalg_sha3_384;

/** Hash algorithm SHA-3 512*/
extern const struct sxhashalg sxhashalg_sha3_512;

/** Hash algorithm SHAKE256, with output size fixed to 114 bytes (for Ed448). */
extern const struct sxhashalg sxhashalg_shake256_114;

/** GM/T 0004-2012: SM3 cryptographic hash algorithm */
extern const struct sxhashalg sxhashalg_sm3;

#define CMDMA_BA413_BUS_MSK 3
#define HASH_INVALID_BYTES  4 /* number of invalid bytes when empty message, required by HW */
#define OFFSET_EXTRAMEM(c)  (sizeof((c)->dma.dmamem) + sizeof((c)->descs))

#define CMDMA_BA418_BUS_MSK (3)

#endif
