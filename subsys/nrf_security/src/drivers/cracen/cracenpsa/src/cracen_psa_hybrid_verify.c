/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_hybrid_verify.h>

#include <psa/crypto.h>
#include <cracen/common.h>
#include <cracen/hardware.h>
#include <cracen/statuscodes.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <internal/ecc/cracen_ecdsa.h>
#include <internal/ml_dsa/cracen_ml_dsa.h>
#include <nrf_security_mutexes.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <sxsymcrypt/hash.h>

#define CRACEN_HYBRID_VERIFY_IS_MESSAGE (1)

extern nrf_security_mutex_t cracen_mutex_asymmetric;
extern nrf_security_mutex_t cracen_mutex_symmetric;

static void reserve_pke_and_cryptomaster(void)
{
	while (true) {
		nrf_security_mutex_lock(cracen_mutex_asymmetric);

		if (nrf_security_mutex_trylock(cracen_mutex_symmetric) == 0) {
			return;
		}

		nrf_security_mutex_unlock(cracen_mutex_asymmetric);

		nrf_security_mutex_lock(cracen_mutex_symmetric);
		nrf_security_mutex_unlock(cracen_mutex_symmetric);
	}
}

static void release_pke_and_cryptomaster(void)
{
	nrf_security_mutex_unlock(cracen_mutex_symmetric);
	nrf_security_mutex_unlock(cracen_mutex_asymmetric);
}

static psa_status_t prepare_ecdsa(const struct cracen_hybrid_verify_ecdsa *ecdsa,
				  const struct sx_pk_ecurve **curve,
				  const struct sxhashalg **hashalg)
{
	psa_status_t status;
	size_t opsz;

	if (ecdsa->public_key == NULL || ecdsa->signature == NULL ||
	    !PSA_ALG_IS_ECDSA(ecdsa->alg) || !cracen_ecc_curve_is_weierstrass(ecdsa->family)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = cracen_hash_get_algo(PSA_ALG_SIGN_GET_HASH(ecdsa->alg), hashalg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_ecc_get_ecurve_from_psa(ecdsa->family, ecdsa->key_bits, curve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* The digest is held on the stack, and cracen_ecdsa_verify_digest_start()
	 * does not support digests larger than the SHA-512 digest either.
	 */
	if (sx_hash_get_alg_digestsz(*hashalg) > PSA_HASH_MAX_SIZE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	opsz = (size_t)sx_pk_curve_opsize(*curve);
	if (ecdsa->public_key_length != 2 * opsz + 1 ||
	    ecdsa->public_key[0] != CRACEN_ECC_PUBKEY_UNCOMPRESSED ||
	    ecdsa->signature_length != 2 * opsz) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t verify_ml_dsa(const uint8_t *message, size_t message_length,
				  const struct cracen_hybrid_verify_ml_dsa *ml_dsa)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attributes, PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY);
	psa_set_key_bits(&attributes, ml_dsa->key_bits);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attributes, ml_dsa->alg);

	return cracen_ml_dsa_verify(CRACEN_HYBRID_VERIFY_IS_MESSAGE, &attributes,
				    ml_dsa->public_key, ml_dsa->public_key_length, ml_dsa->alg,
				    message, message_length, NULL, 0, ml_dsa->signature,
				    ml_dsa->signature_length);
}

psa_status_t cracen_hybrid_verify(const uint8_t *message, size_t message_length,
				 const struct cracen_hybrid_verify_ml_dsa *ml_dsa,
				 const struct cracen_hybrid_verify_ecdsa *ecdsa)
{
	uint8_t digest[PSA_HASH_MAX_SIZE];
	const struct sx_pk_ecurve *curve = NULL;
	const struct sxhashalg *hashalg = NULL;
	psa_status_t ml_dsa_status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_status_t ecdsa_status = PSA_ERROR_CORRUPTION_DETECTED;
	sx_pk_req request;
	int sx_status;

	if (ml_dsa == NULL || ecdsa == NULL || (message == NULL && message_length != 0) ||
	    ml_dsa->public_key == NULL || ml_dsa->signature == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* The ML-DSA parameter set, the key length and the signature length are
	 * validated by cracen_ml_dsa_verify().
	 */
	ecdsa_status = prepare_ecdsa(ecdsa, &curve, &hashalg);
	if (ecdsa_status != PSA_SUCCESS) {
		return ecdsa_status;
	}

	ml_dsa_status = cracen_init();
	if (ml_dsa_status != PSA_SUCCESS) {
		return ml_dsa_status;
	}

	reserve_pke_and_cryptomaster();

	sx_status = cracen_hash_input(message, message_length, hashalg, digest);
	if (sx_status != SX_OK) {
		release_pke_and_cryptomaster();
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_ecdsa_verify_digest_start(&request, ecdsa->public_key + 1, digest,
						    sx_hash_get_alg_digestsz(hashalg), curve,
						    ecdsa->signature);
	if (sx_status != SX_OK) {
		release_pke_and_cryptomaster();
		return silex_statuscodes_to_psa(sx_status);
	}

	ml_dsa_status = verify_ml_dsa(message, message_length, ml_dsa);
	ecdsa_status = silex_statuscodes_to_psa(cracen_ecdsa_verify_digest_finish(&request));

	release_pke_and_cryptomaster();

	return ecdsa_status != PSA_SUCCESS ? ecdsa_status : ml_dsa_status;
}
