/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the PSA Crypto Driver API.

#include <string.h>

#include "psa/crypto.h"
#include "oberon_aead.h"

#ifdef PSA_NEED_OBERON_CCM_AES
#include "ocrypto_aes_ccm.h"
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
#include "ocrypto_aes_gcm.h"
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
#include "ocrypto_chacha20_poly1305.h"
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */


static psa_status_t oberon_aead_setup(
    oberon_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg, uint8_t decrypt)
{
    size_t tag_length;
    psa_algorithm_t short_alg;
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    ocrypto_chacha20_poly1305_ctx *cp_ctx;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */

    short_alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg, 0);
    tag_length = PSA_ALG_AEAD_GET_TAG_LENGTH(alg);

    switch (psa_get_key_type(attributes)) {
    case PSA_KEY_TYPE_AES:
        if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        switch (short_alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
        _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_aes_ccm_ctx), "oberon_aead_operation_t.ctx too small");
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
            ocrypto_aes_ccm_init((ocrypto_aes_ccm_ctx*)&operation->ctx, key, key_length, NULL, 0, 0, 0, 0);
            break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
        _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_aes_gcm_ctx), "oberon_aead_operation_t.ctx too small");
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
            ocrypto_aes_gcm_init((ocrypto_aes_gcm_ctx*)&operation->ctx, key, key_length, NULL);
            break;
#endif /* PSA_NEED_OBERON_GCM_AES */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_chacha20_poly1305_ctx), "oberon_aead_operation_t.ctx too small");
    case PSA_KEY_TYPE_CHACHA20:
        if (alg == PSA_ALG_CHACHA20_POLY1305) {
            if (key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
            cp_ctx = (ocrypto_chacha20_poly1305_ctx *)&operation->ctx;
            memcpy(cp_ctx->enc_ctx.cipher, key, key_length);
        } else {
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)key;
        (void)key_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    operation->decrypt = decrypt;
    operation->alg = short_alg;
    operation->tag_length = (uint8_t)tag_length;
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_encrypt_setup(
    oberon_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg)
{
    return oberon_aead_setup(operation, attributes, key, key_length, alg, 0);
}

psa_status_t oberon_aead_decrypt_setup(
    oberon_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg)
{
    return oberon_aead_setup(operation, attributes, key, key_length, alg, 1);
}

psa_status_t oberon_aead_set_lengths(
    oberon_aead_operation_t *operation,
    size_t ad_length,
    size_t plaintext_length)
{
    operation->ad_length = ad_length;
    operation->pt_length = plaintext_length;
    operation->length_set = 1;
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_set_nonce(
    oberon_aead_operation_t *operation,
    const uint8_t *nonce, size_t nonce_length)
{
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    ocrypto_chacha20_poly1305_ctx *cp_ctx;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
        if (nonce_length < 7 || nonce_length > 13) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_aes_ccm_init((ocrypto_aes_ccm_ctx*)&operation->ctx,
                             NULL, 0, nonce, nonce_length,
                             operation->tag_length, operation->pt_length, operation->ad_length);
        break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
        if (nonce_length == 0) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_aes_gcm_init_iv((ocrypto_aes_gcm_ctx*)&operation->ctx, nonce, nonce_length);
        break;
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
        if (nonce_length != 8 && nonce_length != 12) return PSA_ERROR_INVALID_ARGUMENT;
        // key in ctx->enc_ctx.cipher
        cp_ctx = (ocrypto_chacha20_poly1305_ctx *)&operation->ctx;
        ocrypto_chacha20_poly1305_init(cp_ctx, nonce, nonce_length, cp_ctx->enc_ctx.cipher);
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)nonce;
        (void)nonce_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_aead_update_ad(
    oberon_aead_operation_t *operation,
    const uint8_t *input, size_t input_length)
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
        ocrypto_aes_ccm_update_aad((ocrypto_aes_ccm_ctx*)&operation->ctx, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
        ocrypto_aes_gcm_update_aad((ocrypto_aes_gcm_ctx*)&operation->ctx, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
        ocrypto_chacha20_poly1305_update_aad((ocrypto_chacha20_poly1305_ctx*)&operation->ctx, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)input;
        (void)input_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (operation->length_set) {
        operation->ad_length -= input_length;
    }
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_update(
    oberon_aead_operation_t *operation,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    *output_length = input_length;

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
        if (operation->decrypt) {
            ocrypto_aes_ccm_update_dec((ocrypto_aes_ccm_ctx*)&operation->ctx, output, input, input_length);
        } else {
            ocrypto_aes_ccm_update_enc((ocrypto_aes_ccm_ctx*)&operation->ctx, output, input, input_length);
        }
        break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
        if (operation->decrypt) {
            ocrypto_aes_gcm_update_dec((ocrypto_aes_gcm_ctx*)&operation->ctx, output, input, input_length);
        } else {
            ocrypto_aes_gcm_update_enc((ocrypto_aes_gcm_ctx*)&operation->ctx, output, input, input_length);
        }
        break;
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
        if (operation->decrypt) {
            ocrypto_chacha20_poly1305_update_dec((ocrypto_chacha20_poly1305_ctx*)&operation->ctx, output, input, input_length);
        } else {
            ocrypto_chacha20_poly1305_update_enc((ocrypto_chacha20_poly1305_ctx*)&operation->ctx, output, input, input_length);
        }
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)input;
        (void)output;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (operation->length_set) {
        operation->pt_length -= input_length;
    }
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_finish(
    oberon_aead_operation_t *operation,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length,
    uint8_t *tag, size_t tag_size, size_t *tag_length)
{
    if (tag_size < operation->tag_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    if (operation->length_set &&
        (operation->pt_length != 0 || operation->ad_length != 0)) return PSA_ERROR_INVALID_ARGUMENT;
    (void)ciphertext;
    (void)ciphertext_size;
    *ciphertext_length = 0;
    *tag_length = operation->tag_length;

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
        ocrypto_aes_ccm_final_enc((ocrypto_aes_ccm_ctx*)&operation->ctx, tag, operation->tag_length);
        break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
        ocrypto_aes_gcm_final_enc((ocrypto_aes_gcm_ctx*)&operation->ctx, tag, operation->tag_length);
        break;
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
        ocrypto_chacha20_poly1305_final_enc((ocrypto_chacha20_poly1305_ctx*)&operation->ctx, tag);
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)tag;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_verify(
    oberon_aead_operation_t *operation,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length,
    const uint8_t *tag, size_t tag_length)
{
    int res;
    if (tag_length != operation->tag_length) return PSA_ERROR_INVALID_SIGNATURE;
    if (operation->length_set &&
        (operation->pt_length != 0 || operation->ad_length != 0)) return PSA_ERROR_INVALID_ARGUMENT;
    (void)plaintext;
    (void)plaintext_size;
    *plaintext_length = 0;

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
        res = ocrypto_aes_ccm_final_dec((ocrypto_aes_ccm_ctx*)&operation->ctx, tag, operation->tag_length);
        break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
        res = ocrypto_aes_gcm_final_dec((ocrypto_aes_gcm_ctx*)&operation->ctx, tag, operation->tag_length);
        break;
#endif /* PSA_NEED_OBERON_GCM_AES */
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
        res = ocrypto_chacha20_poly1305_final_dec((ocrypto_chacha20_poly1305_ctx*)&operation->ctx, tag);
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)tag;
        return PSA_ERROR_NOT_SUPPORTED;
    }
    if (res) return PSA_ERROR_INVALID_SIGNATURE;

    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}

psa_status_t oberon_aead_abort(
    oberon_aead_operation_t *operation)
{
    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}


typedef union {
#ifdef PSA_NEED_OBERON_CCM_AES
    ocrypto_aes_ccm_ctx ccm;
#endif
#ifdef PSA_NEED_OBERON_GCM_AES
    ocrypto_aes_gcm_ctx gcm;
#endif
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    ocrypto_chacha20_poly1305_ctx cp;
#endif
    int dummy;
} ocrypto_context;

psa_status_t oberon_aead_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *plaintext, size_t plaintext_length,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length)
{
    ocrypto_context ctx;
    size_t tag_length = PSA_ALG_AEAD_GET_TAG_LENGTH(alg);
    size_t length = plaintext_length + tag_length;
    if (length < tag_length || ciphertext_size < length) return PSA_ERROR_BUFFER_TOO_SMALL;
    *ciphertext_length = length;

    switch (psa_get_key_type(attributes)) {
    case PSA_KEY_TYPE_AES:
        if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        switch (PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg, 0)) {
#ifdef PSA_NEED_OBERON_CCM_AES
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
            if (tag_length < 4 || tag_length > 16 || (tag_length & 1)) return PSA_ERROR_INVALID_ARGUMENT;
            if (nonce_length < 7 || nonce_length > 13) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_aes_ccm_init(&ctx.ccm,
                key, key_length, nonce, nonce_length,
                tag_length, plaintext_length, additional_data_length);
            if (additional_data_length) {
                ocrypto_aes_ccm_update_aad(&ctx.ccm, additional_data, additional_data_length);
            }
            ocrypto_aes_ccm_update_enc(&ctx.ccm, ciphertext, plaintext, plaintext_length);
            ocrypto_aes_ccm_final_enc(&ctx.ccm, ciphertext + plaintext_length, tag_length);
            break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
            if (tag_length < 4 || tag_length > 16 || nonce_length == 0) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_aes_gcm_init(&ctx.gcm, key, key_length, NULL);
            ocrypto_aes_gcm_init_iv(&ctx.gcm, nonce, nonce_length);
            if (additional_data_length) {
                ocrypto_aes_gcm_update_aad(&ctx.gcm, additional_data, additional_data_length);
            }
            ocrypto_aes_gcm_update_enc(&ctx.gcm, ciphertext, plaintext, plaintext_length);
            ocrypto_aes_gcm_final_enc(&ctx.gcm, ciphertext + plaintext_length, tag_length);
            break;
#endif /* PSA_NEED_OBERON_GCM_AES */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_KEY_TYPE_CHACHA20:
        if (alg == PSA_ALG_CHACHA20_POLY1305) {
            if (key_length != 32 || (nonce_length != 8 && nonce_length != 12)) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_chacha20_poly1305_init(&ctx.cp, nonce, nonce_length, key);
            if (additional_data_length) {
                ocrypto_chacha20_poly1305_update_aad(&ctx.cp, additional_data, additional_data_length);
            }
            ocrypto_chacha20_poly1305_update_enc(&ctx.cp, ciphertext, plaintext, plaintext_length);
            ocrypto_chacha20_poly1305_final_enc(&ctx.cp, ciphertext + plaintext_length);
        } else {
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)key;
        (void)key_length;
        (void)nonce;
        (void)nonce_length;
        (void)additional_data;
        (void)additional_data_length;
        (void)plaintext;
        (void)ciphertext;
        (void)ctx;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_aead_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *ciphertext, size_t ciphertext_length,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length)
{
    ocrypto_context ctx;
    int res;
    size_t tag_length = PSA_ALG_AEAD_GET_TAG_LENGTH(alg);
    size_t pt_length = ciphertext_length - tag_length;
    if (ciphertext_length < tag_length) return PSA_ERROR_INVALID_ARGUMENT;
    if (plaintext_size < pt_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    *plaintext_length = pt_length;

    switch (psa_get_key_type(attributes)) {
    case PSA_KEY_TYPE_AES:
        if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        switch (PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg, 0)) {
#ifdef PSA_NEED_OBERON_CCM_AES
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
            if (tag_length < 4 || tag_length > 16 || (tag_length & 1)) return PSA_ERROR_INVALID_ARGUMENT;
            if (nonce_length < 7 || nonce_length > 13) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_aes_ccm_init(&ctx.ccm,
                key, key_length, nonce, nonce_length,
                tag_length, pt_length, additional_data_length);
            if (additional_data_length) {
                ocrypto_aes_ccm_update_aad(&ctx.ccm, additional_data, additional_data_length);
            }
            ocrypto_aes_ccm_update_dec(&ctx.ccm, plaintext, ciphertext, pt_length);
            res = ocrypto_aes_ccm_final_dec(&ctx.ccm, ciphertext + pt_length, tag_length);
            break;
#endif /* PSA_NEED_OBERON_CCM_AES */
#ifdef PSA_NEED_OBERON_GCM_AES
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
            if (tag_length < 4 || tag_length > 16 || nonce_length == 0) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_aes_gcm_init(&ctx.gcm, key, key_length, NULL);
            ocrypto_aes_gcm_init_iv(&ctx.gcm, nonce, nonce_length);
            if (additional_data_length) {
                ocrypto_aes_gcm_update_aad(&ctx.gcm, additional_data, additional_data_length);
            }
            ocrypto_aes_gcm_update_dec(&ctx.gcm, plaintext, ciphertext, pt_length);
            res = ocrypto_aes_gcm_final_dec(&ctx.gcm, ciphertext + pt_length, tag_length);
            break;
#endif /* PSA_NEED_OBERON_GCM_AES */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#ifdef PSA_NEED_OBERON_CHACHA20_POLY1305
    case PSA_KEY_TYPE_CHACHA20:
        if (alg == PSA_ALG_CHACHA20_POLY1305) {
            if (key_length != 32 || (nonce_length != 8 && nonce_length != 12)) return PSA_ERROR_INVALID_ARGUMENT;
            ocrypto_chacha20_poly1305_init(&ctx.cp, nonce, nonce_length, key);
            if (additional_data_length) {
                ocrypto_chacha20_poly1305_update_aad(&ctx.cp, additional_data, additional_data_length);
            }
            ocrypto_chacha20_poly1305_update_dec(&ctx.cp, plaintext, ciphertext, pt_length);
            res = ocrypto_chacha20_poly1305_final_dec(&ctx.cp, ciphertext + pt_length);
        } else {
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
    default:
        (void)key;
        (void)key_length;
        (void)nonce;
        (void)nonce_length;
        (void)additional_data;
        (void)additional_data_length;
        (void)ciphertext;
        (void)plaintext;
        (void)ctx;
        return PSA_ERROR_NOT_SUPPORTED;
    }
    if (res) return PSA_ERROR_INVALID_SIGNATURE;

    return PSA_SUCCESS;
}
