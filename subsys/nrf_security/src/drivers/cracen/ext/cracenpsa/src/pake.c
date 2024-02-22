/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include "common.h"
#include "cracen_psa_primitives.h"
#include "psa/crypto_sizes.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"
#include <psa_crypto_driver_wrappers.h>

psa_status_t cracen_pake_setup(cracen_pake_operation_t *operation,
			       const psa_pake_cipher_suite_t *cipher_suite)
{
	operation->alg = cipher_suite->algorithm;
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_setup(&operation->cracen_jpake_ctx, cipher_suite);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_setup(&operation->cracen_srp_ctx, cipher_suite);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_setup(&operation->cracen_spake2p_ctx, cipher_suite);
		}
		break;
	default:
		(void)cipher_suite;
	}

	return status;
}

psa_status_t cracen_pake_set_password_key(cracen_pake_operation_t *operation,
					  const psa_key_attributes_t *attributes,
					  const uint8_t *password, size_t password_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_set_password_key(&operation->cracen_jpake_ctx,
							       attributes, password,
							       password_length);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_set_password_key(&operation->cracen_srp_ctx, attributes,
							     password, password_length);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_set_password_key(&operation->cracen_spake2p_ctx,
								 attributes, password,
								 password_length);
		}
		break;
	default:
		(void)attributes;
		(void)password;
		(void)password_length;
	}

	return status;
}

psa_status_t cracen_pake_set_user(cracen_pake_operation_t *operation, const uint8_t *user_id,
				  size_t user_id_len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_set_user(&operation->cracen_jpake_ctx, user_id,
						       user_id_len);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_set_user(&operation->cracen_srp_ctx, user_id,
						     user_id_len);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_set_user(&operation->cracen_spake2p_ctx, user_id,
							 user_id_len);
		}
		break;
	default:
		(void)user_id;
		(void)user_id_len;
	}

	return status;
}

psa_status_t cracen_pake_set_peer(cracen_pake_operation_t *operation, const uint8_t *peer_id,
				  size_t peer_id_len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_set_peer(&operation->cracen_jpake_ctx, peer_id,
						       peer_id_len);
		}
		break;
	case PSA_ALG_SRP_6:
		return PSA_ERROR_NOT_SUPPORTED;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			return cracen_spake2p_set_peer(&operation->cracen_spake2p_ctx, peer_id,
						       peer_id_len);
		}
		break;
	default:
		(void)peer_id;
		(void)peer_id_len;
	}

	return status;
}

psa_status_t cracen_pake_set_role(cracen_pake_operation_t *operation, psa_pake_role_t role)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_set_role(&operation->cracen_jpake_ctx, role);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_set_role(&operation->cracen_srp_ctx, role);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_set_role(&operation->cracen_spake2p_ctx, role);
		}
		break;
	default:
		(void)role;
	}

	return status;
}

psa_status_t cracen_pake_output(cracen_pake_operation_t *operation, psa_pake_step_t step,
				uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_output(&operation->cracen_jpake_ctx, step, output,
						     output_size, output_length);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_output(&operation->cracen_srp_ctx, step, output,
						   output_size, output_length);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_output(&operation->cracen_spake2p_ctx, step, output,
						       output_size, output_length);
		}
		break;
	default:
		(void)step;
		(void)output;
		(void)output_size;
		(void)output_length;
	}

	return status;
}

psa_status_t cracen_pake_input(cracen_pake_operation_t *operation, psa_pake_step_t step,
			       const uint8_t *input, size_t input_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_input(&operation->cracen_jpake_ctx, step, input,
						    input_length);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_input(&operation->cracen_srp_ctx, step, input,
						  input_length);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_input(&operation->cracen_spake2p_ctx, step, input,
						      input_length);
		}
		break;
	default:
		(void)step;
		(void)input;
		(void)input_length;
	}

	return status;
}

psa_status_t cracen_pake_get_implicit_key(cracen_pake_operation_t *operation, uint8_t *output,
					  size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_get_implicit_key(&operation->cracen_jpake_ctx, output,
							       output_size, output_length);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_get_implicit_key(&operation->cracen_srp_ctx, output,
							     output_size, output_length);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_get_implicit_key(
				&operation->cracen_spake2p_ctx, output, output_size, output_length);
		}
		break;
	default:
		(void)output;
		(void)output_size;
		(void)output_length;
	}

	return status;
}

psa_status_t cracen_pake_abort(cracen_pake_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	switch (operation->alg) {
	case PSA_ALG_JPAKE:
		if (IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
			status = cracen_jpake_abort(&operation->cracen_jpake_ctx);
		}
		break;
	case PSA_ALG_SRP_6:
		if (IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
			status = cracen_srp_abort(&operation->cracen_srp_ctx);
		}
		break;
	case PSA_ALG_SPAKE2P:
		if (IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
			status = cracen_spake2p_abort(&operation->cracen_spake2p_ctx);
		}
		break;
	default:
	}

	return status;
}
