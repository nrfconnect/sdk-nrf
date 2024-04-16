/* PBKDF2 key derivation function with HMAC as PRF, based on RFC 8018.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the PBKDF2 task:
 *      1. HMAC sub-task workmem (size: hash block size + hash digest size).
 *      2. outer loop counter (size: 4 bytes).
 *      3. T_i (size: hash digest size).
 *      4. U_j (size: hash digest size).
 *      5. password hash (size: hash digest size). This area is not used if the
 *         password does not need to be hashed (its size is less than the hash
 *         block size).
 *
 * The workmem required size can be computed with:
 *      4 + 4*hash_digest_size + hash_block_size
 *
 * Some example values are:
 *      workmem size for PBKDF2 with HMAC-SHA1:      148 bytes
 *      workmem size for PBKDF2 with HMAC-SHA2-256:  196 bytes
 *      workmem size for PBKDF2 with HMAC-SHA2-512:  388 bytes
 *
 * Implementation notes
 * - we do not check that dklen <= (2^32 - 1)*hLen (see RFC 8018 section 5.2).
 *   In fact, dkLen is provided by the user as a size_t argument: performing the
 *   check would make sense only if the width of the size_t type is more than 32
 *   bits, however we cannot assume that is the case.
 */

#include <string.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/pbkdf2.h"
#include "../include/sicrypto/hmac.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include "waitqueue.h"
#include "final.h"
#include "util.h"

#define PBKDF2_COUNTER_SZ 4

/* Size of the workmem of the HMAC subtask. */
#define HMAC_WORKMEM_SZ(digestsz, blocksz) ((digestsz) + (blocksz))

static int pbkdf2_innerloop_continue(struct sitask *t, struct siwq *wq);
static int pbkdf2_produce_continue(struct sitask *t);

/* Return a pointer to the part of workmem that is specific to 'pbkdf2'. */
static inline char *get_workmem_pointer(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	return t->workmem + HMAC_WORKMEM_SZ(digestsz, blocksz);
}

static inline size_t get_workmem_size(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	return t->workmemsz - HMAC_WORKMEM_SZ(digestsz, blocksz);
}

/* Complete the last iteration of the inner loop */
static int pbkdf2_innerloop_finish(struct sitask *t, struct siwq *wq)
{
	size_t copysz;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *Ti = get_workmem_pointer(t) + PBKDF2_COUNTER_SZ;
	(void)wq;

	/* an output block is ready, copy bytes to the user's output buffer */
	copysz = (t->params.pbkdf2.outsz >= digestsz) ? digestsz : t->params.pbkdf2.outsz;

	memcpy(t->params.pbkdf2.out, Ti, copysz);

	t->params.pbkdf2.out += copysz;
	t->params.pbkdf2.outsz -= copysz;

	if (t->params.pbkdf2.outsz) {
		/* perform another outer loop iteration */
		return pbkdf2_produce_continue(t);
	}

	/* end of outer loop, hence of the task */
	return si_task_mark_final(t, SX_OK);
}

static int pbkdf2_innerloop_continue(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *Ti = get_workmem_pointer(t) + PBKDF2_COUNTER_SZ;
	char *Uj = Ti + digestsz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return si_task_mark_final(t, t->statuscode);
	}

	/* compute T_i = T_i ^ U_j */
	si_xorbytes(Ti, Uj, digestsz);

	t->params.pbkdf2.iter_ctr++;

	if (t->params.pbkdf2.iter_ctr < t->params.pbkdf2.iterations) {
		si_wq_run_after(t, &t->params.pbkdf2.wq, pbkdf2_innerloop_continue);

		si_mac_create_hmac(t, t->hashalg, t->params.pbkdf2.pswd, t->params.pbkdf2.pswdsz);
		si_task_consume(t, Uj, digestsz);
		si_task_produce(t, Uj, digestsz);

		return SX_ERR_HW_PROCESSING;
	} else {
		return pbkdf2_innerloop_finish(t, NULL);
	}
}

/* Perform an outer loop iteration and the first inner loop iteration. The
 * pbkdf2_workmem argument must point to the workmem of the PBKDF2 task.
 */
static int pbkdf2_outer_loop(struct sitask *t, char *pbkdf2_workmem)
{
	char *outer_ctr = pbkdf2_workmem;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *Uj = pbkdf2_workmem + PBKDF2_COUNTER_SZ + digestsz;
	const size_t pbkdf2_workmem_size = get_workmem_size(t);

	/* reset inner loop counter */
	t->params.pbkdf2.iter_ctr = 0;

	/* initialize T_i to zero */
	safe_memset(pbkdf2_workmem + PBKDF2_COUNTER_SZ, pbkdf2_workmem_size - PBKDF2_COUNTER_SZ, 0,
		    digestsz);

	si_wq_run_after(t, &t->params.pbkdf2.wq, pbkdf2_innerloop_continue);

	si_mac_create_hmac(t, t->hashalg, t->params.pbkdf2.pswd, t->params.pbkdf2.pswdsz);

	si_task_consume(t, t->params.pbkdf2.salt, t->params.pbkdf2.saltsz);
	si_task_consume(t, outer_ctr, PBKDF2_COUNTER_SZ);

	/* HMAC output written to the U_j part of workmem */
	si_task_produce(t, Uj, digestsz);

	return SX_ERR_HW_PROCESSING;
}

static int pbkdf2_produce_continue(struct sitask *t)
{
	char *pbkdf2mem = get_workmem_pointer(t);
	char *outer_ctr = pbkdf2mem;

	/* increment the outer loop counter */
	si_be_add((unsigned char *)outer_ctr, PBKDF2_COUNTER_SZ, 1);

	return pbkdf2_outer_loop(t, pbkdf2mem);
}

/* The sz argument is the length in bytes of the output keying material */
static void pbkdf2_produce_start(struct sitask *t, char *out, size_t sz)
{
	char *wmem = get_workmem_pointer(t);

	if (sz == 0) {
		si_task_mark_final(t, SX_OK);
		return;
	}

	t->params.pbkdf2.out = out;
	t->params.pbkdf2.outsz = sz;

	/* initialize outer loop counter to 1, big endian */
	wmem[0] = 0;
	wmem[1] = 0;
	wmem[2] = 0;
	wmem[3] = 1;

	pbkdf2_outer_loop(t, wmem);
}

static int pbkdf2_setup_prod(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	t->actions.produce = pbkdf2_produce_start;
	t->statuscode = SX_ERR_READY;

	return SX_ERR_READY;
}

void si_kdf_create_pbkdf2(struct sitask *t, const struct sxhashalg *hashalg, const char *password,
			  size_t passwordsz, const char *salt, size_t saltsz, size_t iterations)
{
	size_t digestsz, blocksz;
	char *pswd_hash;

	/* iteration count must be a positive integer */
	if (iterations == 0) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* get the hash digest size and block size */
	digestsz = sx_hash_get_alg_digestsz(hashalg);
	blocksz = sx_hash_get_alg_blocksz(hashalg);

	if (t->workmemsz < 4 + 4 * digestsz + blocksz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->hashalg = hashalg;
	t->params.pbkdf2.iterations = iterations;
	t->params.pbkdf2.pswd = password;
	t->params.pbkdf2.pswdsz = passwordsz;
	t->params.pbkdf2.salt = salt;
	t->params.pbkdf2.saltsz = saltsz;

	/* if the password needs to be hashed, do it here to avoid repeating the
	 * key hash computation for each HMAC task execution
	 */
	if (t->params.pbkdf2.pswdsz > blocksz) {
		char *wmem = get_workmem_pointer(t);

		pswd_hash = wmem + PBKDF2_COUNTER_SZ + 2 * digestsz;

		si_hash_create(t, t->hashalg);
		si_task_consume(t, t->params.pbkdf2.pswd, t->params.pbkdf2.pswdsz);
		si_task_produce(t, pswd_hash, digestsz);

		si_wq_run_after(t, &t->params.pbkdf2.wq, pbkdf2_setup_prod);

		/* point to the hashed password */
		t->params.pbkdf2.pswd = pswd_hash;
		t->params.pbkdf2.pswdsz = digestsz;
	} else {
		/* to properly run pbkdf2_setup_prod(), statuscode must be SX_OK
		 */
		t->statuscode = SX_OK;

		pbkdf2_setup_prod(t, NULL);
	}
}
