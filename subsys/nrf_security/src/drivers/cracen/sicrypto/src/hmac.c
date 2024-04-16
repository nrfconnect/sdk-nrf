/* HMAC implementation, based on FIPS 198-1.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the HMAC task:
 *      1. Area used to store values obtained from processing the key.
 *         Size: hash algorithm block size.
 *      2. Area used to store the intermediate hash value.
 *         Size: hash algorithm digest size.
 */

#include <string.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include "../include/sicrypto/hmac.h"
#include "../include/sicrypto/hash.h"
#include "waitqueue.h"
#include "final.h"

/* xor bytes in 'buf' with a constant 'v', in place */
static void xorbuf(char *buf, char v, size_t sz)
{
	for (size_t i = 0; i < sz; i++) {
		*buf = *buf ^ v;
		buf++;
	}
}

static int finish_hmac_computation(struct sitask *t, struct siwq *wq)
{
	size_t blocksz, digestsz;
	(void)wq;

	if (t->statuscode) {
		return t->statuscode;
	}

	/* task for computing the 2nd hash of the HMAC algorithm */
	si_hash_create(t, t->hashalg);

	blocksz = sx_hash_get_alg_blocksz(t->hashalg);
	digestsz = sx_hash_get_alg_digestsz(t->hashalg);

	/* compute K0 xor opad (0x36 ^ 0x5C = 0x6A) */
	xorbuf(t->workmem, 0x6A, blocksz);

	/* feed and start the hash task, result at location provided by the user
	 */
	si_task_consume(t, t->workmem, blocksz + digestsz);
	si_task_produce(t, t->output, digestsz);

	return SX_ERR_HW_PROCESSING;
}

static void hmac_produce(struct sitask *t, char *out, size_t sz)
{
	int r;
	size_t blocksz, digestsz;

	digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	if (sz < digestsz) {
		sx_hash_free(&t->u.h);
		si_task_mark_final(t, SX_ERR_OUTPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* store pointer for final HMAC result, we need it later */
	t->output = out;

	blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	si_wq_run_after(t, &t->params.hmac.wq, finish_hmac_computation);

	/* run hash (1st hash of HMAC algorithm), result inside workmem */
	r = sx_hash_digest(&t->u.h, t->workmem + blocksz);
	if (r) {
		si_task_mark_final(t, r);
	}
}

static int start_hmac_computation(struct sitask *t, struct siwq *wq)
{
	size_t blocksz;
	(void)wq;

	if (t->statuscode) {
		return t->statuscode;
	}

	/* task for computing the 1st hash in the HMAC algorithm */
	si_hash_create(t, t->hashalg);

	/* override the produce function pointer */
	t->actions.produce = hmac_produce;

	blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	/* The "prepared" key (called K0 in FIPS 198-1) is available and
	 * t->workmem points to it. Here we compute K0 xor ipad.
	 */
	xorbuf(t->workmem, 0x36, blocksz);

	/* start feeding the hash task */
	si_task_consume(t, t->workmem, blocksz);

	return SX_ERR_READY;
}

void si_mac_create_hmac(struct sitask *t, const struct sxhashalg *hashalg, const char *key,
			size_t keysz)
{
	size_t digestsz, blocksz;

	t->hashalg = hashalg;
	digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	blocksz = sx_hash_get_alg_blocksz(t->hashalg);

	if (t->workmemsz < blocksz + digestsz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* a key longer than hash block size needs an additional preparation
	 * step
	 */
	if (keysz > blocksz) {
		/* hash the key */
		si_hash_create(t, t->hashalg);
		si_task_consume(t, key, keysz);
		si_task_produce(t, t->workmem, digestsz);

		/* zero padding */
		safe_memset(t->workmem + digestsz, t->workmemsz - digestsz, 0, blocksz - digestsz);
		si_wq_run_after(t, &t->params.hmac.wq, start_hmac_computation);
	} else {
		memcpy(t->workmem, key, keysz);
		/* zero padding */
		safe_memset(t->workmem + keysz, t->workmemsz - keysz, 0, blocksz - keysz);

		t->statuscode = SX_OK;
		start_hmac_computation(t, NULL);
	}
}
