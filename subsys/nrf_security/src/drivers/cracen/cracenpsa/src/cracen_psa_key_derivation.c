/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_key_derivation.h>

#include <cracen/common.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <internal/ecdh/cracen_ecdh_weierstrass.h>
#include <internal/ecdh/cracen_ecdh_montgomery.h>
#include <internal/key_derivation/cracen_hkdf.h>
#include <internal/key_derivation/cracen_pbkdf2_hmac.h>
#include <internal/key_derivation/cracen_sp800_108_ctr_mac.h>
#include <internal/key_derivation/cracen_tls12.h>
#include <internal/key_derivation/cracen_wpa3_sae_h2e.h>
#include <internal/key_derivation/cracen_srp_password_hash.h>
#include <cracen_psa_primitives.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <stdint.h>
#include <string.h>

static psa_status_t ecc_key_agreement_check_alg(psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (PSA_ALG_IS_RAW_KEY_AGREEMENT(alg) && PSA_ALG_IS_ECDH(alg)) {
		IF_ENABLED(PSA_NEED_CRACEN_ECDH_MONTGOMERY, (status = PSA_SUCCESS));
		IF_ENABLED(PSA_NEED_CRACEN_ECDH_SECP_R1, (status = PSA_SUCCESS));
	} else if (PSA_ALG_IS_ECDH(alg) || PSA_ALG_IS_FFDH(alg)) {
		status = PSA_ERROR_NOT_SUPPORTED;
	} else {
		status = PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

psa_status_t cracen_key_derivation_setup(cracen_key_derivation_operation_t *operation,
					 psa_algorithm_t alg)
{
	operation->alg = alg;

#if defined(PSA_NEED_CRACEN_HKDF)
	if (PSA_ALG_IS_HKDF(operation->alg)	   ||
	    PSA_ALG_IS_HKDF_EXPAND(operation->alg) ||
	    PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		return cracen_hkdf_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_HKDF */

#if defined(PSA_NEED_CRACEN_PBKDF2_HMAC)
	if (PSA_ALG_IS_PBKDF2(operation->alg)) {
		return cracen_pbkdf2_hmac_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_PBKDF2_HMAC */

#if defined(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
	if (operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
		return cracen_tls12_ecjpake_to_pms_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS */

#if defined(PSA_NEED_CRACEN_TLS12_PRF)
	if (PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		return cracen_tls12_prf_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PRF */

#if defined(PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
	if (PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		return cracen_tls12_psk_to_ms_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
	if (PSA_ALG_IS_SRP_PASSWORD_HASH(alg)) {
		return cracen_srp_password_hash_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_SRP_PASSWORD_HASH */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		return cracen_sp800_108_ctr_mac_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
	if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		return cracen_sp800_108_ctr_mac_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */

#if defined(PSA_NEED_CRACEN_WPA3_SAE_H2E)
	if (PSA_ALG_IS_WPA3_SAE_H2E(alg)) {
		return cracen_wpa3_sae_h2e_setup(operation);
	}
#endif /* PSA_NEED_CRACEN_WPA3_SAE_H2E */

	return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t cracen_key_derivation_set_capacity(cracen_key_derivation_operation_t *operation,
						size_t capacity)
{
	if (operation->state == CRACEN_KD_STATE_INVALID) {
		return PSA_ERROR_BAD_STATE;
	}

	if (capacity > operation->capacity) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	operation->capacity = capacity;

	return PSA_SUCCESS;
}

psa_status_t cracen_key_derivation_input_bytes(cracen_key_derivation_operation_t *operation,
					       psa_key_derivation_step_t step, const uint8_t *data,
					       size_t data_length)
{
#if defined(PSA_NEED_CRACEN_HKDF)
	if (PSA_ALG_IS_HKDF(operation->alg)	   ||
	    PSA_ALG_IS_HKDF_EXPAND(operation->alg) ||
	    PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		return cracen_hkdf_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_HKDF */

#if defined(PSA_NEED_CRACEN_PBKDF2_HMAC)
	if (PSA_ALG_IS_PBKDF2(operation->alg)) {
		return cracen_pbkdf2_hmac_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_PBKDF2_HMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		return cracen_sp800_108_ctr_mac_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
	if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		return cracen_sp800_108_ctr_mac_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */

#if defined(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
	if (operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
		return cracen_tls12_ecjpake_to_pms_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS */

#if defined(PSA_NEED_CRACEN_TLS12_PRF)
	if (PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		return cracen_tls12_prf_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PRF */

#if defined(PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
	if (PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		return cracen_tls12_psk_to_ms_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
	if (PSA_ALG_IS_SRP_PASSWORD_HASH(operation->alg)) {
		return cracen_srp_password_hash_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_SRP_PASSWORD_HASH */

#if defined(PSA_NEED_CRACEN_WPA3_SAE_H2E)
	if (PSA_ALG_IS_WPA3_SAE_H2E(operation->alg)) {
		return cracen_wpa3_sae_h2e_input_bytes(operation, step, data, data_length);
	}
#endif /* PSA_NEED_CRACEN_WPA3_SAE_H2E */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_input_key(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size)
{
	if (operation->alg != PSA_ALG_SP800_108_COUNTER_CMAC &&
	    !PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		return cracen_key_derivation_input_bytes(operation, step, key_buffer,
							 key_buffer_size);
	}

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		return cracen_sp800_108_ctr_cmac_input_key(operation, step, attributes,
							   key_buffer, key_buffer_size);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
	if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		return cracen_sp800_108_ctr_hmac_input_key(operation, step, attributes,
							   key_buffer, key_buffer_size);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_input_integer(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step, uint64_t value)
{
#if defined(PSA_NEED_CRACEN_PBKDF2_HMAC)
	if (PSA_ALG_IS_PBKDF2(operation->alg)) {
		return cracen_pbkdf2_hmac_input_integer(operation, step, value);
	}
#endif /* PSA_NEED_CRACEN_PBKDF2_HMAC */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_agreement(const psa_key_attributes_t *attributes, const uint8_t *priv_key,
				  size_t priv_key_size, const uint8_t *publ_key,
				  size_t publ_key_size, uint8_t *output, size_t output_size,
				  size_t *output_length, psa_algorithm_t alg)
{
	psa_ecc_family_t curve_family = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	size_t curve_bits = psa_get_key_bits(attributes);
	psa_status_t psa_status;
	const struct sx_pk_ecurve *curve;

	*output_length = 0;

	psa_status = ecc_key_agreement_check_alg(alg);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status = cracen_ecc_get_ecurve_from_psa(curve_family, curve_bits, &curve);
	if (psa_status != PSA_SUCCESS) {
		return PSA_SUCCESS;
	}

	if (sx_pk_curve_opsize(curve) != priv_key_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#if defined(PSA_NEED_CRACEN_ECDH_SECP_R1)
	if (cracen_ecc_curve_is_weierstrass(curve_family)) {
		return cracen_ecdh_wrstr_calc_secret(curve, priv_key, priv_key_size,
								publ_key, publ_key_size, output,
								output_size, output_length);
	}
#endif /* PSA_NEED_CRACEN_ECDH_SECP_R1 */

#if defined(PSA_NEED_CRACEN_ECDH_MONTGOMERY)
	if (curve_family == PSA_ECC_FAMILY_MONTGOMERY) {
		return cracen_ecdh_montgmr_calc_secret(curve, priv_key, priv_key_size,
								publ_key, publ_key_size, output,
								output_size, output_length);
	}
#endif /* PSA_NEED_CRACEN_ECDH_MONTGOMERY */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_output_bytes(cracen_key_derivation_operation_t *operation,
						uint8_t *output, size_t output_length)
{
#if defined(PSA_NEED_CRACEN_HKDF)
	if (PSA_ALG_IS_HKDF(operation->alg)	   ||
	    PSA_ALG_IS_HKDF_EXPAND(operation->alg) ||
	    PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		return cracen_hkdf_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_HKDF */

#if defined(PSA_NEED_CRACEN_PBKDF2_HMAC)
	if (PSA_ALG_IS_PBKDF2(operation->alg)) {
		return cracen_pbkdf2_hmac_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_PBKDF2_HMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		return cracen_sp800_108_ctr_cmac_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
	if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		return cracen_sp800_108_ctr_hmac_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */

#if defined(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
	if (operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
		return cracen_tls12_ecjpake_to_pms_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS */

#if defined(PSA_NEED_CRACEN_TLS12_PRF)
	if (PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		return cracen_tls12_prf_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PRF */

#if defined(PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
	if (PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		return cracen_tls12_psk_to_ms_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
	if (PSA_ALG_IS_SRP_PASSWORD_HASH(operation->alg)) {
		return cracen_srp_password_hash_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_SRP_PASSWORD_HASH */

#if defined(PSA_NEED_CRACEN_WPA3_SAE_H2E)
	if (PSA_ALG_IS_WPA3_SAE_H2E(operation->alg)) {
		return cracen_wpa3_sae_h2e_output_bytes(operation, output, output_length);
	}
#endif /* PSA_NEED_CRACEN_WPA3_SAE_H2E */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_abort(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_SUCCESS;

#if defined(PSA_NEED_CRACEN_WPA3_SAE_H2E)
	if (PSA_ALG_IS_WPA3_SAE_H2E(operation->alg)) {
		status = cracen_wpa3_sae_h2e_abort(operation);
	}
#endif /* PSA_NEED_CRACEN_WPA3_SAE_H2E */

#if defined(PSA_NEED_CRACEN_HKDF)
	if (PSA_ALG_IS_HKDF(operation->alg)) {
		status = cracen_hkdf_abort(operation);
	}
#endif /* PSA_NEED_CRACEN_HKDF */

#if defined(PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
	if (PSA_ALG_IS_SRP_PASSWORD_HASH(operation->alg)) {
		status = cracen_srp_password_hash_abort(operation);
	}
#endif /* PSA_NEED_CRACEN_SRP_PASSWORD_HASH */

	safe_memzero(operation, sizeof(*operation));
	return status;
}
