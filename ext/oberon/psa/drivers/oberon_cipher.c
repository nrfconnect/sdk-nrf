/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_cipher.h"

#if defined(PSA_NEED_OBERON_CTR_AES) || defined(PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES)
#include "ocrypto_aes_ctr.h"
#endif
#if defined(PSA_NEED_OBERON_CBC_PKCS7_AES) || defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES) ||       \
	defined(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
#include "ocrypto_aes_cbc_pkcs.h"
#endif
#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
#include "ocrypto_chacha20.h"
#endif

#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
static void oberon_aes_ccm_star_set_iv(
    ocrypto_aes_ctr_ctx *ctx,
    const uint8_t iv[13])
{
    uint8_t counter[16];
    counter[0] = 1; // 2 bytes counter
    memcpy(&counter[1], iv, 13);
    counter[14] = 0;
    counter[15] = 1; // initial count
    ocrypto_aes_ctr_init(ctx, NULL, 0, counter);
}
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */

static psa_status_t oberon_cipher_setup(
    oberon_cipher_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg, uint8_t decrypt)
{
#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_chacha20_ctx), "oberon_cipher_operation_t.ctx too small");
    if (alg == PSA_ALG_STREAM_CIPHER && psa_get_key_type(attributes) == PSA_KEY_TYPE_CHACHA20) {
        if (key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_chacha20_init((ocrypto_chacha20_ctx*)operation->ctx, NULL, 0, key, 0);
        operation->decrypt = decrypt;
        operation->alg = alg;
        return PSA_SUCCESS;
    }
#endif /* PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20 */

    if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) return PSA_ERROR_NOT_SUPPORTED;
    if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;

    switch (alg) {
#if defined(PSA_NEED_OBERON_CTR_AES) || defined(PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES)
	_Static_assert(sizeof operation->ctx >= sizeof(ocrypto_aes_ctr_ctx),
		       "oberon_cipher_operation_t.ctx too small");
#ifdef PSA_NEED_OBERON_CTR_AES
    case PSA_ALG_CTR:
#endif /* PSA_NEED_OBERON_CTR_AES */
#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
    case PSA_ALG_CCM_STAR_NO_TAG:
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
	ocrypto_aes_ctr_init((ocrypto_aes_ctr_ctx *)operation->ctx, key, key_length, NULL);
	break;
#endif /* PSA_NEED_OBERON_CTR_AES || PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
	_Static_assert(sizeof operation->ctx >= sizeof(ocrypto_aes_cbc_pkcs_ctx),
		       "oberon_cipher_operation_t.ctx too small");
    case PSA_ALG_CBC_PKCS7:
        ocrypto_aes_cbc_pkcs_init((ocrypto_aes_cbc_pkcs_ctx*)operation->ctx, key, key_length, NULL, decrypt);
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#if defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES) || defined(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
	_Static_assert(sizeof operation->ctx >= sizeof(ocrypto_aes_cbc_pkcs_ctx),
		       "oberon_cipher_operation_t.ctx too small");
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_OBERON_ECB_NO_PADDING_AES
    case PSA_ALG_ECB_NO_PADDING:
#endif /* PSA_NEED_OBERON_ECB_NO_PADDING_AES */
	ocrypto_aes_cbc_pkcs_init((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx, key, key_length, NULL,
				  decrypt * 2);
	break;
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES || PSA_NEED_OBERON_ECB_NO_PADDING_AES */
    default:
        (void)key;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    operation->decrypt = decrypt;
    operation->alg = alg;
    return PSA_SUCCESS;
}

psa_status_t oberon_cipher_encrypt_setup(
    oberon_cipher_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg)
{
    return oberon_cipher_setup(operation, attributes, key, key_length, alg, 0);
}

psa_status_t oberon_cipher_decrypt_setup(
    oberon_cipher_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg)
{
    return oberon_cipher_setup(operation, attributes, key, key_length, alg, 1);
}

psa_status_t oberon_cipher_set_iv(
    oberon_cipher_operation_t *operation,
    const uint8_t *iv, size_t iv_length)
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    case PSA_ALG_STREAM_CIPHER:
        if (iv_length != 12) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_chacha20_init((ocrypto_chacha20_ctx*)operation->ctx, iv, 12, NULL, 0);
        break;
#endif /* PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20 */
#ifdef PSA_NEED_OBERON_CTR_AES
    case PSA_ALG_CTR:
        if (iv_length != 16) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_aes_ctr_init((ocrypto_aes_ctr_ctx*)operation->ctx, NULL, 0, iv);
        break;
#endif /* PSA_NEED_OBERON_CTR_AES */
#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
    case PSA_ALG_CCM_STAR_NO_TAG:
        if (iv_length != 13) return PSA_ERROR_INVALID_ARGUMENT;
        oberon_aes_ccm_star_set_iv((ocrypto_aes_ctr_ctx*)operation->ctx, iv);
        break;
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
#if defined(PSA_NEED_OBERON_CBC_PKCS7_AES) || defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES)
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
    case PSA_ALG_CBC_PKCS7:
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
	if (iv_length != 16) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	ocrypto_aes_cbc_pkcs_init((ocrypto_aes_cbc_pkcs_ctx*)operation->ctx, NULL, 0, iv, 0);
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES || PSA_NEED_OBERON_CBC_NO_PADDING_AES */
    default:
        (void)iv;
        (void)iv_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_cipher_update(
    oberon_cipher_operation_t *operation,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    size_t out_len;
    
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    case PSA_ALG_STREAM_CIPHER:
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_chacha20_update((ocrypto_chacha20_ctx*)operation->ctx, output, input, input_length);
        *output_length = input_length;
        break;
#endif /* PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20 */
#if defined(PSA_NEED_OBERON_CTR_AES) || defined(PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES)
#ifdef PSA_NEED_OBERON_CTR_AES
    case PSA_ALG_CTR:
#endif /* PSA_NEED_OBERON_CTR_AES */
#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
    case PSA_ALG_CCM_STAR_NO_TAG:
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	ocrypto_aes_ctr_update((ocrypto_aes_ctr_ctx*)operation->ctx, output, input, input_length);
        *output_length = input_length;
        break;
#endif /* PSA_NEED_OBERON_CTR_AES || PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
#if defined(PSA_NEED_OBERON_CBC_PKCS7_AES) || defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES) ||       \
	defined(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
    case PSA_ALG_CBC_PKCS7:
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_OBERON_ECB_NO_PADDING_AES
    case PSA_ALG_ECB_NO_PADDING:
#endif /* PSA_NEED_OBERON_ECB_NO_PADDING_AES */
	out_len = ocrypto_aes_cbc_pkcs_output_size((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx,
						   input_length);
	if (output_size < out_len) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_aes_cbc_pkcs_update((ocrypto_aes_cbc_pkcs_ctx*)operation->ctx, output, input, input_length);
        *output_length = out_len;
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES || PSA_NEED_OBERON_CBC_NO_PADDING_AES ||                   \
	  PSA_NEED_OBERON_ECB_NO_PADDING_AES */
    default:
        (void)input;
        (void)input_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)out_len;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_cipher_finish(
    oberon_cipher_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;
    *output_length = 0;

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
    case PSA_ALG_CBC_PKCS7:
        if (output_size < 16) return PSA_ERROR_BUFFER_TOO_SMALL;
        if (operation->decrypt) {
            if (ocrypto_aes_cbc_pkcs_output_size((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx, 1) == 0) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            res = ocrypto_aes_cbc_pkcs_final_dec((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx, output, output_length);
            if (res) return PSA_ERROR_INVALID_PADDING;
        } else {
            ocrypto_aes_cbc_pkcs_final_enc((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx, output);
            *output_length = 16;
        }
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#if defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES) || defined(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_OBERON_ECB_NO_PADDING_AES
    case PSA_ALG_ECB_NO_PADDING:
#endif /* PSA_NEED_OBERON_ECB_NO_PADDING_AES */
	if (ocrypto_aes_cbc_pkcs_output_size((ocrypto_aes_cbc_pkcs_ctx *)operation->ctx, 15) > 0) {
	    return PSA_ERROR_INVALID_ARGUMENT;
	}
	break;
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES || PSA_NEED_OBERON_ECB_NO_PADDING_AES */
    default:
        (void)output;
        (void)output_size;
        (void)res;
        break; // nothing to do
    }

    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}

psa_status_t oberon_cipher_abort(
    oberon_cipher_operation_t *operation)
{
    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}


typedef union {
#if defined(PSA_NEED_OBERON_CTR_AES) || defined(PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES)
    ocrypto_aes_ctr_ctx ctr;
#endif
#if defined(PSA_NEED_OBERON_CBC_PKCS7_AES) || defined(PSA_NEED_OBERON_CBC_NO_PADDING_AES) ||       \
	defined(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
    ocrypto_aes_cbc_pkcs_ctx cbc;
#endif
#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    ocrypto_chacha20_ctx ch;
#endif
    int dummy;
} ocrypto_context;

psa_status_t oberon_cipher_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *iv, size_t iv_length,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    ocrypto_context ctx;
    size_t out_len;

#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    if (alg == PSA_ALG_STREAM_CIPHER && psa_get_key_type(attributes) == PSA_KEY_TYPE_CHACHA20) {
        if (key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        if (iv_length != 12) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_chacha20_init(&ctx.ch, iv, iv_length, key, 0);
        ocrypto_chacha20_update(&ctx.ch, output, input, input_length);
        return PSA_SUCCESS;
    }
#endif /* PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20 */

    if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) return PSA_ERROR_NOT_SUPPORTED;
    if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;

    switch (alg) {
#ifdef PSA_NEED_OBERON_CTR_AES
    case PSA_ALG_CTR:
        if (iv_length != 16) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_aes_ctr_init(&ctx.ctr, key, key_length, iv);
        ocrypto_aes_ctr_update(&ctx.ctr, output, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_CTR_AES */
#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
    case PSA_ALG_CCM_STAR_NO_TAG:
        if (iv_length != 13) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_aes_ctr_init(&ctx.ctr, key, key_length, NULL);
        oberon_aes_ccm_star_set_iv(&ctx.ctr, iv);
        ocrypto_aes_ctr_update(&ctx.ctr, output, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
    case PSA_ALG_CBC_PKCS7:
        if (iv_length != 16) return PSA_ERROR_INVALID_ARGUMENT;
        out_len = (input_length + 16) & ~15; // next multiple of block size
        if (output_size < out_len) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = out_len;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, iv, 0);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input, input_length);
        ocrypto_aes_cbc_pkcs_final_enc(&ctx.cbc, output + (input_length & ~15));
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
        if (iv_length != 16) return PSA_ERROR_INVALID_ARGUMENT;
        if ((input_length & 15) != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, iv, 0);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_OBERON_ECB_NO_PADDING_AES
    case PSA_ALG_ECB_NO_PADDING:
        if (iv_length != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if ((input_length & 15) != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, NULL, 0);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_ECB_NO_PADDING_AES */
    default:
        (void)key;
        (void)iv;
        (void)iv_length;
        (void)input;
        (void)input_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)ctx;
        (void)out_len;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_cipher_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    ocrypto_context ctx;
    int res;

#ifdef PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20
    if (alg == PSA_ALG_STREAM_CIPHER && psa_get_key_type(attributes) == PSA_KEY_TYPE_CHACHA20) {
        if (input_length < 12) return PSA_ERROR_INVALID_ARGUMENT;
        if (key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length - 12) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length - 12;
        ocrypto_chacha20_init(&ctx.ch, input, 12, key, 0);
        ocrypto_chacha20_update(&ctx.ch, output, input + 12, input_length - 12);
        return PSA_SUCCESS;
    }
#endif /* PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20 */

    if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) return PSA_ERROR_NOT_SUPPORTED;
    if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;

    switch (alg) {
#ifdef PSA_NEED_OBERON_CTR_AES
    case PSA_ALG_CTR:
        if (input_length < 16) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length - 16) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length - 16;
        ocrypto_aes_ctr_init(&ctx.ctr, key, key_length, input);
        ocrypto_aes_ctr_update(&ctx.ctr, output, input + 16, input_length - 16);
        break;
#endif /* PSA_NEED_OBERON_CTR_AES */
#ifdef PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES
    case PSA_ALG_CCM_STAR_NO_TAG:
        if (input_length < 13) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length - 13) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length - 13;
        ocrypto_aes_ctr_init(&ctx.ctr, key, key_length, NULL);
        oberon_aes_ccm_star_set_iv(&ctx.ctr, input);
        ocrypto_aes_ctr_update(&ctx.ctr, output, input + 13, input_length - 13);
        break;
#endif /* PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES */
#ifdef PSA_NEED_OBERON_CBC_PKCS7_AES
    case PSA_ALG_CBC_PKCS7:
        if (input_length < 32 || (input_length & 15) != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length - 16) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, input, 1);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input + 16, input_length - 16);
        res = ocrypto_aes_cbc_pkcs_final_dec(&ctx.cbc, output + input_length - 32, output_length);
        if (res) return PSA_ERROR_INVALID_PADDING;
        *output_length += input_length - 32;
        break;
#endif /* PSA_NEED_OBERON_CBC_PKCS7_AES */
#ifdef PSA_NEED_OBERON_CBC_NO_PADDING_AES
    case PSA_ALG_CBC_NO_PADDING:
        if (input_length < 16 || (input_length & 15) != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length - 16) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length - 16;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, input, 2);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input + 16, input_length - 16);
        break;
#endif /* PSA_NEED_OBERON_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_OBERON_ECB_NO_PADDING_AES
    case PSA_ALG_ECB_NO_PADDING:
        if ((input_length & 15) != 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (output_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *output_length = input_length;
        ocrypto_aes_cbc_pkcs_init(&ctx.cbc, key, key_length, NULL, 2);
        ocrypto_aes_cbc_pkcs_update(&ctx.cbc, output, input, input_length);
        break;
#endif /* PSA_NEED_OBERON_ECB_NO_PADDING_AES */
    default:
        (void)key;
        (void)input;
        (void)input_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)ctx;
        (void)res;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}
