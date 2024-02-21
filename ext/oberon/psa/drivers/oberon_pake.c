/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the Arm PSA Crypto Driver API.

#include <string.h>

#include "psa/crypto.h"
#include "oberon_pake.h"


psa_status_t oberon_pake_setup(
    oberon_pake_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    operation->alg = psa_pake_cs_get_algorithm(cipher_suite);

#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_setup(
            &operation->ctx.oberon_jpake_ctx,
            attributes, password, password_length,
            cipher_suite);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_setup(
            &operation->ctx.oberon_spake2p_ctx,
            attributes, password, password_length,
            cipher_suite);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_setup(
            &operation->ctx.oberon_srp_ctx,
            attributes, password, password_length,
            cipher_suite);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)attributes;
        (void)password;
        (void)password_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_pake_set_role(
    oberon_pake_operation_t *operation,
    psa_pake_role_t role)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return PSA_SUCCESS;
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_set_role(
            &operation->ctx.oberon_spake2p_ctx, role);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_set_role(
            &operation->ctx.oberon_srp_ctx, role);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        (void)role;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_set_user(
    oberon_pake_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_set_user(
            &operation->ctx.oberon_jpake_ctx, user_id, user_id_len);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_set_user(
            &operation->ctx.oberon_spake2p_ctx, user_id, user_id_len);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_set_user(
            &operation->ctx.oberon_srp_ctx, user_id, user_id_len);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        (void)user_id;
        (void)user_id_len;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_set_peer(
    oberon_pake_operation_t *operation,
    const uint8_t *peer_id, size_t peer_id_len)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_set_peer(
            &operation->ctx.oberon_jpake_ctx, peer_id, peer_id_len);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_set_peer(
            &operation->ctx.oberon_spake2p_ctx, peer_id, peer_id_len);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
    {
        (void)operation;
        (void)peer_id;
        (void)peer_id_len;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_set_context(
    oberon_pake_operation_t *operation,
    const uint8_t *context, size_t context_len)
{
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_set_context(
            &operation->ctx.oberon_spake2p_ctx, context, context_len);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
    {
        (void)operation;
        (void)context;
        (void)context_len;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_output(
    oberon_pake_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_output(
            &operation->ctx.oberon_jpake_ctx, step, output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_output(
            &operation->ctx.oberon_spake2p_ctx, step, output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_output(
            &operation->ctx.oberon_srp_ctx, step, output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        (void)step;
        (void)output;
        (void)output_size;
        (void)output_length;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_input(
    oberon_pake_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_input(
            &operation->ctx.oberon_jpake_ctx, step, input, input_length);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_input(
            &operation->ctx.oberon_spake2p_ctx, step, input, input_length);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_input(
            &operation->ctx.oberon_srp_ctx, step, input, input_length);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        (void)step;
        (void)input;
        (void)input_length;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_get_shared_key(
    oberon_pake_operation_t *operation,
    const psa_key_attributes_t *attributes,
    uint8_t *key, size_t key_size, size_t *key_length)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_get_shared_key(
            &operation->ctx.oberon_jpake_ctx, key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_get_shared_key(
            &operation->ctx.oberon_spake2p_ctx, key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_get_shared_key(
            &operation->ctx.oberon_srp_ctx, key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        (void)attributes;
        (void)key;
        (void)key_size;
        (void)key_length;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_pake_abort(
    oberon_pake_operation_t *operation)
{
#ifdef PSA_NEED_OBERON_JPAKE
    if (PSA_ALG_IS_JPAKE(operation->alg)) {
        return oberon_jpake_abort(
            &operation->ctx.oberon_jpake_ctx);
    } else
#endif /* PSA_NEED_OBERON_JPAKE */
#ifdef PSA_NEED_OBERON_SPAKE2P
    if (PSA_ALG_IS_SPAKE2P(operation->alg)) {
        return oberon_spake2p_abort(
            &operation->ctx.oberon_spake2p_ctx);
    } else
#endif /* PSA_NEED_OBERON_SPAKE2P */
#ifdef PSA_NEED_OBERON_SRP_6
    if (PSA_ALG_IS_SRP_6(operation->alg)) {
        return oberon_srp_abort(
            &operation->ctx.oberon_srp_ctx);
    } else
#endif /* PSA_NEED_OBERON_SRP_6 */
    {
        (void)operation;
        return PSA_ERROR_BAD_STATE;
    }
}
