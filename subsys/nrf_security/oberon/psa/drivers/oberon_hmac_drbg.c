/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_hmac_drbg.h"
#include "psa_crypto_driver_wrappers.h"


/* HMAC-DRBG implementation based on NIST.SP.800-90Ar1 */


#define ENTROPY_FACTOR       4  // fraction of entropy in entropy data
#define RESEED_INTERVAL      10000

#define KEY_LEN              PSA_HASH_LENGTH(OBERON_HMAC_DRBG_HASH_ALG)
#define BLOCK_LEN            PSA_HASH_LENGTH(OBERON_HMAC_DRBG_HASH_ALG)
#define MAX_ENTROPY_BITS     (PSA_BYTES_TO_BITS(BLOCK_LEN) * 3 / 2)
#define MAX_ENTROPY_DATA     ((PSA_BITS_TO_BYTES(MAX_ENTROPY_BITS) * ENTROPY_FACTOR + BLOCK_LEN - 1) / BLOCK_LEN * BLOCK_LEN)
#define MAX_BITS_PER_REQUEST (1 << 19)  // NIST.SP.800-90Ar1:Table 2


// output = HMAC(k, v || tag || data)
static psa_status_t hmac_drbg_hmac(
    oberon_hmac_drbg_context_t *context,
    uint8_t tag, // 0xFF = no tag
    const uint8_t *data, size_t data_length, // may be NULL
    uint8_t output[BLOCK_LEN])
{
    psa_status_t status;
    size_t len;
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(KEY_LEN));
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);

    status = psa_driver_wrapper_mac_sign_setup(
        &context->hmac_op,
        &attr, context->k, BLOCK_LEN,
        PSA_ALG_HMAC(OBERON_HMAC_DRBG_HASH_ALG));
    if (status) goto exit;

    status = psa_driver_wrapper_mac_update(&context->hmac_op, context->v, BLOCK_LEN);
    if (status) goto exit;
       
    if (tag != 0xFF) {
        status = psa_driver_wrapper_mac_update(&context->hmac_op, &tag, 1);
        if (status) goto exit;
    }

    if (data) {
        status = psa_driver_wrapper_mac_update(&context->hmac_op, data, data_length);
        if (status) goto exit;
    }

    status = psa_driver_wrapper_mac_sign_finish(&context->hmac_op, output, BLOCK_LEN, &len);

exit:
    psa_driver_wrapper_mac_abort(&context->hmac_op);
    return status;
}

// update the generator state
static psa_status_t hmac_drbg_update(
    oberon_hmac_drbg_context_t *context,
    const uint8_t *data, size_t data_length) // may be NULL
{
    psa_status_t status;

    status = hmac_drbg_hmac(context, 0x00, data, data_length, context->k);
    if (status) return status;

    status = hmac_drbg_hmac(context, 0xFF, NULL, 0, context->v);
    if (status) return status;

    if (data) {
        status = hmac_drbg_hmac(context, 0x01, data, data_length, context->k);
        if (status) return status;

        status = hmac_drbg_hmac(context, 0xFF, NULL, 0, context->v);
        if (status) return status;
    }

    return PSA_SUCCESS;
}

// update the generator state with new entropy
static psa_status_t hmac_drbg_entropy_update(
    oberon_hmac_drbg_context_t *context,
    size_t entropy_bits)
{
    psa_status_t status;
    uint8_t seed[MAX_ENTROPY_DATA];
    size_t estimate_bits, total_bits, seed_length;

    // get entropy
    total_bits = 0;
    seed_length = 0;
    do {
        status = psa_driver_wrapper_get_entropy(0, &estimate_bits, seed + seed_length, BLOCK_LEN);
        if (status) return status;
        total_bits += estimate_bits;
        seed_length += BLOCK_LEN;
        if (seed_length >= sizeof seed) return PSA_ERROR_INSUFFICIENT_ENTROPY;
    } while (total_bits < entropy_bits);

    // update state
    return hmac_drbg_update(context, seed, seed_length);
}

// instantiate the generator state
psa_status_t oberon_hmac_drbg_init(
    oberon_hmac_drbg_context_t *context)
{
    psa_status_t status;

#ifdef OBERON_USE_MUTEX
    oberon_mutex_init(&context->mutex);
#endif

    // initialize generator state with k = 0 and v = 0
    memset(context->k, 0, sizeof context->k); // 0x00 .. 0x00
    memset(context->v, 1, sizeof context->v); // 0x01 .. 0x01

    // initial seeding
    status = hmac_drbg_entropy_update(context, MAX_ENTROPY_BITS);
    if (status) return status;

    context->reseed_counter = 1;
    return PSA_SUCCESS;
}

// generate random bytes
psa_status_t oberon_hmac_drbg_get_random(
    oberon_hmac_drbg_context_t *context,
    uint8_t *output,
    size_t output_size)
{
    psa_status_t status;

#ifdef OBERON_USE_MUTEX
    if (oberon_mutex_lock(&context->mutex)) {
        return PSA_ERROR_GENERIC_ERROR;
    }
#endif

    if (context->reseed_counter == 0) return PSA_ERROR_BAD_STATE;
    if (output_size > PSA_BITS_TO_BYTES(MAX_BITS_PER_REQUEST)) return PSA_ERROR_INVALID_ARGUMENT;

    // reseed generator if necessary
    if (context->reseed_counter > RESEED_INTERVAL) {
        status = hmac_drbg_entropy_update(context, PSA_BYTES_TO_BITS(KEY_LEN));
        if (status) return status;
        context->reseed_counter = 1;
    }

    // generate output
    for (;;) {
        // generate a MAC block
        status = hmac_drbg_hmac(context, 0xFF, NULL, 0, context->v);
        if (status) return status;
        if (output_size <= BLOCK_LEN) {
            memcpy(output, context->v, output_size);
            break;
        }
        memcpy(output, context->v, BLOCK_LEN);
        output += BLOCK_LEN;
        output_size -= BLOCK_LEN;
    }

    // update generator state for backtracking resistance
    status = hmac_drbg_update(context, NULL, 0);
    if (status) return status;

    context->reseed_counter++;

#ifdef OBERON_USE_MUTEX
    if (oberon_mutex_unlock(&context->mutex)) {
        return PSA_ERROR_GENERIC_ERROR;
    }
#endif

    return PSA_SUCCESS;
}

// cleanup the generator state
psa_status_t oberon_hmac_drbg_free(
    oberon_hmac_drbg_context_t *context)
{
    psa_driver_wrapper_mac_abort(&context->hmac_op);
#ifdef OBERON_USE_MUTEX
    oberon_mutex_free(&context->mutex);
#endif
    memset(context, 0, sizeof *context);
    return PSA_SUCCESS;
}
