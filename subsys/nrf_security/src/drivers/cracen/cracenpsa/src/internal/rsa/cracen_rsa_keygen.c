/* RSA key generation, based on FIPS 186-4.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/rsa/cracen_rsa_common.h>
#include <internal/rsa/cracen_rsa_coprime_check.h>

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/core.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/cmddefs/rsa.h>
#include <silexpk/iomem.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen/common.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen_psa_primitives.h>
#include <stdio.h>

#define WORKMEM_SIZE (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) * 2)

/* Offset to the part of workmem that is specific to the 'checkpq' function. */
#define CHECKPQ_WORKMEM_OFST(candidatesz) (candidatesz)

/* Detect the RSA key type (normal or CRT). */
#define CRACEN_RSA_KEY_CRT(keyptr) ((keyptr)->slotmask >> 3)

/* Big endian unsigned representation of the product of first 130 odd primes.
 * Note that the factor 2 is not included in this product.
 */
static const uint8_t product_small_primes[128] = {
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
static int check_abs_diff(uint8_t *p, uint8_t *q, size_t sz)
{
	int deltahigh_sign;
	int deltalow_sign;
	int diffsmall1;
	int diffsmall2;
	int diffbig;

	uint8_t p12 = p[12]; /* keep original value to restore later */
	uint8_t q12 = q[12];

	/* phigh is p[0], p[1], ..., p[12] after applying mask 0xF0 to p[12] */
	p[12] &= 0xF0;
	q[12] &= 0xF0;

	/* Get sign of deltahigh. Since cracen_be_cmp() works on bytes, we are
	 * comparing phigh*16 with qhigh*16 but that does not change the result.
	 */
	deltahigh_sign = cracen_be_cmp(p, q, 13, 0);

	/* If phigh and qhigh are equal, then abs(p - q) is less than 2^(sz*8 -
	 * 100) so we set the diffsmall1 flag.
	 */
	diffsmall1 = (deltahigh_sign == 0);

	/* Swap p and q if phigh is less than qhigh. A non constant-time swap is
	 * ok: the only information we may leak here is about p being greater than or
	 * less than q.
	 */
	if (deltahigh_sign == -1) {
		uint8_t *tmp_ptr = p;
		uint8_t tmp = p12;

		p = q;
		p12 = q12;
		q = tmp_ptr;
		q12 = tmp;
	}

	/* Get sign of deltahigh - 1. The carry argument must be -16 to emulate
	 * an initial carry of -1.
	 */
	deltahigh_sign = cracen_be_cmp(p, q, 13, -16);

	/* plow is p[12], p[13], ..., p[sz-1] after applying mask 0x0F to p[12]
	 */
	p[12] = p12 & 0x0F;
	q[12] = q12 & 0x0F;

	/* get sign of deltalow */
	deltalow_sign = cracen_be_cmp(&p[12], &q[12], sz - 12, 0);

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
static void be_sub(uint8_t *v, size_t sz, uint8_t subtrahend)
{
	int tmp = subtrahend;

	while (sz > 0) {
		sz--;
		tmp = v[sz] - tmp;
		v[sz] = tmp & 0xFF;
		tmp = (tmp >> 8) & 1;
	}
}

/* Return pointer to the integer under test. */
static uint8_t *get_candidate_prime(struct cracen_rsacheckpq *rsacheckpq)
{
	if (rsacheckpq->q == NULL) {
		return rsacheckpq->p;
	} else {
		return rsacheckpq->q;
	}
}

/* Offset to the part of workmem that is specific to the 'genprivkey' function. */
#define GENPRIVKEY_WORKMEM_OFST(keysz) (keysz)

static int rsagenpriv_crt_finish(sx_pk_req *req, size_t keysz, uint8_t *workmem,
				 struct cracen_rsa_key *priv_key)
{
	struct sx_pk_inops_rsa_crt_keyparams inputs;
	uint8_t *wmem = workmem + GENPRIVKEY_WORKMEM_OFST(keysz);
	size_t primefactorsz = keysz >> 1;
	int sizes[] = {primefactorsz, primefactorsz, keysz};

	/* Copy p and q to output */
	memcpy(priv_key->elements[0]->bytes, wmem, primefactorsz);
	memcpy(priv_key->elements[1]->bytes, wmem + primefactorsz, primefactorsz);

	sx_pk_set_cmd(req, SX_PK_CMD_RSA_CRT_KEYPARAMS);

	int status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}

	sx_wrpkmem(inputs.p.addr, wmem, primefactorsz);
	sx_wrpkmem(inputs.q.addr, wmem + primefactorsz, primefactorsz);
	sx_wrpkmem(inputs.privkey.addr, workmem, keysz);

	sx_pk_run(req);
	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	/* get dp, dq and qinv from device memory slots (take the last
	 * primefactorsz bytes of each slot)
	 */
	sx_rdpkmem(priv_key->elements[2]->bytes, outputs[0] + primefactorsz, primefactorsz);
	sx_rdpkmem(priv_key->elements[3]->bytes, outputs[1] + primefactorsz, primefactorsz);
	sx_rdpkmem(priv_key->elements[4]->bytes, outputs[2] + primefactorsz, primefactorsz);

	return SX_OK;
}

static int rsagenpriv_finish(sx_pk_req *req, struct cracen_rsa_key *key, size_t keysz)
{
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(req);

	/* get modulus n */
	sx_rdpkmem(key->elements[0]->bytes, outputs[0], keysz);

	/* get private exponent d */
	sx_rdpkmem(key->elements[1]->bytes, outputs[2], keysz);

	return SX_OK;
}

static int miller_rabin_run(sx_pk_req *req, const uint8_t *candidate, const uint8_t *random,
			    size_t candidatesz)
{
	struct sx_pk_inops_miller_rabin inputs;
	int sizes[] = {candidatesz, candidatesz};

	sx_pk_set_cmd(req, SX_PK_CMD_MILLER_RABIN);

	int status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}

	sx_wrpkmem(inputs.n.addr, candidate, candidatesz);
	sx_wrpkmem(inputs.a.addr, random, candidatesz);

	sx_pk_run(req);

	return sx_pk_wait(req);
}

static int miller_rabin_get_random(sx_pk_req *req, uint8_t *workmem,
				   struct cracen_rsacheckpq *rsacheckpq)
{
	int sx_status;
	size_t candidatesz = rsacheckpq->candidatesz;
	uint8_t *wmem = workmem + CHECKPQ_WORKMEM_OFST(candidatesz);

	/* When using cracen_get_rnd_in_range, the upper limit argument must be odd,
	 * which is the case here since it is equal to the candidate prime - 2. The
	 * random output is placed in the second part of wmem.
	 */
	while (rsacheckpq->mrrounds > 0) {
		/* step 5.2: get a random integer in [2, candidate prime - 2]
		 * from the environment's PRNG function
		 */
		psa_status_t status =
			cracen_get_rnd_in_range((const uint8_t *)wmem, candidatesz, workmem);
		if (status != PSA_SUCCESS) {
			return SX_ERR_UNKNOWN_ERROR;
		}

		/* add 1 to the random integer so that it is in [2, candidate prime - 2]
		 */
		cracen_be_add(workmem, candidatesz, 1);
		uint8_t *candprime = get_candidate_prime(rsacheckpq);

		/* execute a Miller Rabin round */
		sx_status = miller_rabin_run(req, candprime, workmem, candidatesz);

		/* if the Miller Rabin round returned SX_ERR_COMPOSITE_VALUE or another
		 * error code, this functions returns the error code.
		 */
		if (sx_status != SX_OK) {
			return sx_status;
		}

		rsacheckpq->mrrounds--;
	}

	/* the candidate is probably prime */
	return SX_OK;
}

/** Generate and check p and q candidates.
 *
 * This function is used, in the context of RSA key generation, to generate two
 * primes p and q that meet the conditions specified in FIPS 186-4. This function
 * follows the method in section B.3.3 ("Generation of Random Primes that are
 * Probably Prime") of FIPS 186-4.and then
 * check whether the
 * given p and q meet all the conditions required to be valid RSA prime factors,
 * in conformance with FIPS 186-4. In particular, this function follows the methods
 * in section B.3.3 ("Generation of Random Primes that are Probably Prime") and
 * section C.3.1 and the number of Miller Rabin rounds is set according to Table
 * C.2.
 *
 * @param[in] workmem      Buffer that the function uses for intermediate results.
 *                         Its size must be \p 2*candidatesz bytes.
 * @param[in] rsagenpq     Structure that contains the parameters needed to
 *                         generate p and q
 *
 *  The key size in bytes, keysz in \p rsagenpq, is restricted to be even. Therefore this
 * function supports only RSA keys whose bit length is a multiple of 16.
 *
 * The public exponent pubexp in \p rsagenpq must not have a zero in its most significant
 * byte.
 *
 * On completion, the value of the status code indicates whether the
 * candidate prime p in \p rsagenpq or q in \p rsagenpq passed the tests
 * successfully (the status code is SX_OK) or not (the status code contains an error code).
 *
 *
 * The public exponent pubexp must not have a zero in its most significant
 * byte.
 *
 * This function works under the assumption that p and q have the same byte size
 * candidatesz in \p rsagenpq and that their bit size is \p 8*candidatesz (the most
 * significant bit in p and q is set).
 *
 * This performs steps , 4.2, 4.3, 4.4, 4.5, 4.5.1, 4.5.2, 4.6,
 * 4.7, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.6.1,  5.6.2  5.7 and 5.8
 * of section B.3.3 of FIPS 186-4.
 *
 * Workmem layout:
 *      1. an area of candidatesz bytes, used for three different things:
 *          - as the workmem for the first invocation of the 'coprime check'
 *          - as the workmem for the second invocation of the 'coprime check'
 *          - to hold the random integers produced by the 'rndinrange'
 *            (which are needed for the Miller Rabin probabilistic primality
 *            test).
 *      2. an area of candidatesz bytes, used to hold a copy of the prime number
 *         under test, which is then decreased by 1 and by 2
 *
 * The first part of workmem needs room for candidatesz bytes. This derives
 * directly from:
 *      max(candidatesz, min(candidatesz, pubexpsz), min(candidatesz, 128))
 */
static int rsagenpq_get_random(sx_pk_req *req, uint8_t *workmem, struct cracen_rsagenpq *rsagenpq)
{
	int sx_status;
	/* step 4.2: get keysz/2 random bytes from the environment's PRNG
	 */
	psa_status_t psa_status = cracen_get_random(NULL, rsagenpq->rndout, rsagenpq->candidatesz);

	if (psa_status != PSA_SUCCESS) {
		return SX_ERR_UNKNOWN_ERROR;
	}

	/* Step 4.3: if the random integer just obtained is not odd, add 1 to
	 * it. The random byte string is interpreted as big endian.
	 */
	rsagenpq->rndout[rsagenpq->candidatesz - 1] |= 1;

	/* To avoid useless iterations, set the most significant bit of the
	 * random integer. In fact, the condition in step 4.4 cannot be met when the
	 * most significant bit is 0.
	 */
	rsagenpq->rndout[0] |= 0x80;

	/* The size of p and q must be at least 13 bytes otherwise the condition
	 * in step 5.4 cannot be met. The bit size of the public exponent must be at
	 * least 17 bits otherwise pubexp would be less than 2^16, which violates one
	 * of the conditions in step 2. Note: we are not exactly enforcing the
	 * condition pubexp > 2^16, in fact a pubexp exactly equal to 2^16 would pass
	 * this check.
	 */
	if ((rsagenpq->candidatesz < 13) || (rsagenpq->pubexpsz < 3)) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* The public exponent must be odd. Its size must not be greater than
	 * the key size, which is twice candidatesz. Its most significant byte must not
	 * be zero.
	 */
	if (((rsagenpq->pubexp[rsagenpq->pubexpsz - 1] & 1) == 0) ||
	    (rsagenpq->pubexpsz > (2 * rsagenpq->candidatesz)) || (rsagenpq->pubexp[0] == 0)) {
		return SX_ERR_INVALID_ARG;
	}

	/* pointer p must not be NULL */
	if (rsagenpq->p == NULL) {
		return SX_ERR_INVALID_ARG;
	}

	/* Preliminary check of the condition in steps 4.4 and 5.5: if the
	 * integers under test do not have their most significant bit set, they will
	 * not meet that condition.
	 */
	if ((!(rsagenpq->p[0] & 0x80)) || ((rsagenpq->q != NULL) && !(rsagenpq->q[0] & 0x80))) {
		return SX_ERR_RSA_PQ_RANGE_CHECK_FAIL;
	}

	/* Preliminary primality check: the integers under test must not be
	 * even.
	 */
	if ((!(rsagenpq->p[rsagenpq->candidatesz - 1] & 1)) ||
	    ((rsagenpq->q != NULL) && !(rsagenpq->q[rsagenpq->candidatesz - 1] & 1))) {
		return SX_ERR_COMPOSITE_VALUE;
	}
	struct cracen_rsacheckpq rsacheckpq;

	rsacheckpq.pubexp = rsagenpq->pubexp;
	rsacheckpq.pubexpsz = rsagenpq->pubexpsz;
	rsacheckpq.candidatesz = rsagenpq->candidatesz;
	rsacheckpq.p = rsagenpq->p;
	rsacheckpq.q = rsagenpq->q;

	/* set number of MR rounds as per Table C.2 in FIPS 186.4 (192*8 = 1536)
	 */
	if (rsacheckpq.candidatesz >= 192) {
		rsacheckpq.mrrounds = 4;
	} else {
		rsacheckpq.mrrounds = 5;
	}

	/* big endian unsigned representation of ceil(2^63.5) */
	static const uint8_t compareval[8] = {0xB5, 0x04, 0xF3, 0x33, 0xF9, 0xDE, 0x64, 0x85};
	uint8_t *candprime = get_candidate_prime(&rsacheckpq);
	int r;

	/* Step 4.4 (or 5.5): compare the most significant 64 bits of the
	 * candidate prime with the value in compareval[]. This approximate test is
	 * stricter than the one prescribed in step 4.4 (or 5.5). The probability that
	 * a valid candidate (one that meets the condition in step 4.4) is rejected by
	 * this test is in the order of 2^(-64).
	 */
	r = cracen_be_cmp((const uint8_t *)candprime, compareval, sizeof(compareval), 0);

	if (r < 0) {
		return SX_ERR_RSA_PQ_RANGE_CHECK_FAIL;
	}
	/* step 5.4, to be done only if we are testing q */
	if (rsacheckpq.q != NULL) {
		/* Use of check_abs_diff() with candidatesz as third argument
		 * corresponds to the test in step 5.4 only if p and q have the most
		 * significant bit set, i.e. their bit size is 8*candidatesz. The checks in
		 * the create function enforce this constraint.
		 */
		r = check_abs_diff(rsacheckpq.p, candprime, rsacheckpq.candidatesz);

		if (r == 1) {
			/* reject candidate prime */
			return SX_ERR_RSA_PQ_RANGE_CHECK_FAIL;
		}
	}

	/* preliminary primality check: the candidate prime and the product of
	 * the first 130 odd primes must be coprime
	 */
	size_t candidatesz = rsacheckpq.candidatesz;

	sx_status = coprime_check_run(req, workmem, WORKMEM_SIZE, candprime, candidatesz,
				      product_small_primes, sizeof(product_small_primes));
	if (sx_status != SX_OK) {
		return sx_status;
	}

	uint8_t *wmem = workmem + CHECKPQ_WORKMEM_OFST(candidatesz);

	candprime = get_candidate_prime(&rsacheckpq);

	/* copy the candidate prime under test to workmem */
	memcpy(wmem, candprime, candidatesz);

	/* subtract 1 to the workmem copy of the candidate prime */
	wmem[candidatesz - 1] &= 0xFE;

	/* step 4.5 (or 5.6): candprime - 1 and public exponent must be coprime
	 */
	sx_status = coprime_check_run(req, workmem, WORKMEM_SIZE, wmem, candidatesz,
				      rsacheckpq.pubexp, rsacheckpq.pubexpsz);
	/* the status code value here is SX_ERR_NOT_INVERTIBLE if
	 * (candidate prime - 1) and the public exponent are not coprime
	 */
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* For the Miller Rabin tests, a random integer in [2, candidate prime -
	 * 2] is needed. We can use rndinrange to obtain a random number in
	 * [1, candidate prime - 3] and then add 1 to it, so that the number falls in
	 * the required interval. The candidate prime in wmem was already decreased by
	 * 1, so here we just need to subtract 1 before passing it as the 'upper limit'
	 * to rndinrange.
	 */
	be_sub(wmem, candidatesz, 1);

	return miller_rabin_get_random(req, workmem, &rsacheckpq);
}

/* function to generate an RSA private key, based on FIPS 186-4.
 *
 * Workmem layout:
 *     1. an area of keysz bytes:
 *		- used for the workmem of the 'rsa generate pq'.
 *		- if the key to generate is in the CRT form, this area is also used to
 *		  hold the private exponent d.
 *     2. an area of keysz/2 bytes, used to hold the generated p.
 *     3. an area of keysz/2 bytes, used to hold the generated q.
 *
 * The computation of p and q is based on FIPS 186-4. This function performs steps 1
 * and 2 of section B.3.3 of FIPS 186-4. The remaining steps of that section are
 * performed by the 'rsagenpq_get_random' function.
 */
int cracen_rsa_generate_privkey(sx_pk_req *req, uint8_t *pubexp, size_t pubexpsz,
				size_t keysz, struct cracen_rsa_key *privkey)
{
	int sx_status;
	/* The workmem size requirement is twice the key size. */
	uint8_t workmem[WORKMEM_SIZE];
	uint8_t *wmem = workmem + GENPRIVKEY_WORKMEM_OFST(keysz);

	if (WORKMEM_SIZE < 2 * keysz) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	if ((pubexpsz == 0) || (keysz == 0)) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* step 1: FIPS 186-4 and FIPS 186-5 require keys of at least 2048 bits
	 * (256 bytes) when using for prime generation the "random primes" method.
	 * Moreover, RSA keys smaller that 2048 bits are considered weak by the
	 * security community.
	 */
	if (keysz < 256) {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	/* step 2: the public exponent must be strictly less than 2^256 */
	if (pubexpsz > 32) {
		return SX_ERR_TOO_BIG;
	}

	/* step 2: the public exponent must be strictly greater than 2^16 */
	if ((pubexpsz <= 2) ||
	    ((pubexpsz == 3) && (pubexp[0] == 1) && (pubexp[1] == 0) && (pubexp[2] == 0))) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* The public exponent must be odd (step 2 requirement). Its most
	 * significant byte must not be zero. We restrict keysz to be even.
	 */
	if (((pubexp[pubexpsz - 1] & 1) == 0) || (pubexp[0] == 0) || (keysz & 1)) {
		return SX_ERR_INVALID_ARG;
	}

	/* The public exp must be odd. Its size must not be greater than the key
	 * size. Its most significant byte must not be zero.
	 */
	if (((pubexp[pubexpsz - 1] & 1) == 0) || (pubexpsz > keysz) || (pubexp[0] == 0)) {
		return SX_ERR_INVALID_ARG;
	}

	/* keysz must be even */
	if (keysz & 1) {
		return SX_ERR_INVALID_ARG;
	}
	struct cracen_rsagenpq rsagenpq;
	size_t primefactorsz = keysz >> 1;

	rsagenpq.pubexp = pubexp;
	rsagenpq.pubexpsz = pubexpsz;
	rsagenpq.candidatesz = primefactorsz;
	rsagenpq.p = wmem;
	rsagenpq.q = wmem + primefactorsz;
	rsagenpq.qptr = NULL;

	sx_pk_set_cmd(req, SX_PK_CMD_RSA_KEYGEN);

	/* step 4.1 */
	rsagenpq.attempts = 0;

	/* first we generate p */
	rsagenpq.rndout = wmem;
	rsagenpq.q = NULL;

	do {
		sx_status = rsagenpq_get_random(req, workmem, &rsagenpq);
		if (sx_status == SX_ERR_COMPOSITE_VALUE || sx_status == SX_ERR_NOT_INVERTIBLE ||
		    sx_status == SX_ERR_RSA_PQ_RANGE_CHECK_FAIL) {
			rsagenpq.attempts++;
			if (rsagenpq.attempts >= 5 * (primefactorsz << 3)) {
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		} else if (sx_status != SX_OK) {
			return sx_status;
		} else {
			/* For compliance */
		}
	} while (sx_status != SX_OK);

	rsagenpq.attempts = 0;
	rsagenpq.q = wmem + primefactorsz; /* point at the q-buffer */
	rsagenpq.rndout = rsagenpq.q;	   /* feed the PRNG into q */

	do {
		sx_status = rsagenpq_get_random(req, workmem, &rsagenpq);
		if (sx_status == SX_ERR_COMPOSITE_VALUE || sx_status == SX_ERR_NOT_INVERTIBLE ||
		    sx_status == SX_ERR_RSA_PQ_RANGE_CHECK_FAIL) {
			rsagenpq.attempts++;
			if (rsagenpq.attempts >= 5 * (primefactorsz << 3)) {
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		} else if (sx_status != SX_OK) {
			return sx_status;
		} else {
			/* For compliance */
		}
	} while (sx_status != SX_OK);

	/* Run RSA keygen */
	{
		struct sx_pk_inops_rsa_keygen inputs;
		int sizes[] = {primefactorsz, primefactorsz, pubexpsz};

		sx_pk_set_cmd(req, SX_PK_CMD_RSA_KEYGEN);

		sx_status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
		if (sx_status != SX_OK) {
			return sx_status;
		}

		sx_wrpkmem(inputs.p.addr, wmem, primefactorsz);
		sx_wrpkmem(inputs.q.addr, wmem + primefactorsz, primefactorsz);
		sx_wrpkmem(inputs.e.addr, pubexp, pubexpsz);

		sx_pk_run(req);
		sx_status = sx_pk_wait(req);
		if (sx_status != SX_OK) {
			return sx_status;
		}
	}

	/* Read private exponent d into workmem for CRT params computation */
	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(workmem, outputs[2], keysz);

	if (CRACEN_RSA_KEY_CRT(privkey)) {
		sx_status = rsagenpriv_crt_finish(req, keysz, workmem, privkey);
	} else {
		sx_status = rsagenpriv_finish(req, privkey, keysz);
	}

	return sx_status;
}
