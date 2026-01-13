/* ECDSA signature generation and verification.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the ECDSA sign function:
 *      1. Hash digest of the message to be signed (size: digestsz).
 *      2. Output of the rndinrange subfunction (size: curve_op_size, which is the
 *         max size in bytes of parameters and operands for the selected curve).
 *
 * Workmem layout for the ECDSA Deterministic sign function:
 *      1. HMAC function requirements (size: digestsz + blocksz)
 *      2. Hash digest of the message to be signed (size: digestsz).
 *      4. K (HMAC key) (size: digestsz)
 *      5. V (size: digestsz)
 *      6. T (size: curve_op_size)
 *
 * Workmem layout for the ECDSA verify function:
 *      1. Hash digest of the message whose signature is being verified
 *         (size: digestsz).
 */

#include <internal/ecc/cracen_ecdsa.h>

#include <stdint.h>
#include <string.h>
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <sxsymcrypt/hash.h>
#include <cracen_psa_primitives.h>
#include <sxsymcrypt/hashdefs.h>
#include <cracen/common.h>
#include <cracen/cracen_hmac.h>

#define DETERMINISTIC_HMAC_STEPS 6
#define MAX_ECDSA_ATTEMPTS	 255

typedef enum {
	INTERNAL_OCTET_NOT_USED,
	INTERNAL_OCTET_ZERO,
	INTERNAL_OCTET_ONE,
} internal_octet_t;

struct ecdsa_hmac_operation {
	int attempts;
	uint16_t tlen;
	uint8_t step;
	int deterministic_retries;
};

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
static void digest2op(const uint8_t *digest, size_t sz, uint8_t *dst, size_t opsz)
{
	if (opsz > sz) {
		sx_clrpkmem(dst, opsz - sz);
		dst += opsz - sz;
	}
	sx_wrpkmem(dst, digest, opsz);
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
		memmove(out_data + (qbytes - data_len), data, data_len);
	}
}

/**
 * @brief Perform bits2octets according to definition in RFC-6979.
 */
void bits2octets(const uint8_t *data, size_t data_len, uint8_t *out_data, const uint8_t *order,
		 size_t order_len)
{
	bits2int(data, data_len, out_data, order_len * 8 - clz_u8(order[0]));

	int ge = cracen_be_cmp(out_data, order, order_len, 0);

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

static void ecdsa_write_pk(const uint8_t *pubkey, uint8_t *x, uint8_t *y, size_t opsz)
{
	sx_wrpkmem(x, pubkey, opsz);
	sx_wrpkmem(y, pubkey + opsz, opsz);
}

static void ecdsa_write_sig(const struct cracen_const_signature *sig, uint8_t *r, uint8_t *s,
			    size_t opsz)
{
	sx_wrpkmem(r, sig->r, opsz);
	sx_wrpkmem(s, sig->s, opsz);
}

static void ecdsa_write_sk(const struct cracen_ecc_priv_key *sk, uint8_t *d, size_t opsz)
{
	sx_wrpkmem(d, sk->d, opsz);
}

static void ecdsa_read_sig(struct cracen_signature *sig, const uint8_t *r, const uint8_t *s,
			   size_t opsz)
{
	sx_rdpkmem(sig->r, r, opsz);
	if (!sig->s) {
		sig->s = sig->r + opsz;
	}
	sx_rdpkmem(sig->s, s, opsz);
}

static int ecdsa_run_generate_sign(sx_pk_req *req,
				    const struct cracen_ecc_priv_key *privkey,
				    const struct sx_pk_ecurve *curve, const uint8_t *workmem,
				    size_t digestsz, size_t opsz,
				    struct sx_pk_inops_ecdsa_generate *inputs)
{
	int status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)inputs);

	if (status != SX_OK) {
		return status;
	}

	ecdsa_write_sk(privkey, inputs->d.addr, opsz);
	sx_wrpkmem(inputs->k.addr, workmem + digestsz, opsz);
	digest2op(workmem, digestsz, inputs->h.addr, opsz);
	sx_pk_run(req);

	return SX_OK;
}

int cracen_ecdsa_sign_message(const struct cracen_ecc_priv_key *privkey,
			      const struct sxhashalg *hashalg, const struct sx_pk_ecurve *curve,
			      const uint8_t *message, size_t message_length, uint8_t *signature)
{
	int status;
	const size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_ecdsa_sign_digest(privkey, hashalg, curve, digest, digestsz, signature);
}

int cracen_ecdsa_sign_digest(const struct cracen_ecc_priv_key *privkey,
			     const struct sxhashalg *hashalg, const struct sx_pk_ecurve *curve,
			     const uint8_t *digest, size_t digest_length, uint8_t *signature)
{
	int status;
	size_t opsz = sx_pk_curve_opsize(curve);
	sx_pk_req req;
	struct sx_pk_inops_ecdsa_generate inputs;
	const uint8_t *curve_n;
	const size_t workmem_requirement = digest_length + opsz;
	struct cracen_signature internal_signature = {0};
	uint8_t workmem[workmem_requirement];

	/* Checking against the hash algorithm with the largest digest we support */
	if (digest_length > SX_HASH_DIGESTSZ_SHA2_512) {
		return SX_ERR_TOO_BIG;
	}

	memcpy(workmem, digest, digest_length);
	curve_n = sx_pk_curve_order(curve);

	internal_signature.r = signature;
	internal_signature.s = signature + opsz;

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_ECDSA_GEN);

	for (int i = 0; i <= MAX_ECDSA_ATTEMPTS; i++) {
		status = cracen_get_rnd_in_range(curve_n, opsz, workmem + digest_length);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		if (i > 0) {
			sx_pk_set_cmd(&req, SX_PK_CMD_ECDSA_GEN);
		}

		status = ecdsa_run_generate_sign(&req, privkey, curve, workmem,
						 digest_length, opsz, &inputs);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		status = sx_pk_wait(&req);

		/* SX_ERR_NOT_INVERTIBLE may be due to silexpk countermeasures. */
		if ((status == SX_ERR_INVALID_SIGNATURE) || (status == SX_ERR_NOT_INVERTIBLE)) {
			if (i == MAX_ECDSA_ATTEMPTS) {
				sx_pk_release_req(&req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
			/* Continue loop to retry */
		} else {
			break;
		}
	}

	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);

	ecdsa_read_sig(&internal_signature, outputs[0], outputs[1], opsz);
	sx_pk_release_req(&req);
	safe_memzero(workmem, workmem_requirement);

	return SX_OK;
}

static int deterministic_ecdsa_hmac(const struct sxhashalg *hashalg, const uint8_t *key,
				    const uint8_t *v, size_t hash_len, uint8_t internal_octet,
				    const uint8_t *sk, const uint8_t *hash, size_t key_len,
				    uint8_t *hmac)
{
	int status;
	struct sxhash hashopctx;
	const size_t workmem_requirement =
		sx_hash_get_alg_digestsz(hashalg) + sx_hash_get_alg_blocksz(hashalg);
	uint8_t workmem[workmem_requirement];

	status = mac_create_hmac(hashalg, &hashopctx, key, hash_len, workmem, workmem_requirement);
	if (status != SX_OK) {
		return status;
	}
	/* The hash context is initialized in mac_create_hmac */
	status = sx_hash_feed(&hashopctx, v, hash_len);
	if (status != SX_OK) {
		return status;
	}

	if (internal_octet != INTERNAL_OCTET_NOT_USED) {
		static uint8_t internal_octet_values[] = {
			[INTERNAL_OCTET_ZERO] = 0,
			[INTERNAL_OCTET_ONE] = 1,
		};
		status = sx_hash_feed(&hashopctx, &internal_octet_values[internal_octet],
				      sizeof(*internal_octet_values));
		if (status != SX_OK) {
			return status;
		}
	}
	if (sk) {
		status = sx_hash_feed(&hashopctx, sk, key_len);
		if (status != SX_OK) {
			return status;
		}
	}
	if (hash) {
		status = sx_hash_feed(&hashopctx, hash, key_len);
		if (status != SX_OK) {
			return status;
		}
	}

	return hmac_produce(&hashopctx, hashalg, hmac, hash_len, workmem);
}

static int run_deterministic_ecdsa_hmac_step(const struct sxhashalg *hashalg, size_t opsz,
					     const uint8_t *curve_n, const size_t digestsz,
					     size_t blocksz, uint8_t *workmem,
					     struct ecdsa_hmac_operation *hmac_op,
					     const struct cracen_ecc_priv_key *privkey)
{
	int status = SX_OK;
	uint8_t *h1 = &workmem[digestsz + blocksz];
	uint8_t *K = h1 + digestsz;
	uint8_t *V = K + digestsz;
	uint8_t *T = V + digestsz;
	size_t copylen;

	switch (hmac_op->step) {
	case 0: /* K = HMAC_K(V || 0x00 || privkey || h1) */
		for (size_t i = 0; i < digestsz; i++) {
			K[i] = 0x0; /* Initialize K = 0x00 0x00 ... */
			V[i] = 0x1; /* Initialize V = 0x01 0x01 ... */
		}

		/* The original h1 must be preserved for the sign operation. We reuse T for the
		 * transformed value.
		 */
		bits2octets(h1, digestsz, T, curve_n, opsz);

		status = deterministic_ecdsa_hmac(hashalg, K, V, digestsz, INTERNAL_OCTET_ZERO,
						  privkey->d, T, opsz, K);
		break;

	case 1: /* V = HMAC_K(V) */
		status = deterministic_ecdsa_hmac(hashalg, K, V, digestsz, INTERNAL_OCTET_NOT_USED,
						  NULL, NULL, opsz, V);
		break;

	case 2: /* K = HMAC_K(V || 0x01 || privkey || h1) */
		status = deterministic_ecdsa_hmac(hashalg, K, V, digestsz, INTERNAL_OCTET_ONE,
						  privkey->d, T, opsz, K);
		break;

	case 3: /* V = HMAC_K(V) */
	case 4: /* same as case 3. */
		status = deterministic_ecdsa_hmac(hashalg, K, V, digestsz, INTERNAL_OCTET_NOT_USED,
						  NULL, NULL, opsz, V);
		break;

	case 5: /* T = T || V */
		copylen = MIN(digestsz, opsz - hmac_op->tlen);

		memcpy(T + hmac_op->tlen, V, copylen);
		hmac_op->tlen += copylen;
		if (hmac_op->tlen < opsz) {
			/* We need more data. */
			hmac_op->step = 4;
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
		int ge = cracen_be_cmp(T, curve_n, opsz, 0);
		bool must_retry =
			hmac_op->deterministic_retries < (MAX_ECDSA_ATTEMPTS - hmac_op->attempts);
		if (is_zero || ge >= 0 || must_retry) {
			/* T is out of range. Retry */
			hmac_op->step = 3;
			hmac_op->tlen = 0;
			hmac_op->deterministic_retries++;
			/* K = HMAC_K(V || 0x00) */
			deterministic_ecdsa_hmac(hashalg, K, V, digestsz, INTERNAL_OCTET_ZERO, NULL,
						 NULL, 0, K);
			return SX_ERR_HW_PROCESSING;
		}
		memcpy(workmem, h1, digestsz);
		memcpy(workmem + digestsz, T, opsz);
		break;
	}
	default:
		return SX_ERR_INVALID_PARAM;
	}
	hmac_op->step++;
	return status;
}

int cracen_ecdsa_sign_digest_deterministic(const struct cracen_ecc_priv_key *privkey,
					   const struct sxhashalg *hashalg,
					   const struct sx_pk_ecurve *curve, const uint8_t *digest,
					   const size_t digestsz, uint8_t *signature)
{
	int status;
	size_t opsz = (size_t)sx_pk_curve_opsize(curve);
	size_t blocksz = sx_hash_get_alg_blocksz(hashalg);
	const size_t workmem_requirement = digestsz * 4 + opsz + blocksz;
	const uint8_t *curve_n = sx_pk_curve_order(curve);
	uint8_t workmem[workmem_requirement];

	sx_pk_req req;
	struct sx_pk_inops_ecdsa_generate inputs;
	struct cracen_signature internal_signature = {0};
	struct ecdsa_hmac_operation hmac_op;

	hmac_op.attempts = MAX_ECDSA_ATTEMPTS;

	internal_signature.r = signature;
	internal_signature.s = signature + opsz;
	memcpy(workmem, digest, digestsz);

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_ECDSA_GEN);

	do {
		hmac_op.deterministic_retries = 0;
		hmac_op.step = 0;
		hmac_op.tlen = 0;
		uint8_t *h1 = workmem + digestsz + blocksz;

		memcpy(h1, workmem, digestsz);
		while (hmac_op.step <= DETERMINISTIC_HMAC_STEPS) {
			status = run_deterministic_ecdsa_hmac_step(hashalg, opsz, curve_n, digestsz,
								   blocksz, workmem, &hmac_op,
								   privkey);
			if (status != SX_OK && status != SX_ERR_HW_PROCESSING) {
				sx_pk_release_req(&req);
				return status;
			}
		}

		status = ecdsa_run_generate_sign(&req, privkey, curve, workmem, digestsz,
						 opsz, &inputs);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		status = sx_pk_wait(&req);

	} while (--hmac_op.attempts &&
		 (status == SX_ERR_INVALID_SIGNATURE || status == SX_ERR_NOT_INVERTIBLE));

	if (status == SX_OK) {
		const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);

		ecdsa_read_sig(&internal_signature, outputs[0], outputs[1], opsz);
	}

	sx_pk_release_req(&req);
	safe_memzero(workmem, workmem_requirement);

	return status;
}

int cracen_ecdsa_sign_message_deterministic(const struct cracen_ecc_priv_key *privkey,
					    const struct sxhashalg *hashalg,
					    const struct sx_pk_ecurve *curve,
					    const uint8_t *message, size_t message_length,
					    uint8_t *signature)
{
	int status;
	const size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_ecdsa_sign_digest_deterministic(privkey, hashalg, curve, digest, digestsz,
						      signature);
}

int cracen_ecdsa_verify_message(const uint8_t *pubkey, const struct sxhashalg *hashalg,
				const uint8_t *message, size_t message_length,
				const struct sx_pk_ecurve *curve, const uint8_t *signature)
{
	int status;
	const size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_ecdsa_verify_digest(pubkey, digest, digestsz, curve, signature);
}

int cracen_ecdsa_verify_digest(const uint8_t *pubkey, const uint8_t *digest, const size_t digestsz,
			       const struct sx_pk_ecurve *curve, const uint8_t *signature)
{
	int status;
	size_t opsz = sx_pk_curve_opsize(curve);

	sx_pk_req req;
	struct sx_pk_inops_ecdsa_verify inputs;
	struct cracen_const_signature internal_signature = {.r = signature, .s = signature + opsz};

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_ECDSA_VER);
	status = sx_pk_list_ecc_inslots(&req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	opsz = sx_pk_curve_opsize(curve);
	ecdsa_write_pk(pubkey, inputs.qx.addr, inputs.qy.addr, opsz);
	ecdsa_write_sig(&internal_signature, inputs.r.addr, inputs.s.addr, opsz);

	digest2op(digest, digestsz, inputs.h.addr, opsz);
	sx_pk_run(&req);
	status = sx_pk_wait(&req);
	sx_pk_release_req(&req);

	return status;
}
