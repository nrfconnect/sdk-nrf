/* RSA key generation, based on FIPS 186-4.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/iomem.h>
#include "../include/sicrypto/sicrypto.h"
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include "../include/sicrypto/rsa_keygen.h"
#include "coprime_check.h"
#include "rndinrange.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"

static int miller_rabin_get_random(struct sitask *t);

/* Big endian unsigned representation of the product of first 130 odd primes.
 * Note that the factor 2 is not included in this product.
 */
static const unsigned char product_small_primes[128] = {
	0x02, 0xc8, 0x5f, 0xf8, 0x70, 0xf2, 0x4b, 0xe8, 0x0f, 0x62, 0xb1, 0xba, 0x6c, 0x20, 0xbd,
	0x72, 0xb8, 0x37, 0xef, 0xdf, 0x12, 0x12, 0x06, 0xd8, 0x7d, 0xb5, 0x6b, 0x7d, 0x69, 0xfa,
	0x4c, 0x02, 0x1c, 0x10, 0x7c, 0x3c, 0xa2, 0x06, 0xfe, 0x8f, 0xa7, 0x08, 0x0e, 0xf5, 0x76,
	0xef, 0xfc, 0x82, 0xf9, 0xb1, 0x0f, 0x57, 0x50, 0x65, 0x6b, 0x77, 0x94, 0xb1, 0x6a, 0xfd,
	0x70, 0x99, 0x6e, 0x91, 0xae, 0xf6, 0xe0, 0xad, 0x15, 0xe9, 0x1b, 0x07, 0x1a, 0xc9, 0xb2,
	0x4d, 0x98, 0xb2, 0x33, 0xad, 0x86, 0xee, 0x05, 0x55, 0x18, 0xe5, 0x8e, 0x56, 0x63, 0x8e,
	0xf1, 0x8b, 0xac, 0x5c, 0x74, 0xcb, 0x35, 0xbb, 0xb6, 0xe5, 0xda, 0xe2, 0x78, 0x3d, 0xd1,
	0xc0, 0xce, 0x7d, 0xec, 0x4f, 0xc7, 0x0e, 0x51, 0x86, 0xd4, 0x11, 0xdf, 0x36, 0x36, 0x8f,
	0x06, 0x1a, 0xa3, 0x60, 0x11, 0xf3, 0x01, 0x79};

/* Return 1 if abs(p - q) <= 2^(sz*8 - 100), 0 otherwise.
 *
 * Inputs p and q are unsigned integers stored as big endian byte arrays of sz
 * bytes. Argument sz must be at least 13. Both p and q must have their most
 * significant bit set.
 *
 * The function is a constant-time implementation of the algorithm described below.
 * We call phigh the bit string made by the 100 most significant bits of p. It is
 * formed by bytes p[0], p[1], ..., p[11] and the higher half of p[12]. The
 * remaining part of p is called plow, so plow is the bit string consisting of the
 * sz*8 - 100 least significant bits of p. Similarly, we isolate qhigh and qlow
 * from q. We also define deltahigh = phigh - qhigh and deltalow = plow - qlow.
 * 1. Swap p and q so that deltahigh >= 0.
 * 2. If deltahigh == 0, then abs(p - q) <= 2^(sz*8 - 100). End.
 * 3. If deltalow > 0, then abs(p - q) > 2^(sz*8 - 100). End.
 * 4. If deltahigh == 1, then abs(p - q) <= 2^(sz*8 - 100). End.
 * 5. Else abs(p - q) > 2^(sz*8 - 100). End.
 */
static int check_abs_diff(unsigned char *p, unsigned char *q, size_t sz)
{
	int deltahigh_sign, deltalow_sign;
	int diffsmall1, diffsmall2, diffbig;

	unsigned char p12 = p[12]; /* keep original value to restore later */
	unsigned char q12 = q[12];

	/* phigh is p[0], p[1], ..., p[12] after applying mask 0xF0 to p[12] */
	p[12] &= 0xF0;
	q[12] &= 0xF0;

	/* Get sign of deltahigh. Since si_be_cmp() works on bytes, we are
	 * comparing phigh*16 with qhigh*16 but that does not change the result.
	 */
	deltahigh_sign = si_be_cmp(p, q, 13, 0);

	/* If phigh and qhigh are equal, then abs(p - q) is less than 2^(sz*8 -
	 * 100) so we set the diffsmall1 flag.
	 */
	diffsmall1 = (deltahigh_sign == 0);

	/* Swap p and q if phigh is less than qhigh. A non constant-time swap is
	 * ok: the only information we may leak here is about p being greater than or
	 * less than q.
	 */
	if (deltahigh_sign == -1) {
		unsigned char *tmp_ptr = p;
		unsigned char tmp = p12;

		p = q;
		p12 = q12;
		q = tmp_ptr;
		q12 = tmp;
	}

	/* Get sign of deltahigh - 1. The carry argument must be -16 to emulate
	 * an initial carry of -1.
	 */
	deltahigh_sign = si_be_cmp(p, q, 13, -16);

	/* plow is p[12], p[13], ..., p[sz-1] after applying mask 0x0F to p[12]
	 */
	p[12] = p12 & 0x0F;
	q[12] = q12 & 0x0F;

	/* get sign of deltalow */
	deltalow_sign = si_be_cmp(&p[12], &q[12], sz - 12, 0);

	/* restore original values */
	p[12] = p12;
	q[12] = q12;

	diffbig = (deltalow_sign > 0);
	diffsmall2 = (deltahigh_sign == 0);

	return (diffsmall1 || (!diffbig && diffsmall2));
}

/* Compute v = v - subtrahend.
 *
 * Argument v is an unsigned integer stored as a big endian byte array of sz bytes.
 * Underflow will occur in the result if v is less than subtrahend. The code is
 * based on the assumption that two's complement representation is used for signed
 * integers.
 */
static void be_sub(unsigned char *v, size_t sz, unsigned char subtrahend)
{
	int tmp = subtrahend;

	for (; sz > 0;) {
		sz--;
		tmp = v[sz] - tmp;
		v[sz] = tmp & 0xFF;
		tmp = (tmp >> 8) & 1;
	}
}

/* Return pointer to the integer under test. */
static char *get_candidate_prime(struct sitask *t)
{
	if (t->params.rsacheckpq.q == NULL) {
		return t->params.rsacheckpq.p;
	} else {
		return t->params.rsacheckpq.q;
	}
}

static int miller_rabin_round_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	sx_pk_release_req(t->pk);

	t->params.rsacheckpq.mrrounds--;

	/* if the Miller Rabin round returned SX_ERR_COMPOSITE_VALUE or another
	 * error code, this task stops with the same value in its statuscodes
	 */
	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	if (t->params.rsacheckpq.mrrounds > 0) {
		/* more Miller Rabin rounds needed */
		return miller_rabin_get_random(t);
	}

	/* the candidate is probably prime */
	return SX_OK;
}

/* Offset to the part of workmem that is specific to the 'checkpq' task. */
#define CHECKPQ_WORKMEM_OFST(candidatesz) (candidatesz)

static int miller_rabin_round_start(struct sitask *t, struct siwq *wq)
{
	size_t candidatesz = t->params.rsacheckpq.candidatesz;
	struct sx_pk_acq_req pkreq;
	char *candprime = get_candidate_prime(t);
	struct sx_buf random, candidate;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* add 1 to the random integer so that it is in [2, candidate prime - 2]
	 */
	si_be_add((unsigned char *)t->workmem, candidatesz, 1);

	random.bytes = t->workmem;
	random.sz = candidatesz;

	candidate.bytes = candprime;
	candidate.sz = candidatesz;

	si_wq_run_after(t, &t->params.rsacheckpq.wq, miller_rabin_round_finish);

	/* execute a Miller Rabin round */
	pkreq = sx_async_miller_rabin_go(&candidate, &random);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->pk = pkreq.req;

	return SX_ERR_HW_PROCESSING;
}

static int miller_rabin_get_random(struct sitask *t)
{
	size_t candidatesz = t->params.rsacheckpq.candidatesz;
	char *wmem = t->workmem + CHECKPQ_WORKMEM_OFST(candidatesz);

	si_wq_run_after(t, &t->params.rsacheckpq.wq, miller_rabin_round_start);

	/* When using the rndinrange task, the upper limit argument must be odd,
	 * which is the case here since it is equal to the candidate prime - 2. The
	 * random output is placed in the second part of wmem.
	 */
	si_rndinrange_create(t, (const unsigned char *)wmem, candidatesz, t->workmem);
	si_task_run(t);

	return SX_ERR_HW_PROCESSING;
}

static int coprimecheck_finish(struct sitask *t, struct siwq *wq)
{
	size_t candidatesz = t->params.rsacheckpq.candidatesz;
	char *wmem = t->workmem + CHECKPQ_WORKMEM_OFST(candidatesz);
	(void)wq;

	/* the status code value here is SX_ERR_NOT_INVERTIBLE if
	 * (candidate prime - 1) and the public exponent are not coprime
	 */
	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* For the Miller Rabin tests, a random integer in [2, candidate prime -
	 * 2] is needed. We can use the rndinrange task to obtain a random number in
	 * [1, candidate prime - 3] and then add 1 to it, so that the number falls in
	 * the required interval. The candidate prime in wmem was already decreased by
	 * 1, so here we just need to subtract 1 before passing it as the 'upper limit'
	 * to the rndinrange task.
	 */
	be_sub((unsigned char *)wmem, candidatesz, 1);

	return miller_rabin_get_random(t);
}

static int coprimecheck_start(struct sitask *t, struct siwq *wq)
{
	size_t candidatesz = t->params.rsacheckpq.candidatesz;
	char *wmem = t->workmem + CHECKPQ_WORKMEM_OFST(candidatesz);
	char *candprime = get_candidate_prime(t);
	(void)wq;

	if (t->statuscode == SX_ERR_NOT_INVERTIBLE) {
		return (t->statuscode = SX_ERR_COMPOSITE_VALUE);
	}

	/* an error may have occurred in the sub task */
	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* copy the candidate prime under test to workmem */
	memcpy(wmem, candprime, candidatesz);

	/* subtract 1 to the workmem copy of the candidate prime */
	wmem[candidatesz - 1] &= 0xFE;

	si_wq_run_after(t, &t->params.rsacheckpq.wq, coprimecheck_finish);

	/* step 4.5 (or 5.6): candprime - 1 and public exponent must be coprime
	 */
	si_create_coprime_check(t, wmem, candidatesz, (const char *)t->params.rsacheckpq.pubexp,
				t->params.rsacheckpq.pubexpsz);

	si_task_run(t);

	return t->statuscode;
}

static void rsacheckpq_run(struct sitask *t)
{
	/* big endian unsigned representation of ceil(2^63.5) */
	static const unsigned char compareval[8] = "\xB5\x04\xF3\x33\xF9\xDE\x64\x85";
	size_t candidatesz = t->params.rsacheckpq.candidatesz;
	char *candprime = get_candidate_prime(t);
	int r;

	/* Step 4.4 (or 5.5): compare the most significant 64 bits of the
	 * candidate prime with the value in compareval[]. This approximate test is
	 * stricter than the one prescribed in step 4.4 (or 5.5). The probability that
	 * a valid candidate (one that meets the condition in step 4.4) is rejected by
	 * this test is in the order of 2^(-64).
	 */
	r = si_be_cmp((const unsigned char *)candprime, compareval, sizeof(compareval), 0);

	if (r < 0) {
		/* reject candidate prime as step 4.4 (or 5.5) failed */
		si_task_mark_final(t, SX_ERR_RSA_PQ_RANGE_CHECK_FAIL);
		return;
	}

	/* step 5.4, to be done only if we are testing q */
	if (t->params.rsacheckpq.q != NULL) {
		/* Use of check_abs_diff() with candidatesz as third argument
		 * corresponds to the test in step 5.4 only if p and q have the most
		 * significant bit set, i.e. their bit size is 8*candidatesz. The checks in
		 * the task create function enforce this constraint.
		 */
		r = check_abs_diff((unsigned char *)t->params.rsacheckpq.p,
				   (unsigned char *)candprime, candidatesz);

		if (r == 1) {
			/* reject candidate prime */
			si_task_mark_final(t, SX_ERR_RSA_PQ_RANGE_CHECK_FAIL);
			return;
		}
	}

	si_wq_run_after(t, &t->params.rsacheckpq.wq, coprimecheck_start);

	/* preliminary primality check: the candidate prime and the product of
	 * the first 130 odd primes must be coprime
	 */
	si_create_coprime_check(t, candprime, candidatesz, (const char *)product_small_primes,
				sizeof(product_small_primes));

	si_task_run(t);
}

/** Create a task to check p and q candidates.
 *
 * This task is used, in the context of RSA key generation, to check whether the
 * given p and q meet all the conditions required to be valid RSA prime factors,
 * in conformance with FIPS 186-4. In particular, this task follows the methods
 * in section B.3.3 ("Generation of Random Primes that are Probably Prime") and
 * section C.3.1 and the number of Miller Rabin rounds is set according to Table
 * C.2.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] pubexp       Public exponent. It is an unsigned integer stored as
 *                         a big endian byte array.
 * @param[in] pubexpsz     Size in bytes of the public exponent.
 * @param[in] p            p value to check. It is an unsigned integer stored as
 *                         a big endian byte array.
 * @param[in] q            q value to check. It is an unsigned integer stored as
 *                         a big endian byte array.
 * @param[in] candidatesz  Size in bytes of \p p and \p q.
 * @param[in] workmem      Buffer that the task uses for intermediate results.
 *                         Its size must be \p 2*candidatesz bytes.
 *
 * The task can be used in two ways. The first way is when its create function
 * is called with \p p not NULL and \p q NULL: in this case the task checks the
 * validity of \p p only. The second way is when the create function is called
 * with both \p p and \p q not NULL: in this case it checks the validity of \p q
 * given \p p.
 *
 * On task completion, the value of the task's status code indicates whether the
 * candidate prime \p p or \p q passed the tests successfully (the status code
 * is SX_OK) or not (the status code contains an error code).
 *
 * Buffers \p p and \p q must be writable buffers, in fact the task temporarily
 * modifies them during its calculations (and then always restores the original
 * values).
 *
 * The public exponent \p pubexp must not have a zero in its most significant
 * byte.
 *
 * This task works under the assumption that p and q have the same byte size
 * \p candidatesz and that their bit size is \p 8*candidatesz (the most
 * significant bit in p and q is set).
 *
 * This task performs steps 4.4, 4.5, 4.5.1, 4.5.2 and 5.4, 5.5, 5.6, 5.6.1,
 * 5.6.2 of section B.3.3 of FIPS 186-4. The calling task is responsible for
 * performing the remaining steps of section B.3.3.
 *
 * When this task returns statuscode values SX_ERR_COMPOSITE_VALUE or
 * SX_ERR_NOT_INVERTIBLE, the calling task has to increment the 'attempts'
 * counter (called i in FIPS 186-4). When this task returns statuscode value
 * SX_ERR_RSA_PQ_RANGE_CHECK_FAIL, the calling task does not have to increment
 * the 'attempts' counter.
 *
 * Workmem layout:
 *      1. an area of candidatesz bytes, used for three different things:
 *          - as the workmem for the first invocation of the 'coprime check'
 *            subtask (when the subtask performs a preliminary primality check).
 *          - as the workmem for the second invocation of the 'coprime check'
 *            subtask (when the subtask performs the checks in step 4.5 or 5.6).
 *          - to hold the random integers produced by the 'rndinrange' subtask
 *            (which are needed for the Miller Rabin probabilistic primality
 *            test).
 *      2. an area of candidatesz bytes, used to hold a copy of the prime number
 *         under test, which is then decreased by 1 and by 2 (the task needs
 *         such values).
 *
 * The first part of workmem needs room for candidatesz bytes. This derives
 * directly from:
 *      max(candidatesz, min(candidatesz, pubexpsz), min(candidatesz, 128))
 */
void si_rsa_create_check_pq(struct sitask *t, const char *pubexp, size_t pubexpsz, char *p, char *q,
			    size_t candidatesz)
{
	/* The size of p and q must be at least 13 bytes otherwise the condition
	 * in step 5.4 cannot be met. The bit size of the public exponent must be at
	 * least 17 bits otherwise pubexp would be less than 2^16, which violates one
	 * of the conditions in step 2. Note: we are not exactly enforcing the
	 * condition pubexp > 2^16, in fact a pubexp exactly equal to 2^16 would pass
	 * this check.
	 */
	if ((candidatesz < 13) || (pubexpsz < 3)) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* The public exponent must be odd. Its size must not be greater than
	 * the key size, which is twice candidatesz. Its most significant byte must not
	 * be zero.
	 */
	if (((pubexp[pubexpsz - 1] & 1) == 0) || (pubexpsz > (2 * candidatesz)) ||
	    (pubexp[0] == 0)) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* pointer p must not be NULL */
	if (p == NULL) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* Preliminary check of the condition in steps 4.4 and 5.5: if the
	 * integers under test do not have their most significant bit set, they will
	 * not meet that condition.
	 */
	if ((!(p[0] & 0x80)) || ((q != NULL) && !(q[0] & 0x80))) {
		si_task_mark_final(t, SX_ERR_RSA_PQ_RANGE_CHECK_FAIL);
		return;
	}

	/* Preliminary primality check: the integers under test must not be
	 * even.
	 */
	if ((!(p[candidatesz - 1] & 1)) || ((q != NULL) && !(q[candidatesz - 1] & 1))) {
		si_task_mark_final(t, SX_ERR_COMPOSITE_VALUE);
		return;
	}

	t->params.rsacheckpq.pubexp = pubexp;
	t->params.rsacheckpq.pubexpsz = pubexpsz;
	t->params.rsacheckpq.candidatesz = candidatesz;
	t->params.rsacheckpq.p = p;
	t->params.rsacheckpq.q = q;

	/* set number of MR rounds as per Table C.2 in FIPS 186.4 (192*8 = 1536)
	 */
	if (candidatesz >= 192) {
		t->params.rsacheckpq.mrrounds = 4;
	} else {
		t->params.rsacheckpq.mrrounds = 5;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsacheckpq_run;
}

static void rsagenpq_get_random(struct sitask *t);

static int rsagenpq_continue(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	/* According to FIPS 186-4 section B.3.3 the 'attempts' counter should
	 * not be incremented if the status code is SX_ERR_RSA_PQ_RANGE_CHECK_FAIL.
	 * However we decide to do the increment in order to rule out the possibility
	 * of endless loops. This does not have a significant impact on the algorithm
	 * because for step 4.4 (or 5.5) the probability of rejection is about 0.41 and
	 * for step 5.4 the probability of rejection is about 2^-100.
	 */
	if ((t->statuscode == SX_ERR_COMPOSITE_VALUE) || (t->statuscode == SX_ERR_NOT_INVERTIBLE) ||
	    (t->statuscode == SX_ERR_RSA_PQ_RANGE_CHECK_FAIL)) {
		/* step 4.6 (or 5.7): increment counter */
		t->params.rsagenpq.attempts++;

		/* step 4.7 (or 5.8): check the number of attempts */
		if (t->params.rsagenpq.attempts >= 5 * (t->params.rsagenpq.candidatesz << 3)) {
			return si_task_mark_final(t, SX_ERR_TOO_MANY_ATTEMPTS);
		}

		/* generate a new random */
		rsagenpq_get_random(t);
	} else if (t->statuscode == SX_OK) {
		/* the task can end if the valid prime factor just found was q
		 */
		if (t->params.rsagenpq.qptr != NULL) {
			return SX_OK;
		}

		/* a valid p was found, now go on to generate q */
		t->params.rsagenpq.qptr = t->params.rsagenpq.q;
		t->params.rsagenpq.rndout = t->params.rsagenpq.q;
		t->params.rsagenpq.attempts = 0; /* step 5.1 */
		rsagenpq_get_random(t);
	}

	return t->statuscode;
}

static int rsagenpq_check_random(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* Step 4.3: if the random integer just obtained is not odd, add 1 to
	 * it. The random byte string is interpreted as big endian.
	 */
	t->params.rsagenpq.rndout[t->params.rsagenpq.candidatesz - 1] |= 1;

	/* To avoid useless iterations, set the most significant bit of the
	 * random integer. In fact, the condition in step 4.4 cannot be met when the
	 * most significant bit is 0.
	 */
	t->params.rsagenpq.rndout[0] |= 0x80;

	si_wq_run_after(t, &t->params.rsagenpq.wq, rsagenpq_continue);

	si_rsa_create_check_pq(t, t->params.rsagenpq.pubexp, t->params.rsagenpq.pubexpsz,
			       t->params.rsagenpq.p, t->params.rsagenpq.qptr,
			       t->params.rsagenpq.candidatesz);
	si_task_run(t);
	return SX_ERR_HW_PROCESSING;
}

static void rsagenpq_get_random(struct sitask *t)
{
	/* step 4.2: get keysz/2 random bytes from the environment's PRNG task
	 */
	psa_status_t status =
		cracen_get_random(NULL, t->params.rsagenpq.rndout, t->params.rsagenpq.candidatesz);
	if (status != PSA_SUCCESS) {
		si_task_mark_final(t, SX_ERR_UNKNOWN_ERROR);
		return;
	}

	si_wq_run_after(t, &t->params.rsagenpq.wq, rsagenpq_check_random);

	/* this is needed when entering this function from rsagenpq_continue()
	 */
	t->statuscode = SX_ERR_HW_PROCESSING;

	t->actions.status = si_status_on_prng;
	t->actions.wait = si_wait_on_prng;
}

/** Create a task to generate the RSA prime factors p and q.
 *
 * This task is used, in the context of RSA key generation, to generate two
 * primes p and q that meet the conditions specified in FIPS 186-4. This task
 * follows the method in section B.3.3 ("Generation of Random Primes that are
 * Probably Prime") of FIPS 186-4.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] pubexp       Public exponent. It is an unsigned integer stored as
 *                         a big endian byte array.
 * @param[in] pubexpsz     Size in bytes of the public exponent.
 * @param[in] keysz        Size in bytes of the RSA key (size of the RSA
 *                         modulus). It must be even.
 * @param[out] p           Buffer where the prime factor p is written, as an
 *                         unsigned integer in big endian form. This buffer must
 *                         have a size of \p keysz/2 bytes.
 * @param[out] q           Buffer where the prime factor q is written, as an
 *                         unsigned integer in big endian form. This buffer must
 *                         have a size of \p keysz/2 bytes.
 * @param[in] workmem      Buffer that the task uses for intermediate results.
 *                         Its size must be equal to \p keysz bytes.
 *
 * The key size in bytes, \p keysz, is restricted to be even. Therefore this
 * task supports only RSA keys whose bit length is a multiple of 16.
 *
 * The public exponent \p pubexp must not have a zero in its most significant
 * byte.
 *
 * This task performs steps 4.1, 4.2, 4.3, 4.6, 4.7 and 5.1, 5.2, 5.3, 5.7, 5.8
 * of section B.3.3 of FIPS 186-4.
 *
 * The workmem for this task is used as the workmem for the "rsa check pq"
 * subtask.
 */
void si_rsa_create_generate_pq(struct sitask *t, const char *pubexp, size_t pubexpsz, size_t keysz,
			       char *p, char *q)
{
	if ((pubexpsz == 0) || (keysz == 0)) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* The public exp must be odd. Its size must not be greater than the key
	 * size. Its most significant byte must not be zero.
	 */
	if (((pubexp[pubexpsz - 1] & 1) == 0) || (pubexpsz > keysz) || (pubexp[0] == 0)) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* keysz must be even */
	if (keysz & 1) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	t->params.rsagenpq.pubexp = pubexp;
	t->params.rsagenpq.pubexpsz = pubexpsz;
	t->params.rsagenpq.candidatesz = keysz >> 1;
	t->params.rsagenpq.p = p;
	t->params.rsagenpq.q = q;
	t->params.rsagenpq.qptr = NULL;

	/* step 4.1 */
	t->params.rsagenpq.attempts = 0;

	/* first we generate p */
	t->params.rsagenpq.rndout = p;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsagenpq_get_random;
}

/* Offset to the part of workmem that is specific to the 'genprivkey' task. */
#define GENPRIVKEY_WORKMEM_OFST(keysz) (keysz)

/* Copy key elements (p, q, dp, dq, qinv) to the user-provided output key. */
static int rsagenpriv_crt_finish(struct sitask *t, struct siwq *wq)
{
	size_t keysz = t->params.rsagenprivkey.keysz;
	char *wmem = t->workmem + GENPRIVKEY_WORKMEM_OFST(keysz);
	size_t primefactorsz = keysz >> 1;
	const char **outputs = sx_pk_get_output_ops(t->pk);
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* get p and q from the workmem */
	memcpy(t->params.rsagenprivkey.key->elements[0]->bytes, wmem, primefactorsz);
	memcpy(t->params.rsagenprivkey.key->elements[1]->bytes, wmem + primefactorsz,
	       primefactorsz);

	/* get dp, dq and qinv from device memory slots (take the last
	 * primefactorsz bytes of each slot)
	 */
	sx_rdpkmem(t->params.rsagenprivkey.key->elements[2]->bytes, outputs[0] + primefactorsz,
		   primefactorsz);
	sx_rdpkmem(t->params.rsagenprivkey.key->elements[3]->bytes, outputs[1] + primefactorsz,
		   primefactorsz);
	sx_rdpkmem(t->params.rsagenprivkey.key->elements[4]->bytes, outputs[2] + primefactorsz,
		   primefactorsz);

	sx_pk_release_req(t->pk);

	return t->statuscode;
}

/* Generate the remaining CRT parameters dp, dq and qinv. */
static int rsagenpriv_crt_continue(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	size_t keysz = t->params.rsagenprivkey.keysz;
	char *wmem = t->workmem + GENPRIVKEY_WORKMEM_OFST(keysz);
	size_t primefactorsz = keysz >> 1;
	const char **outputs = sx_pk_get_output_ops(t->pk);
	sx_op p, q, privexp;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* copy the private exponent d to workmem */
	sx_rdpkmem(t->workmem, outputs[2], keysz);

	sx_pk_release_req(t->pk);

	p.bytes = wmem;
	p.sz = primefactorsz;

	q.bytes = wmem + primefactorsz;
	q.sz = primefactorsz;

	privexp.bytes = t->workmem;
	privexp.sz = keysz;

	si_wq_run_after(t, &t->params.rsagenprivkey.wq, rsagenpriv_crt_finish);

	pkreq = sx_async_rsa_crt_keyparams_go(&p, &q, &privexp);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->pk = pkreq.req;

	return SX_ERR_HW_PROCESSING;
}

static int rsagenpriv_finish(struct sitask *t, struct siwq *wq)
{
	size_t keysz = t->params.rsagenprivkey.keysz;
	const char **outputs = sx_pk_get_output_ops(t->pk);
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* get modulus n */
	sx_rdpkmem(t->params.rsagenprivkey.key->elements[0]->bytes, outputs[0], keysz);

	/* get private exponent d */
	sx_rdpkmem(t->params.rsagenprivkey.key->elements[1]->bytes, outputs[2], keysz);

	sx_pk_release_req(t->pk);

	return t->statuscode;
}

/* Detect the RSA key type (normal or CRT). */
#define IS_RSA_KEY_CRT(keyptr) ((keyptr)->slotmask >> 3)

/* Start generation of n and d. */
static int rsagenpriv_generate_nd(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	size_t keysz = t->params.rsagenprivkey.keysz;
	char *wmem = t->workmem + GENPRIVKEY_WORKMEM_OFST(keysz);
	size_t primefactorsz = keysz >> 1;
	sx_op p, q, pubexp;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	p.bytes = wmem;
	p.sz = primefactorsz;

	q.bytes = wmem + primefactorsz;
	q.sz = primefactorsz;

	pubexp.bytes = (char *)t->params.rsagenprivkey.pubexp;
	pubexp.sz = t->params.rsagenprivkey.pubexpsz;

	if (IS_RSA_KEY_CRT(t->params.rsagenprivkey.key)) {
		si_wq_run_after(t, &t->params.rsagenprivkey.wq, rsagenpriv_crt_continue);
	} else {
		si_wq_run_after(t, &t->params.rsagenprivkey.wq, rsagenpriv_finish);
	}

	pkreq = sx_async_rsa_keygen_go(&p, &q, &pubexp);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->pk = pkreq.req;

	return SX_ERR_HW_PROCESSING;
}

/* Task to generate an RSA private key, based on FIPS 186-4.
 *
 * Workmem layout:
 *     1. an area of keysz bytes:
 *		- used for the workmem of the 'rsa generate pq' subtask.
 *		- if the key to generate is in the CRT form, this area is also used to
 *		  hold the private exponent d.
 *     2. an area of keysz/2 bytes, used to hold the generated p.
 *     3. an area of keysz/2 bytes, used to hold the generated q.
 *
 * The computation of p and q is based on FIPS 186-4. This task performs steps 1
 * and 2 of section B.3.3 of FIPS 186-4. The remaining steps of that section are
 * performed by the 'generate pq' subtask.
 */
void si_rsa_create_genprivkey(struct sitask *t, const char *pubexp, size_t pubexpsz, size_t keysz,
			      struct si_rsa_key *privkey)
{
	if (t->workmemsz < 2 * keysz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* step 1: FIPS 186-4 and FIPS 186-5 require keys of at least 2048 bits
	 * (256 bytes) when using for prime generation the "random primes" method.
	 * Moreover, RSA keys smaller that 2048 bits are considered weak by the
	 * security community.
	 */
	if (keysz < 256) {
		si_task_mark_final(t, SX_ERR_INCOMPATIBLE_HW);
		return;
	}

	/* step 2: the public exponent must be strictly less than 2^256 */
	if (pubexpsz > 32) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* step 2: the public exponent must be strictly greater than 2^16 */
	if ((pubexpsz <= 2) ||
	    ((pubexpsz == 3) && (pubexp[0] == 1) && (pubexp[1] == 0) && (pubexp[2] == 0))) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* The public exponent must be odd (step 2 requirement). Its most
	 * significant byte must not be zero. We restrict keysz to be even.
	 */
	if (((pubexp[pubexpsz - 1] & 1) == 0) || (pubexp[0] == 0) || (keysz & 1)) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	t->params.rsagenprivkey.pubexp = pubexp;
	t->params.rsagenprivkey.pubexpsz = pubexpsz;
	t->params.rsagenprivkey.keysz = keysz;
	t->params.rsagenprivkey.key = privkey;

	si_wq_run_after(t, &t->params.rsagenprivkey.wq, rsagenpriv_generate_nd);

	char *wmem = t->workmem + GENPRIVKEY_WORKMEM_OFST(keysz);

	/* generate prime factors p, q */
	si_rsa_create_generate_pq(t, pubexp, pubexpsz, keysz, wmem, wmem + (keysz >> 1));
}
