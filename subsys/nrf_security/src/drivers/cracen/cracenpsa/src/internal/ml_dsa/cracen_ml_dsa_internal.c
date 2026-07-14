/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include <cracen_psa_xof.h>
#include <cracen/common.h>
#include <zephyr/toolchain.h>

static __maybe_unused const uint8_t oid_sha256[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01
};

static __maybe_unused const uint8_t oid_sha384[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02
};

static __maybe_unused const uint8_t oid_sha512[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03
};

static __maybe_unused const uint8_t oid_sha3_256[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x08
};

static __maybe_unused const uint8_t oid_sha3_384[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x09
};

static __maybe_unused const uint8_t oid_sha3_512[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0A
};

static __maybe_unused const uint8_t oid_shake128[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0B
};

static __maybe_unused const uint8_t oid_shake256[ML_DSA_HASH_OID_BYTES] = {
	0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0C
};

const uint8_t *cracen_ml_dsa_hash_oid(psa_algorithm_t alg)
{
	switch (PSA_ALG_GET_HASH(alg)) {
	case PSA_ALG_SHA_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_256, (return oid_sha256));
	case PSA_ALG_SHA_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_384, (return oid_sha384));
	case PSA_ALG_SHA_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_512, (return oid_sha512));
	case PSA_ALG_SHA3_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_256, (return oid_sha3_256));
	case PSA_ALG_SHA3_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_384, (return oid_sha3_384));
	case PSA_ALG_SHA3_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_512, (return oid_sha3_512));
	case PSA_ALG_SHAKE128:
		IF_ENABLED(PSA_NEED_CRACEN_SHAKE128, (return oid_shake128));
	case PSA_ALG_SHAKE256:
		IF_ENABLED(PSA_NEED_CRACEN_SHAKE256, (return oid_shake256));
	default:
		return NULL;
	}
}

size_t cracen_ml_dsa_calc_vector_sz_bytes(uint32_t bit_len)
{
	return (size_t)ML_DSA_POLY_COEFFS_COUNT * bit_len / 8;
}

uint32_t cracen_ml_dsa_bit_length(uint32_t x)
{
	if (x == 0) {
		return 0;
	}

	return 32u - __builtin_clz(x);
}

int32_t cracen_ml_dsa_reduce_to_zq(int32_t a)
{
	a += (a >> 31) & ML_DSA_PRIME_NUM;
	if (a >= ML_DSA_PRIME_NUM) {
		a -= ML_DSA_PRIME_NUM;
	}

	return a;
}

int32_t cracen_ml_dsa_abs_coeff(int32_t coeff)
{
	int32_t sign_mask = coeff >> 31;

	return coeff - (sign_mask & (2 * coeff));
}

int32_t cracen_ml_dsa_ge_bound_mask(int32_t magnitude, int32_t bound)
{
	return ~(magnitude - bound) >> 31;
}

psa_status_t cracen_ml_dsa_shake256_digest(const uint8_t *in, size_t in_len,
					   uint8_t *out, size_t out_len)
{
	cracen_xof_operation_t operation;
	psa_status_t status;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, in, in_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, out, out_len);

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}

psa_status_t cracen_ml_dsa_compute_msg_representative(const uint8_t *pk_digest, uint8_t domain,
						      const uint8_t *ctx, size_t ctx_len,
						      const uint8_t *oid, size_t oid_len,
						      const uint8_t *msg, size_t msg_len,
						      uint8_t *msg_representative)
{
	cracen_xof_operation_t operation;
	psa_status_t status;
	uint8_t prefix[2];

	prefix[0] = domain;
	prefix[1] = (uint8_t)ctx_len;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, pk_digest, ML_DSA_PK_DIGEST_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_update(&operation, prefix, sizeof(prefix));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (ctx_len > 0) {
		status = cracen_xof_update(&operation, ctx, ctx_len);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	if (oid_len > 0) {
		status = cracen_xof_update(&operation, oid, oid_len);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	if (msg_len > 0) {
		status = cracen_xof_update(&operation, msg, msg_len);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = cracen_xof_output(&operation, msg_representative, ML_DSA_MSG_RPZTV_SZ_BYTES);

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}

psa_status_t cracen_ml_dsa_compute_commitment_hash(const uint8_t *msg_representative,
						   const uint8_t *commitment,
						   size_t commitment_len,
						   uint8_t *commitment_hash,
						   size_t commitment_hash_len)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_xof_operation_t operation;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, msg_representative, ML_DSA_MSG_RPZTV_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_update(&operation, commitment, commitment_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, commitment_hash, commitment_hash_len);

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}
