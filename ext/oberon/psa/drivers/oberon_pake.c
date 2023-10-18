/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "psa/crypto.h"
#include "oberon_pake.h"

psa_status_t oberon_pake_setup(oberon_pake_operation_t *operation,
			       const psa_pake_cipher_suite_t *cipher_suite)
{
	operation->alg = cipher_suite->algorithm;

	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_setup(&operation->ctx.oberon_jpake_ctx, cipher_suite);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_setup(&operation->ctx.oberon_spake2p_ctx, cipher_suite);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_setup(&operation->ctx.oberon_srp_ctx, cipher_suite);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)cipher_suite;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_pake_set_password_key(oberon_pake_operation_t *operation,
					  const psa_key_attributes_t *attributes,
					  const uint8_t *password, size_t password_length)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_set_password_key(&operation->ctx.oberon_jpake_ctx, attributes,
						     password, password_length);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_set_password_key(&operation->ctx.oberon_spake2p_ctx,
						       attributes, password, password_length);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_set_password_key(&operation->ctx.oberon_srp_ctx, attributes,
						   password, password_length);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)attributes;
		(void)password;
		(void)password_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_set_user(oberon_pake_operation_t *operation, const uint8_t *user_id,
				  size_t user_id_len)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_set_user(&operation->ctx.oberon_jpake_ctx, user_id,
					     user_id_len);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_set_user(&operation->ctx.oberon_spake2p_ctx, user_id,
					       user_id_len);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_set_user(&operation->ctx.oberon_srp_ctx, user_id, user_id_len);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)user_id;
		(void)user_id_len;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_set_peer(oberon_pake_operation_t *operation, const uint8_t *peer_id,
				  size_t peer_id_len)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_set_peer(&operation->ctx.oberon_jpake_ctx, peer_id,
					     peer_id_len);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_set_peer(&operation->ctx.oberon_spake2p_ctx, peer_id,
					       peer_id_len);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return PSA_ERROR_NOT_SUPPORTED; // no peer id in SRP
#endif						/* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)peer_id;
		(void)peer_id_len;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_set_role(oberon_pake_operation_t *operation, psa_pake_role_t role)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_set_role(&operation->ctx.oberon_jpake_ctx, role);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_set_role(&operation->ctx.oberon_spake2p_ctx, role);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_set_role(&operation->ctx.oberon_srp_ctx, role);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)role;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_output(oberon_pake_operation_t *operation, psa_pake_step_t step,
				uint8_t *output, size_t output_size, size_t *output_length)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_output(&operation->ctx.oberon_jpake_ctx, step, output,
					   output_size, output_length);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_output(&operation->ctx.oberon_spake2p_ctx, step, output,
					     output_size, output_length);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_output(&operation->ctx.oberon_srp_ctx, step, output, output_size,
					 output_length);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)step;
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_input(oberon_pake_operation_t *operation, psa_pake_step_t step,
			       const uint8_t *input, size_t input_length)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_input(&operation->ctx.oberon_jpake_ctx, step, input,
					  input_length);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_input(&operation->ctx.oberon_spake2p_ctx, step, input,
					    input_length);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_input(&operation->ctx.oberon_srp_ctx, step, input, input_length);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)step;
		(void)input;
		(void)input_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_get_implicit_key(oberon_pake_operation_t *operation, uint8_t *output,
					  size_t output_size, size_t *output_length)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_get_implicit_key(&operation->ctx.oberon_jpake_ctx, output,
						     output_size, output_length);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_get_implicit_key(&operation->ctx.oberon_spake2p_ctx, output,
						       output_size, output_length);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_get_implicit_key(&operation->ctx.oberon_srp_ctx, output,
						   output_size, output_length);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t oberon_pake_abort(oberon_pake_operation_t *operation)
{
	switch (operation->alg) {
#ifdef PSA_NEED_OBERON_JPAKE
	case PSA_ALG_JPAKE:
		return oberon_jpake_abort(&operation->ctx.oberon_jpake_ctx);
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
	case PSA_ALG_SPAKE2P:
		return oberon_spake2p_abort(&operation->ctx.oberon_spake2p_ctx);
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
	case PSA_ALG_SRP_6:
		return oberon_srp_abort(&operation->ctx.oberon_srp_ctx);
#endif /* PSA_NEED_OBERON_SRP_6 */
	default:
		return PSA_ERROR_BAD_STATE;
	}
}
