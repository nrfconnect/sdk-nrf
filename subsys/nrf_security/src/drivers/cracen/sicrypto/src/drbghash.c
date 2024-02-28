/* DRBG Hash, based on NIST SP 800-90A Rev. 1.
 *
 * Copyright (c) 2020-2021 Silex Insight
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout in a DRBG Hash task:
 *      1. hash_df internal counter (size: 1 byte).
 *      2. hash_df argument "no_of_bits_to_return" (seedlen) (size: 4 bytes).
 *      3. constant values (0, 1 and 3) that need to be prepended to V before
 *         hash operations at different points of the algorithm (size: 1 byte).
 *      4. V (size: digestsz*ceil(seedlen/digestsz) bytes). This area reserved
 *         for V is oversized to simplify the hash computations in hash_df. Only
 *         the first seedlen bytes of it are meaningful.
 *      5. C (size: digestsz*ceil(seedlen/digestsz) bytes). This area reserved
 *         for C is oversized to simplify the hash computations in hash_df. Only
 *         the first seedlen bytes of it are meaningful.
 *      6. "work area" used for various intermediate values (size: seedlen
 *          bytes).
 *
 * The total size required for workmem depends on the hash algorithm choice:
 *      SHA-1                       181 bytes (6 + 60 + 60 + 55)
 *      SHA-224 and SHA-512/224     173 bytes (6 + 56 + 56 + 55)
 *      SHA-256 and SHA-512/256     189 bytes (6 + 64 + 64 + 55)
 *      SHA-384                     405 bytes (6 + 144 + 144 + 111)
 *      SHA-512                     373 bytes (6 + 128 + 128 + 111)
 *
 * In the formulas above, the numbers 60, 56, 64, 144 and 128 are the values of
 * digestsz*ceil(seedlen/digestsz), i.e. seedlen rounded up to the next multiple
 * of digestsz.
 *
 * Notes
 * - Supported hash algorithms: SHA1, SHA-224, SHA-512/224, SHA-256,
 *   SHA-512/256, SHA-384 and SHA-512.
 * - The "additional input" defined in NIST SP 800-90A (used in the generate and
 *   the reseed processes) is not supported.
 */

#include <string.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/drbghash.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "waitqueue.h"
#include "final.h"
#include "util.h"

#define SMALL_SEEDLEN_SZ	  55
#define BIG_SEEDLEN_SZ		  111
#define AUX_BYTES		  6
#define RESEED_INTERVAL		  ((uint64_t)1 << 48) /* 2^48 as per NIST spec */
#define MAX_REQUEST_SZ		  (1 << 16)	      /* 2^19 bits as per NIST spec */
#define NOT_INSTANTIATED	  0
#define INSTANTIATION_IN_PROGRESS 1
#define INSTANTIATED		  2

static int install_drbg_functions(struct sitask *t, struct siwq *wq);
static int hashdf_c_start(struct sitask *t, struct siwq *wq);
static void consume_reseed(struct sitask *t, const char *msg, size_t sz);

/* Compute v = v + summand
 * - v is an unsigned integer stored as a big endian byte array of sz bytes
 * - summand must be <= than the maximum value of a uint64_t minus 255
 * - final carry is discarded: addition is modulo 2^(sz*8)
 */
static void be_add_uint64(unsigned char *v, size_t sz, uint64_t summand)
{
	for (; sz > 0;) {
		sz--;
		summand += v[sz];
		v[sz] = summand & 0xFF;
		summand >>= 8;
	}
}

/* Add two unsigned integers stored as big endian byte arrays (a = a + b);
 * - parameter a is used as both input and output
 * - bsz must be <= asz
 * - final carry is discarded: addition is modulo 2^(asz*8)
 * - time constant implementation
 */
static void be_array_add(unsigned char *a, size_t asz, const unsigned char *b, size_t bsz)
{
	unsigned int tmp = 0;
	size_t i;

	for (i = 0; i < bsz; i++) {
		tmp += a[asz - 1 - i] + b[bsz - 1 - i];
		a[asz - 1 - i] = tmp & 0xFF;
		tmp >>= 8;
	}

	for (; i < asz; i++) {
		tmp += a[asz - 1 - i];
		a[asz - 1 - i] = tmp & 0xFF;
		tmp >>= 8;
	}
}

/* Return the maximum security strength, in bytes, that a DRBG Hash can support,
 * as a function of the selected hash algorithm. Based on NIST SP 800-57 Part 1
 * Rev. 5, table 3.
 */
static size_t get_security_strength(struct sitask *t)
{
	switch (sx_hash_get_alg_digestsz(t->hashalg)) {
	case 20: /* SHA1 */
		return 16;
	case 28: /* SHA-224 or SHA-512/224 */
		return 24;
	case 32: /* SHA-256 or SHA-512/256 */
	case 48: /* SHA-384 */
	case 64: /* SHA-512 */
		return 32;
	default:
		return 0;
	}
}

/* Update V at the end of the generate process. */
static int drbghash_gen_update_v(struct sitask *t, struct siwq *wq)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	unsigned char *vbase = (unsigned char *)(t->workmem + AUX_BYTES);
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	unsigned char *cbase = vbase + (drbgpars->hashdf_iterations) * outlen;
	unsigned char *workarea = cbase + (drbgpars->hashdf_iterations) * outlen;
	(void)wq;

	/* compute V = (V + H + C + reseed_counter) mod 2^seedlen_bits */
	be_array_add(vbase, drbgpars->seedlen, workarea, outlen);
	be_array_add(vbase, drbgpars->seedlen, cbase, drbgpars->seedlen);

	/* it is safe to use be_add_uint64() because drbgpars->reseed_ctr cannot
	 * be greater than RESEED_INTERVAL, so it will always be <= than the maximum
	 * value of a uint64_t minus 255
	 */
	be_add_uint64(vbase, drbgpars->seedlen, drbgpars->reseed_ctr);

	drbgpars->reseed_ctr++;
	return install_drbg_functions(t, NULL);
}

/* Compute H at the end of the generate process. */
static int drbghash_gen_compute_h(struct sitask *t)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = t->workmem + AUX_BYTES + 2 * (drbgpars->hashdf_iterations) * outlen;

	t->workmem[5] = 3;

	/* hash task to compute H = Hash(0x03 || V), with result in workarea */
	si_hash_create(t, t->hashalg);
	si_task_consume(t, &(t->workmem[5]), 1 + drbgpars->seedlen);
	si_task_produce(t, workarea, outlen);

	si_wq_run_after(t, &drbgpars->wq, drbghash_gen_update_v);
	return SX_ERR_HW_PROCESSING;
}

static int hashgen_finish(struct sitask *t, struct siwq *wq)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = t->workmem + AUX_BYTES + 2 * (drbgpars->hashdf_iterations) * outlen;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* copy the right amount of bytes to the user output buffer */
	memcpy(t->output, workarea, drbgpars->outsz);
	return drbghash_gen_compute_h(t);
}

static int hashgen_step(struct sitask *t);

/* Increment workarea, then perform a hashgen step. */
static int hashgen_step_increment(struct sitask *t, struct siwq *wq)
{
	unsigned char one = 1;
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = t->workmem + AUX_BYTES + 2 * (drbgpars->hashdf_iterations) * outlen;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* compute workarea = (workarea + 1) mod 2^seedlen */
	be_array_add((unsigned char *)workarea, drbgpars->seedlen, &one, 1);
	return hashgen_step(t);
}

static int hashgen_step(struct sitask *t)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = t->workmem + AUX_BYTES + 2 * (drbgpars->hashdf_iterations) * outlen;

	if (drbgpars->outsz == 0) {
		return drbghash_gen_compute_h(t);
	}

	/* create and feed hash task */
	si_hash_create(t, t->hashalg);
	si_task_consume(t, workarea, drbgpars->seedlen);

	if (drbgpars->outsz >= outlen) {
		drbgpars->outsz -= outlen;

		/* result written directly to user's output buffer */
		si_task_produce(t, t->output, outlen);

		t->output += outlen;
		si_wq_run_after(t, &drbgpars->wq, hashgen_step_increment);
	} else {
		/* there are still some bytes to produce, but less than outlen
		 */
		si_task_produce(t, workarea, outlen);
		si_wq_run_after(t, &drbgpars->wq, hashgen_finish);
	}

	return SX_ERR_HW_PROCESSING;
}

static void drbghash_produce_start(struct sitask *t, char *out, size_t sz)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	char *vbase = t->workmem + AUX_BYTES;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = vbase + 2 * (drbgpars->hashdf_iterations) * outlen;

	if (drbgpars->reseed_ctr > RESEED_INTERVAL) {
		si_task_mark_final(t, SX_ERR_RESEED_NEEDED);
		t->actions.consume = consume_reseed;
		return;
	}

	if (sz > MAX_REQUEST_SZ) {
		si_task_mark_final(t, SX_ERR_INVALID_REQ_SZ);
		return;
	}

	/* store output pointer and requested number of random bytes */
	t->output = out;
	drbgpars->outsz = sz;

	/* copy V in "work area" of workmem */
	memcpy(workarea, vbase, drbgpars->seedlen);

	hashgen_step(t);
}

/* Internal iteration of hash_df, when computing V for instantiate or reseed. */
static int hashdf_v(struct sitask *t, struct siwq *wq)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	char *vbase = t->workmem + AUX_BYTES;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = vbase + 2 * (drbgpars->hashdf_iterations) * outlen;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* increment hash_df internal counter */
	t->workmem[0]++;

	/* create hash task */
	si_hash_create(t, t->hashalg);

	/* consume operations differ for reseeding and instantiation */
	if (drbgpars->progress == INSTANTIATED) {
		/* feed preamble, V and entropy */
		si_task_consume(t, t->workmem, AUX_BYTES);
		si_task_consume(t, workarea, drbgpars->seedlen);
		si_task_consume(t, drbgpars->entropy, drbgpars->entropysz);
	} else {
		/* feed preamble, entropy, nonce and personalization string */
		si_task_consume(t, t->workmem, 5);
		si_task_consume(t, drbgpars->entropy, drbgpars->entropysz);
		si_task_consume(t, drbgpars->nonce, drbgpars->noncesz);
		si_task_consume(t, drbgpars->pers, drbgpars->perssz);
	}

	/* run the hash task */
	si_task_produce(t, t->output, outlen);

	t->output += outlen;

	/* move on to compute C if all internal hash_df iterations have been
	 * done
	 */
	if (((int)t->workmem[0]) == drbgpars->hashdf_iterations) {
		si_wq_run_after(t, &drbgpars->wq, hashdf_c_start);
	} else {
		si_wq_run_after(t, &drbgpars->wq, hashdf_v);
	}

	return SX_ERR_HW_PROCESSING;
}

/* Set some initial values then call hashdf_v() during instantiate or reseed. */
static void hashdf_v_start(struct sitask *t)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	char *vbase = t->workmem + AUX_BYTES;
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	char *workarea = vbase + 2 * (drbgpars->hashdf_iterations) * outlen;

	t->workmem[0] = 0; /* hash_df internal counter */
	t->workmem[5] = 1; /* used when computing V during a reseed */
	t->output = vbase;

	/* if this is a reseeding, copy V in "work area" of workmem */
	if (drbgpars->progress == INSTANTIATED) {
		memcpy(workarea, vbase, drbgpars->seedlen);
	}

	/* override state SX_ERR_HW_PROCESSING to be able to run hashdf_v() */
	t->statuscode = SX_OK;

	hashdf_v(t, NULL);
}

static void consume_reseed(struct sitask *t, const char *msg, size_t sz)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;

	/* check that entropy input has valid size */
	if (sz < get_security_strength(t)) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* save entropy pointer and size into DRBG parameters */
	drbgpars->entropy = msg;
	drbgpars->entropysz = sz;

	/* if reseeding at the end of seed life */
	if (t->statuscode == SX_ERR_RESEED_NEEDED) {
		t->statuscode = SX_ERR_READY;
	}

	drbgpars->reseed_ctr = 1;

	/* install and uninstall functions */
	t->actions.run = hashdf_v_start;
	t->actions.consume = NULL;
}

/* Install the actions that are allowed after the instantiation is complete:
 * produce (to generate random bytes) and consume (to reseed).
 */
static int install_drbg_functions(struct sitask *t, struct siwq *wq)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	if (drbgpars->progress == INSTANTIATION_IN_PROGRESS) {
		drbgpars->progress = INSTANTIATED;
	}

	t->actions.produce = drbghash_produce_start;
	t->actions.consume = consume_reseed;

	t->statuscode = SX_ERR_READY;

	return SX_ERR_READY;
}

/* Internal iteration of hash_df, when computing C. */
static int hashdf_c(struct sitask *t, struct siwq *wq)
{
	size_t outlen = sx_hash_get_alg_digestsz(t->hashalg);
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	t->workmem[0]++;

	/* create hash task, feed preamble and V, then run it */
	si_hash_create(t, t->hashalg);
	si_task_consume(t, t->workmem, AUX_BYTES + drbgpars->seedlen);
	si_task_produce(t, t->output, outlen);

	t->output += outlen;

	/* stop when all internal hash_df iterations have been done */
	if (((int)t->workmem[0]) == drbgpars->hashdf_iterations) {
		/* a last step is needed to set the DRBG produce function */
		si_wq_run_after(t, &drbgpars->wq, install_drbg_functions);
	} else {
		si_wq_run_after(t, &drbgpars->wq, hashdf_c);
	}

	return SX_ERR_HW_PROCESSING;
}

/* Set some initial values then call hashdf_c() during instantiate or reseed. */
static int hashdf_c_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	t->workmem[0] = 0; /* hash_df internal counter */
	t->workmem[5] = 0;

	return hashdf_c(t, NULL);
}

static void consume_pers_string(struct sitask *t, const char *msg, size_t sz)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;

	/* save personalization string pointer and size into DRBG parameters */
	drbgpars->pers = msg;
	drbgpars->perssz = sz;

	/* uninstall consume function */
	t->actions.consume = NULL;
}

static void consume_nonce(struct sitask *t, const char *msg, size_t sz)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;

	/* save nonce pointer and size into DRBG parameters */
	drbgpars->nonce = msg;
	drbgpars->noncesz = sz;

	t->actions.consume = consume_pers_string;
}

/* Returns the minimum required workmem size for a DRBG Hash task, computed with
 * the formula: 6 + 2*digestsz*ceiling(seedsz/digestsz) + seedsz.
 */
static size_t minworkmemsz(size_t digestsz)
{
	size_t seedsz, ceil, workmemsz;

	seedsz = (digestsz <= 32) ? 55 : 111;
	ceil = (seedsz + digestsz - 1) / digestsz;
	workmemsz = 6 + 2 * digestsz * ceil + seedsz;

	return workmemsz;
}

void si_prng_create_drbg_hash(struct sitask *t, const struct sxhashalg *hashalg,
			      const char *entropy, size_t entropysz)
{
	struct siparams_drbghash *drbgpars = &t->params.drbghash;
	size_t security_strength, digestsz;

	t->statuscode = SX_ERR_READY;
	t->actions = (struct siactions){0};
	t->actions.consume = consume_nonce;
	t->actions.run = hashdf_v_start;

	t->hashalg = hashalg;

	security_strength = get_security_strength(t);
	if (security_strength == 0) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* check that entropy input has valid size */
	if (entropysz < security_strength) {
		si_task_mark_final(t, SX_ERR_INSUFFICIENT_ENTROPY);
		return;
	}

	digestsz = sx_hash_get_alg_digestsz(hashalg);
	if (t->workmemsz < minworkmemsz(digestsz)) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* the hash digest size is what NIST.SP.800-90Ar1 calls outlen */
	switch (digestsz) {
	case 20: /* SHA1 */
		drbgpars->seedlen = SMALL_SEEDLEN_SZ;
		drbgpars->hashdf_iterations = 3;
		break;
	case 28: /* SHA-224 or SHA-512/224 */
	case 32: /* SHA-256 or SHA-512/256 */
		drbgpars->seedlen = SMALL_SEEDLEN_SZ;
		drbgpars->hashdf_iterations = 2;
		break;
	case 48: /* SHA-384 */
		drbgpars->seedlen = BIG_SEEDLEN_SZ;
		drbgpars->hashdf_iterations = 3;
		break;
	case 64: /* SHA-512 */
		drbgpars->seedlen = BIG_SEEDLEN_SZ;
		drbgpars->hashdf_iterations = 2;
		break;
	default:
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	drbgpars->progress = INSTANTIATION_IN_PROGRESS;
	drbgpars->entropy = entropy;
	drbgpars->entropysz = entropysz;
	drbgpars->pers = NULL;
	drbgpars->perssz = 0;
	drbgpars->nonce = NULL;
	drbgpars->noncesz = 0;
	drbgpars->reseed_ctr = 1;

	/* Convert seedlen from bytes to bits and write that value to workmem
	 * (bytes 1 to 4, big endian). These bytes serve as the "no_of_bits_to_return"
	 * argument to hash_df.
	 */
	uint32_t tmp = drbgpars->seedlen << 3;

	t->workmem[1] = (tmp >> 24) & 0xFF;
	t->workmem[2] = (tmp >> 16) & 0xFF;
	t->workmem[3] = (tmp >> 8) & 0xFF;
	t->workmem[4] = tmp & 0xFF;
}
