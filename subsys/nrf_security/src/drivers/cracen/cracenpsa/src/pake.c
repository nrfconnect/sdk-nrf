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
			       const psa_key_attributes_t *attributes, const uint8_t *password,
			       size_t password_length, const psa_pake_cipher_suite_t *cipher_suite)
{
	operation->alg = psa_pake_cs_get_algorithm(cipher_suite);

	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status = cracen_jpake_setup(&operation->cracen_jpake_ctx, attributes, password,
					    password_length, cipher_suite);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_setup(&operation->cracen_srp_ctx, attributes, password,
					  password_length, cipher_suite);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_setup(&operation->cracen_spake2p_ctx, attributes, password,
					      password_length, cipher_suite);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_set_user(cracen_pake_operation_t *operation, const uint8_t *user_id,
				  size_t user_id_len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status = cracen_jpake_set_user(&operation->cracen_jpake_ctx, user_id, user_id_len);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_set_user(&operation->cracen_srp_ctx, user_id, user_id_len);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_set_user(&operation->cracen_spake2p_ctx, user_id,
						 user_id_len);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_set_peer(cracen_pake_operation_t *operation, const uint8_t *peer_id,
				  size_t peer_id_len)
{
	if (PSA_ALG_IS_JPAKE(operation->alg) && IS_ENABLED(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		return cracen_jpake_set_peer(&operation->cracen_jpake_ctx, peer_id, peer_id_len);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg) && IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		return cracen_spake2p_set_peer(&operation->cracen_spake2p_ctx, peer_id,
					       peer_id_len);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t cracen_pake_set_context(cracen_pake_operation_t *operation, const uint8_t *context,
				     size_t context_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_set_context(&operation->cracen_spake2p_ctx, context,
						    context_length);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_set_role(cracen_pake_operation_t *operation, psa_pake_role_t role)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status = cracen_jpake_set_role(&operation->cracen_jpake_ctx, role);
#else
		status = PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_set_role(&operation->cracen_srp_ctx, role);
#else
		status = PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_set_role(&operation->cracen_spake2p_ctx, role);
#else
		status = PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_output(cracen_pake_operation_t *operation, psa_pake_step_t step,
				uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status = cracen_jpake_output(&operation->cracen_jpake_ctx, step, output,
					     output_size, output_length);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_output(&operation->cracen_srp_ctx, step, output, output_size,
					   output_length);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_output(&operation->cracen_spake2p_ctx, step, output,
					       output_size, output_length);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_input(cracen_pake_operation_t *operation, psa_pake_step_t step,
			       const uint8_t *input, size_t input_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status =
			cracen_jpake_input(&operation->cracen_jpake_ctx, step, input, input_length);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_input(&operation->cracen_srp_ctx, step, input, input_length);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_input(&operation->cracen_spake2p_ctx, step, input,
					      input_length);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_get_shared_key(cracen_pake_operation_t *operation,
					const psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status =
			cracen_jpake_get_shared_key(&operation->cracen_jpake_ctx, attributes,
						    key_buffer, key_buffer_size, key_buffer_length);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_get_shared_key(&operation->cracen_srp_ctx, attributes,
						   key_buffer, key_buffer_size, key_buffer_length);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_get_shared_key(&operation->cracen_spake2p_ctx, attributes,
						       key_buffer, key_buffer_size,
						       key_buffer_length);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}

psa_status_t cracen_pake_abort(cracen_pake_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (PSA_ALG_IS_JPAKE(operation->alg)) {
#ifdef PSA_NEED_CRACEN_ECJPAKE
		status = cracen_jpake_abort(&operation->cracen_jpake_ctx);
#endif /* PSA_NEED_CRACEN_ECJPAKE */
	} else if (PSA_ALG_IS_SRP_6(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SRP_6
		status = cracen_srp_abort(&operation->cracen_srp_ctx);
#endif /* PSA_NEED_CRACEN_SRP_6 */
	} else if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
#ifdef PSA_NEED_CRACEN_SPAKE2P
		status = cracen_spake2p_abort(&operation->cracen_spake2p_ctx);
#endif /* PSA_NEED_CRACEN_SPAKE2P */
	}

	return status;
}
