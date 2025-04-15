/* HMAC-based Extract-and-Expand Key Derivation Function (HKDF).
 *
 * Based on RFC 5869 "HMAC-based Extract-and-Expand Key Derivation
 * Function (HKDF)" and https://en.wikipedia.org/wiki/HKDF.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the HKDF task:
 *      1. HMAC sub-task workmem (size: hash block size + hash digest size).
 *      2. prk (size: hash digestsz).
 *      3. t (size: hash digestsz).
 *      4. counter (size: 1 byte).
 *
 * The workmem required size can be computed with:
 *      1 + 3*hash_digest_size + hash_block_size
 *
 * Some example values are:
 *      workmem size for HKDF with HMAC-SHA1:      125 bytes
 *      workmem size for HKDF with HMAC-SHA2-256:  161 bytes
 *      workmem size for HKDF with HMAC-SHA2-512:  321 bytes
 */

#include <string.h>
#include <sxsymcrypt/hash.h>
#include "../include/sicrypto/hmac.h"
#include "../include/sicrypto/hkdf.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include "final.h"
#include "waitqueue.h"

/* Offset to the part of workmem that is specific to the 'hkdf' task. */
#define HKDF_WORKMEM_OFST(digestsz, blocksz) ((digestsz) + (blocksz))

static int continue_hkdf_prod(struct sitask *t, struct siwq *wq)
{
	size_t sz = t->params.hkdf.outsz;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->hashalg);
	char *prk = t->workmem + HKDF_WORKMEM_OFST(digestsz, blocksz);
	char *tmp = prk + digestsz;
	char *counter = tmp + digestsz;

	(void)wq;

	if (t->statuscode) {
		return si_task_mark_final(t, t->statuscode);
	}

	if (sz <= digestsz) {
		memcpy(t->params.hkdf.output, tmp, sz);
		return si_task_mark_final(t, SX_OK);
	}
	sz = digestsz;
	memcpy(t->params.hkdf.output, tmp, sz);
	t->params.hkdf.output += sz;

	si_wq_run_after(t, &t->params.hkdf.wq, continue_hkdf_prod);
	t->params.hkdf.outsz -= digestsz;
	si_mac_create_hmac(t, t->hashalg, prk, digestsz);
	/* Can consume + produce immediately because prk size is
	 * smaller than hash block size.
	 */
	si_task_consume(t, tmp, digestsz);
	si_task_consume(t, t->params.hkdf.info, t->params.hkdf.infosz);
	*counter += 1;
	si_task_consume(t, counter, 1);
	si_task_produce(t, tmp, digestsz);

	return SX_ERR_HW_PROCESSING;
}

static void consume_hkdf_info(struct sitask *t, const char *info, size_t sz)
{
	t->params.hkdf.info = info;
	t->params.hkdf.infosz = sz;

	/* uninstall consume action: only one call allowed to provide "info" */
	t->actions.consume = NULL;
}

static void start_hkdf_produce(struct sitask *t, char *out, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->hashalg);
	char *prk = t->workmem + HKDF_WORKMEM_OFST(digestsz, blocksz);
	char *tmp = prk + digestsz;
	char *counter = tmp + digestsz;

	t->params.hkdf.output = out;
	t->params.hkdf.outsz = sz;
	*counter = 1;

	if (sz == 0) {
		si_task_mark_final(t, SX_OK);
		return;
	}
	if (sz > 255 * digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	si_wq_run_after(t, &t->params.hkdf.wq, continue_hkdf_prod);

	si_mac_create_hmac(t, t->hashalg, prk, digestsz);
	/* Can consume + produce immediately because prk size is
	 * smaller than hash block size.
	 */
	si_task_consume(t, t->params.hkdf.info, t->params.hkdf.infosz);
	si_task_consume(t, counter, 1);
	si_task_produce(t, tmp, digestsz);
}

static int setup_hkdf_prod(struct sitask *t, struct siwq *wq)
{
	(void)wq;
	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}
	t->actions.produce = start_hkdf_produce;
	t->actions.consume = consume_hkdf_info;
	t->statuscode = SX_ERR_READY;

	return SX_ERR_READY;
}

void si_kdf_create_hkdf(struct sitask *t, const struct sxhashalg *hashalg, const char *ikm,
			size_t ikmsz, const char *salt, size_t saltsz)
{
	t->hashalg = hashalg;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	if (t->workmemsz < 1 + 3 * digestsz + blocksz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	const size_t prk_salt_offset = HKDF_WORKMEM_OFST(digestsz, blocksz);
	char *prk_salt = t->workmem + prk_salt_offset;

	/* Limit salt size in current implementation to the digest size. This
	 * way a "wait" after si_mac_create_hmac() can be avoided.
	 * In practice, this limitation is reasonable as RFC 5869 "HMAC-based
	 * Extract-and-Expand Key Derivation Function (HKDF)" section 3.1
	 * "To Salt or not to Salt" recommends salt sizes up to the digest size.
	 */
	if (saltsz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}
	if (!saltsz) {
		safe_memset(prk_salt, t->workmemsz - prk_salt_offset, 0, digestsz);
		salt = prk_salt;
		saltsz = digestsz;
	}

	si_wq_run_after(t, &t->params.hkdf.wq, setup_hkdf_prod);
	t->params.hkdf.infosz = 0;

	si_mac_create_hmac(t, hashalg, salt, saltsz);
	/* Can consume and produce immediately because the code above ensures
	 * that salt size is smaller than hash block size.
	 */
	si_task_consume(t, ikm, ikmsz);
	si_task_produce(t, prk_salt, digestsz);
}
