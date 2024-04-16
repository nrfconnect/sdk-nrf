/* ECDSA signature generation and verification.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the ECDSA sign task:
 *      1. Hash digest of the message to be signed (size: digestsz).
 *      2. Output of the rndinrange subtask (size: curve_op_size, which is the
 *         max size in bytes of parameters and operands for the selected curve).
 *
 * Workmem layout for the ECDSA Deterministic sign task:
 *      1. HMAC task requirements (size: digestsz + blocksz)
 *      2. Hash digest of the message to be signed (size: digestsz).
 *      4. K (HMAC key) (size: digestsz)
 *      5. V (size: digestsz)
 *      6. T (size: curve_op_size)
 *
 * Workmem layout for the ECDSA verify task:
 *      1. Hash digest of the message whose signature is being verified
 *         (size: digestsz).
 */

#include <stdint.h>
#include <string.h>
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <silexpk/cmddefs/ecc.h>
#include <sxsymcrypt/sha2.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/ecdsa.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "rndinrange.h"
#include "sicrypto/internal.h"
#include "sxsymcrypt/hash.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"
#include "sigdefs.h"
#include <sicrypto/hmac.h>

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define ROUND_UP(x, align)                                                                         \
	(((unsigned long)(x) + ((unsigned long)(align)-1)) & ~((unsigned long)(align)-1))
#define INTERNAL_OCTET_NOT_USED ((uint8_t)0xFFu)

#ifndef MAX_ECDSA_ATTEMPTS
#define MAX_ECDSA_ATTEMPTS 255
#endif

/* Counts leading zeroes in a u8 */
static int clz_u8(uint8_t i)
{
	int r = 0;

	while (((1 << (7 - r)) & i) == 0) {
		r++;
	}
	return r;
}

/** Convert a digest into an operand for ECDSA
 *
 * The raw digest may need to be padded or truncated to fit the curve
 * operand used for ECDSA.
 *
 * Conversion could also imply byte order inversion, but that is not
 * implemented here. It's expected that SilexPK takes big endian
 * operands here.
 *
 * As the digest size is expressed in bytes, this procedure does not
 * work for curves which have sizes not multiples of 8 bits.
 */
static void digest2op(const char *digest, size_t sz, char *dst, size_t opsz)
{
	if (opsz > sz) {
		sx_clrpkmem(dst, opsz - sz);
		dst += opsz - sz;
	}
	sx_wrpkmem(dst, digest, opsz);
}

static void si_ecdsa_write_sk(const struct si_eccsk *sk, char *d, size_t opsz)
{
	sx_wrpkmem(d, sk->d, opsz);
}

static void si_ecdsa_write_pk(const struct si_eccpk *pk, char *x, char *y, size_t opsz)
{
	sx_wrpkmem(x, pk->qx, opsz);
	sx_wrpkmem(y, pk->qy, opsz);
}

static void si_ecdsa_write_sig(const struct si_sig_signature *sig, char *r, char *s, size_t opsz)
{
	sx_wrpkmem(r, sig->r, opsz);
	sx_wrpkmem(s, sig->s, opsz);
}

static void si_ecdsa_read_sig(struct si_sig_signature *sig, const char *r, const char *s,
			      size_t opsz)
{
	sx_rdpkmem(sig->r, r, opsz);
	if (!sig->s) {
		sig->s = sig->r + opsz;
	}
	sx_rdpkmem(sig->s, s, opsz);
}

static int finish_ecdsa_ver(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	sx_pk_release_req(t->pk);
	return t->statuscode;
}

static void run_ecdsa_ver(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_verify inputs;
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_ver.pubkey->hashalg);
	int opsz;

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECDSA_VER);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}
	pkreq.status =
		sx_pk_list_ecc_inslots(pkreq.req, t->params.ecdsa_ver.pubkey->key.eckey.curve, 0,
				       (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}
	t->pk = pkreq.req;
	opsz = sx_pk_curve_opsize(t->params.ecdsa_ver.pubkey->key.eckey.curve);
	si_ecdsa_write_pk(&t->params.ecdsa_ver.pubkey->key.eckey, inputs.qx.addr, inputs.qy.addr,
			  opsz);
	si_ecdsa_write_sig(t->params.ecdsa_ver.signature, inputs.r.addr, inputs.s.addr, opsz);
	digest2op(t->workmem, digestsz, inputs.h.addr, opsz);
	sx_pk_run(pkreq.req);
	t->statuscode = SX_ERR_HW_PROCESSING;
	si_wq_run_after(t, &t->params.ecdsa_ver.wq, finish_ecdsa_ver);
}

static void ecdsa_hash_message(struct sitask *t)
{
	/* Override the statuscode value to be able to run si_task_produce(). */
	t->statuscode = SX_ERR_READY;
	si_task_produce(t, t->workmem, sx_hash_get_digestsz(&t->u.h));
}

static void ecdsa_ver_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_ver.pubkey->hashalg);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(t->workmem, data, sz);

	t->actions.consume = NULL;
	t->actions.run = run_ecdsa_ver;
}

static int ecdsa_verify_init(struct sitask *t, const struct si_sig_pubkey *pubkey,
			     const struct si_sig_signature *signature)
{
	if (t->workmemsz < sx_hash_get_alg_digestsz(pubkey->hashalg)) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	t->params.ecdsa_ver.pubkey = pubkey;
	t->params.ecdsa_ver.signature = signature;

	return SX_OK;
}

static void si_sig_create_ecdsa_verify_digest(struct sitask *t, const struct si_sig_pubkey *pubkey,
					      const struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = ecdsa_ver_consume_digest;

	ecdsa_verify_init(t, pubkey, signature);
}

static int ecdsa_ver_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	run_ecdsa_ver(t);

	return t->statuscode;
}

static void si_sig_create_ecdsa_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
				       const struct si_sig_signature *signature)
{
	int r;

	r = ecdsa_verify_init(t, pubkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.ecdsa_ver.wq, ecdsa_ver_start);
	si_hash_create(t, pubkey->hashalg);
	t->actions.run = ecdsa_hash_message;
}

static void run_ecdsa_sign_rnd(struct sitask *t);
static void run_ecdsa_sign_deterministic(struct sitask *t);

static int finish_ecdsa_sign(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	/* SX_ERR_NOT_INVERTIBLE may be due to silexpk countermeasures. */
	if (((t->statuscode == SX_ERR_INVALID_SIGNATURE) ||
	     (t->statuscode == SX_ERR_NOT_INVERTIBLE)) &&
	    (t->params.ecdsa_sign.attempts--)) {
		sx_pk_release_req(t->pk);
		if (t->params.ecdsa_sign.deterministic) {
			run_ecdsa_sign_deterministic(t);
		} else {
			run_ecdsa_sign_rnd(t);
		}
		return SX_ERR_HW_PROCESSING;
	}

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_curve_opsize(t->params.ecdsa_sign.privkey->key.eckey.curve);

	si_ecdsa_read_sig(t->params.ecdsa_sign.signature, outputs[0], outputs[1], opsz);
	sx_pk_release_req(t->pk);

	return SX_OK;
}

static int start_ecdsa_sign(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_generate inputs;
	int opsz;
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_sign.privkey->hashalg);
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECDSA_GEN);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	pkreq.status =
		sx_pk_list_ecc_inslots(pkreq.req, t->params.ecdsa_sign.privkey->key.eckey.curve, 0,
				       (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	t->pk = pkreq.req;
	opsz = sx_pk_curve_opsize(t->params.ecdsa_sign.privkey->key.eckey.curve);
	si_ecdsa_write_sk(&t->params.ecdsa_sign.privkey->key.eckey, inputs.d.addr, opsz);
	sx_wrpkmem(inputs.k.addr, t->workmem + digestsz, opsz);
	digest2op(t->workmem, digestsz, inputs.h.addr, opsz);
	sx_pk_run(pkreq.req);
	t->statuscode = SX_ERR_HW_PROCESSING;
	si_wq_run_after(t, &t->params.ecdsa_sign.wq, finish_ecdsa_sign);

	return SX_ERR_HW_PROCESSING;
}

static void run_ecdsa_sign_rnd(struct sitask *t)
{
	int opsz;
	const char *curve_n;
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_sign.privkey->hashalg);

	opsz = sx_pk_curve_opsize(t->params.ecdsa_sign.privkey->key.eckey.curve);
	curve_n = sx_pk_curve_order(t->params.ecdsa_sign.privkey->key.eckey.curve);

	si_wq_run_after(t, &t->params.ecdsa_sign.wq, start_ecdsa_sign);

	si_rndinrange_create(t, (const unsigned char *)curve_n, opsz, t->workmem + digestsz);
	si_task_run(t);
}

static void deterministic_ecdsa_hmac(struct sitask *t, const struct sxhashalg *hashalg,
				     uint8_t *key, const uint8_t *v, size_t hash_len,
				     uint8_t internal_octet, uint8_t *sk, uint8_t *hash,
				     size_t key_len, uint8_t *hmac)
{
	si_mac_create_hmac(t, hashalg, key, hash_len);
	si_task_consume(t, v, hash_len);

	if (internal_octet != INTERNAL_OCTET_NOT_USED) {
		si_task_consume(t, &internal_octet, sizeof(internal_octet));
	}
	if (sk) {
		si_task_consume(t, sk, key_len);
	}
	if (hash) {
		si_task_consume(t, hash, key_len);
	}

	si_task_produce(t, hmac, hash_len);
	si_task_run(t);
}

/**
 * @brief Perform bits2int according to definition in RFC-6979.
 */
void bits2int(const uint8_t *data, size_t data_len, uint8_t *out_data, size_t qlen)
{
	size_t data_bitlen = data_len * 8;
	size_t qbytes = ROUND_UP(qlen, 8) / 8;

	if (data_bitlen > qlen) {
		uint32_t rshift = qbytes * 8 - qlen;

		memmove(out_data, data, qbytes);

		if (rshift) {
			uint8_t prev = 0;

			for (size_t i = 0; i < qbytes; i++) {
				uint8_t tmp = out_data[i];

				out_data[i] = prev << (8 - rshift) | (tmp >> rshift);
				prev = tmp;
			}
		}

	} else {
		memset(out_data, 0, qbytes - data_len);
		memmove(out_data + (qbytes - data_len), data, qbytes);
	}
}

/**
 * @brief Perform bits2octects according to definition in RFC-6979.
 */
void bits2octets(const uint8_t *data, size_t data_len, uint8_t *out_data, const uint8_t *order,
		 size_t order_len)
{
	bits2int(data, data_len, out_data, order_len * 8 - clz_u8(order[0]));

	int ge = si_be_cmp(out_data, order, order_len, 0);

	if (ge >= 0) {
		uint8_t carry = 0;

		for (size_t i = order_len; i > 0; i--) {
			uint32_t a = out_data[i - 1];
			uint32_t b = order[i - 1] + carry;

			if (b > a) {
				a += 0x100;
				carry = 1;
			} else {
				carry = 0;
			}
			out_data[i - 1] = a - b;
		}
	}
}

static int run_deterministic_ecdsa_hmac_step(struct sitask *t, struct siwq *wq)
{
	int opsz;
	const char *curve_n;
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_sign.privkey->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->params.ecdsa_sign.privkey->hashalg);

	opsz = sx_pk_curve_opsize(t->params.ecdsa_sign.privkey->key.eckey.curve);
	curve_n = sx_pk_curve_order(t->params.ecdsa_sign.privkey->key.eckey.curve);

	uint8_t *h1 = t->workmem + digestsz + blocksz;
	uint8_t *K = h1 + digestsz;
	uint8_t *V = K + digestsz;
	uint8_t *T = V + digestsz;
	uint8_t step = t->params.ecdsa_sign.deterministic_hmac_step;

	switch (step) {
	case 0: /* K = HMAC_K(V || 0x00 || privkey || h1) */
		for (size_t i = 0; i < digestsz; i++) {
			K[i] = 0x0; /* Initialize K = 0x00 0x00 ... */
			V[i] = 0x1; /* Initialize V = 0x01 0x01 ... */
		}

		/* The original h1 must be preserved for the sign operation. We reuse T for the
		 * transformed value.
		 */
		bits2octets(h1, digestsz, T, curve_n, opsz);

		si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);
		deterministic_ecdsa_hmac(t, t->params.ecdsa_sign.privkey->hashalg, K, V, digestsz,
					 0, t->params.ecdsa_sign.privkey->key.eckey.d, T, opsz, K);
		break;

	case 1: /* V = HMAC_K(V) */
		si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);
		deterministic_ecdsa_hmac(t, t->params.ecdsa_sign.privkey->hashalg, K, V, digestsz,
					 INTERNAL_OCTET_NOT_USED, NULL, NULL, opsz, V);
		break;

	case 2: /* K = HMAC_K(V || 0x01 || privkey || h1) */
		si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);
		deterministic_ecdsa_hmac(t, t->params.ecdsa_sign.privkey->hashalg, K, V, digestsz,
					 1, t->params.ecdsa_sign.privkey->key.eckey.d, T, opsz, K);
		break;

	case 3: /* V = HMAC_K(V) */
	case 4: /* same as case 3. */
		si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);
		deterministic_ecdsa_hmac(t, t->params.ecdsa_sign.privkey->hashalg, K, V, digestsz,
					 INTERNAL_OCTET_NOT_USED, NULL, NULL, opsz, V);
		break;

	case 5: /* T = T || V */
		si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);

		size_t copylen = MIN(digestsz, opsz - t->params.ecdsa_sign.tlen);

		memcpy(T + t->params.ecdsa_sign.tlen, V, copylen);
		t->params.ecdsa_sign.tlen += copylen;
		if (t->params.ecdsa_sign.tlen < opsz) {
			/* We need more data. */
			t->params.ecdsa_sign.deterministic_hmac_step = 4;
			return SX_ERR_HW_PROCESSING;
		}
		break;

	case 6: /* Verify that T is in range [1, q-1] */
	{
		int rshift = clz_u8(curve_n[0]);

		/* Only use the first qlen bits that was generated. */
		bits2int(T, opsz, T, opsz * 8 - rshift);

		bool is_zero = true;

		for (size_t i = 0; i < opsz; i++) {
			if (T[i]) {
				is_zero = false;
				break;
			}
		}
		int ge = si_be_cmp(T, curve_n, opsz, 0);
		bool must_retry = t->params.ecdsa_sign.deterministic_retries <
				  (MAX_ECDSA_ATTEMPTS - t->params.ecdsa_sign.attempts);
		if (is_zero || ge >= 0 || must_retry) {
			/* T is out of range. Retry */
			si_wq_run_after(t, wq, run_deterministic_ecdsa_hmac_step);
			t->params.ecdsa_sign.deterministic_hmac_step = 3;
			t->params.ecdsa_sign.tlen = 0;
			t->params.ecdsa_sign.deterministic_retries++;
			/* K = HMAC_K(V || 0x00) */
			deterministic_ecdsa_hmac(t, t->params.ecdsa_sign.privkey->hashalg, K, V,
						 digestsz, 0, NULL, NULL, 0, K);
			return SX_ERR_HW_PROCESSING;
		}

		si_wq_run_after(t, wq, start_ecdsa_sign);

		/* Copy parameters to start_ecdsa_sign. .*/
		memcpy(t->workmem, h1, digestsz);
		memcpy(t->workmem + digestsz, T, opsz);
	}
	}

	t->params.ecdsa_sign.deterministic_hmac_step++;

	return SX_ERR_HW_PROCESSING;
}

static void run_ecdsa_sign_deterministic(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_sign.privkey->hashalg);
	size_t blocksz = sx_hash_get_alg_blocksz(t->params.ecdsa_sign.privkey->hashalg);

	uint8_t *h1 = t->workmem + digestsz + blocksz;

	memcpy(h1, t->workmem, digestsz);

	t->params.ecdsa_sign.deterministic_retries = 0;
	t->params.ecdsa_sign.deterministic_hmac_step = 0;
	t->params.ecdsa_sign.tlen = 0;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	run_deterministic_ecdsa_hmac_step(t, &t->params.ecdsa_sign.wq);
}

static void ecdsa_sign_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->params.ecdsa_sign.privkey->hashalg);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(t->workmem, data, sz);

	t->actions.consume = NULL;
	if (t->params.ecdsa_sign.deterministic) {
		t->actions.run = run_ecdsa_sign_deterministic;
	} else {
		t->actions.run = run_ecdsa_sign_rnd;
	}
}

static int ecdsa_sign_init(struct sitask *t, const struct si_sig_privkey *privkey,
			   struct si_sig_signature *signature)
{
	size_t digestsz = sx_hash_get_alg_digestsz(privkey->hashalg);
	size_t opsz = (size_t)sx_pk_curve_opsize(privkey->key.eckey.curve);
	size_t blocksz = sx_hash_get_alg_blocksz(privkey->hashalg);

	size_t workmem_requirement = digestsz + opsz;

	if (t->params.ecdsa_sign.deterministic) {
		workmem_requirement += digestsz * 3 + blocksz;
	}

	if (t->workmemsz < workmem_requirement) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	t->params.ecdsa_sign.privkey = privkey;
	t->params.ecdsa_sign.signature = signature;
	t->params.ecdsa_sign.attempts = MAX_ECDSA_ATTEMPTS;

	return SX_OK;
}

static void si_sig_create_ecdsa_sign_digest(struct sitask *t, const struct si_sig_privkey *privkey,
					    struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = ecdsa_sign_consume_digest;
	t->params.ecdsa_sign.deterministic = false;

	ecdsa_sign_init(t, privkey, signature);
}

static void si_sig_create_ecdsa_sign_digest_deterministic(struct sitask *t,
							  const struct si_sig_privkey *privkey,
							  struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = ecdsa_sign_consume_digest;
	t->params.ecdsa_sign.deterministic = true;

	ecdsa_sign_init(t, privkey, signature);
}

static int ecdsa_sign_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	if (t->params.ecdsa_sign.deterministic) {
		run_ecdsa_sign_deterministic(t);
	} else {
		run_ecdsa_sign_rnd(t);
	}

	return t->statuscode;
}

static void si_sig_create_ecdsa_sign(struct sitask *t, const struct si_sig_privkey *privkey,
				     struct si_sig_signature *signature)
{
	int r;

	r = ecdsa_sign_init(t, privkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.ecdsa_sign.wq, ecdsa_sign_start);
	si_hash_create(t, privkey->hashalg);
	t->actions.run = ecdsa_hash_message;
	t->params.ecdsa_sign.deterministic = false;
}

static void si_sig_create_ecdsa_sign_deterministic(struct sitask *t,
						   const struct si_sig_privkey *privkey,
						   struct si_sig_signature *signature)
{
	int r;

	r = ecdsa_sign_init(t, privkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.ecdsa_sign.wq, ecdsa_sign_start);
	si_hash_create(t, privkey->hashalg);
	t->actions.run = ecdsa_hash_message;
	t->params.ecdsa_sign.deterministic = true;
}

static void create_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
			  struct si_sig_pubkey *pubkey)
{
	pubkey->hashalg = privkey->hashalg;
	si_ecc_create_genpubkey(t, &privkey->key.eckey, &pubkey->key.eckey);
}

static unsigned short int get_key_opsz(const struct si_sig_privkey *privkey)
{
	return sx_pk_curve_opsize(privkey->key.eckey.curve);
}

const struct si_sig_def *const si_sig_def_ecdsa = &(const struct si_sig_def){
	.sign = si_sig_create_ecdsa_sign,
	.sign_digest = si_sig_create_ecdsa_sign_digest,
	.verify = si_sig_create_ecdsa_verify,
	.verify_digest = si_sig_create_ecdsa_verify_digest,
	.pubkey = create_pubkey,
	.getkeyopsz = get_key_opsz,
	.sigcomponents = 2,
};

const struct si_sig_def *const si_sig_def_ecdsa_deterministic = &(const struct si_sig_def){
	.sign = si_sig_create_ecdsa_sign_deterministic,
	.sign_digest = si_sig_create_ecdsa_sign_digest_deterministic,
	.verify = si_sig_create_ecdsa_verify,
	.verify_digest = si_sig_create_ecdsa_verify_digest,
	.pubkey = create_pubkey,
	.getkeyopsz = get_key_opsz,
	.sigcomponents = 2,
};
