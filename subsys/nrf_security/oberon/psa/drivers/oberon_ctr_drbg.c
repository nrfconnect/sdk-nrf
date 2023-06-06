/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_ctr_drbg.h"
#include "psa_crypto_driver_wrappers.h"


/* CTR-DRBG implementation based on NIST.SP.800-90Ar1 */


#define ENTROPY_FACTOR       1  // fraction of entropy in entropy data
#define RESEED_INTERVAL      10000

#define KEY_LEN              PSA_BITS_TO_BYTES(256)
#define BLOCK_LEN            PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)
#define SEED_LEN             (KEY_LEN + BLOCK_LEN)  // NIST.SP.800-90Ar1:Table 3
#define MAX_BITS_PER_REQUEST (1 << 19)              // NIST.SP.800-90Ar1:Table 3
#define MAX_ENTROPY_BLOCKS   ((SEED_LEN * ENTROPY_FACTOR + BLOCK_LEN - 1) / BLOCK_LEN)


// get entropy from entropy driver
static psa_status_t get_entropy(
    uint8_t data[SEED_LEN],
    size_t entropy_bits)
{
#if ENTROPY_FACTOR == 1
    // Use entropy input without derivation function.
    psa_status_t status;
    size_t estimate_bits;

    // get SEED_LEN full entropy bits
    status = psa_driver_wrapper_get_entropy(0, &estimate_bits, data, SEED_LEN);
    if (status) return status;
    if (estimate_bits < entropy_bits) return PSA_ERROR_INSUFFICIENT_ENTROPY;

    return PSA_SUCCESS;
#else /* ENTROPY_FACTOR > 1 */
    // Use entropy input with block cipher derivation function.
    // We use a CMAC instead of the CBC-MAC with explicit length input specified in NIST.SP.800-90Ar1.
    union {
        struct {
            psa_mac_operation_t mac_op;
            uint8_t input[MAX_ENTROPY_BLOCKS * BLOCK_LEN]; // we have to store the whole entropy input here
        };
        psa_cipher_operation_t cipher_op;
    } u;
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    uint8_t key[KEY_LEN];
    uint8_t temp[(SEED_LEN + BLOCK_LEN - 1) / BLOCK_LEN * BLOCK_LEN];
    uint8_t iv[BLOCK_LEN];
    psa_status_t status;
    size_t in_length, ret_length, temp_length, data_length, len;
    size_t estimate_bits, total_bits;
    unsigned i;

    // get entropy
    total_bits = 0;
    in_length = 0;
    do {
        status = psa_driver_wrapper_get_entropy(0, &estimate_bits, u.input + in_length, BLOCK_LEN);
        if (status) return status;
        total_bits += estimate_bits;
        in_length += BLOCK_LEN;
        if (in_length >= MAX_ENTROPY_BLOCKS * BLOCK_LEN) return PSA_ERROR_INSUFFICIENT_ENTROPY;
    } while (total_bits < entropy_bits);

    // get key
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(KEY_LEN));
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
    for (i = 0; i < KEY_LEN; i++) key[i] = (uint8_t)i;
    ret_length = SEED_LEN;
    memset(iv, 0, sizeof iv);

    // get temp
    i = 0;
    for (temp_length = 0; temp_length < SEED_LEN; temp_length += BLOCK_LEN) {
        memset(&u.mac_op, 0, sizeof u.mac_op);
        status = psa_driver_wrapper_mac_sign_setup(&u.mac_op, &attr, key, sizeof key, PSA_ALG_CMAC);
        if (status) return status;
        iv[0] = i;
        status = psa_driver_wrapper_mac_update(&u.mac_op, iv, BLOCK_LEN); // IV
        if (status) return status;
        status = psa_driver_wrapper_mac_update(&u.mac_op, u.input, in_length); // input
        if (status) return status;
        status = psa_driver_wrapper_mac_sign_finish(&u.mac_op, temp + temp_length, BLOCK_LEN, &len);
        if (status) return status;
        i++;
    }

    // set key
    memset(&u.cipher_op, 0, sizeof u.cipher_op);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
    status = psa_driver_wrapper_cipher_encrypt_setup(
        &u.cipher_op,
        &attr, temp, KEY_LEN,
        PSA_ALG_ECB_NO_PADDING);
    if (status) return status;

    // set X
    memcpy(iv, temp + KEY_LEN, BLOCK_LEN);

    // get output
    data_length = SEED_LEN;
    for (;;) {
        status = psa_driver_wrapper_cipher_update(
            &u.cipher_op,
            iv, BLOCK_LEN,
            iv, BLOCK_LEN, &len);
        if (status) return status;

        if (data_length <= BLOCK_LEN) {
            memcpy(data, iv, data_length);
            return PSA_SUCCESS;
        }

        memcpy(data, iv, BLOCK_LEN);
        data += BLOCK_LEN;
        data_length -= BLOCK_LEN;
    }
#endif /* ENTROPY_FACTOR */
}

// AES-CTR based encrypt function
static psa_status_t ctr_drbg_cipher(
    oberon_ctr_drbg_context_t *context,
    const uint8_t *input, // may be NULL
    uint8_t *cipher, size_t cipher_length)
{
    uint8_t block[BLOCK_LEN];
    size_t length;
    psa_status_t status;

    while (cipher_length >= BLOCK_LEN) {
        // increment counter part of v
        context->counter_part++;

        // generate a cipher block
        status = psa_driver_wrapper_cipher_update(
            &context->aes_op,
            context->v, BLOCK_LEN,
            input ? block : cipher, BLOCK_LEN, &length);
        if (status) return status;

        // combine cipher with input
        if (input) {
            oberon_xor(cipher, block, input, BLOCK_LEN);
        }

        cipher += BLOCK_LEN;
        cipher_length -= BLOCK_LEN;
    }

    if (cipher_length) {
        // increment counter part of v
        context->counter_part++;

        // generate a cipher block
        status = psa_driver_wrapper_cipher_update(
            &context->aes_op,
            context->v, BLOCK_LEN,
            block, BLOCK_LEN, &length);
        if (status) return status;

        if (input) {
            oberon_xor(cipher, block, input, cipher_length);
        } else {
            memcpy(cipher, block, cipher_length);
        }
    }

    return PSA_SUCCESS;
}

// reset the generator state from seed
static psa_status_t ctr_drbg_reset(
    oberon_ctr_drbg_context_t *context,
    const uint8_t seed[SEED_LEN])
{
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(KEY_LEN));
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);

    // reset cipher in case it was not properly cleaned up before
    psa_driver_wrapper_cipher_abort(&context->aes_op);

    // restart generator with new key
    status = psa_driver_wrapper_cipher_encrypt_setup(
        &context->aes_op,
        &attributes, seed, KEY_LEN,
        PSA_ALG_ECB_NO_PADDING);
    if (status) return status;

    // set new v
    memcpy(context->v, seed + KEY_LEN, BLOCK_LEN);

    return PSA_SUCCESS;
}

// update the generator state
static psa_status_t ctr_drbg_update(
    oberon_ctr_drbg_context_t *context,
    const uint8_t input[SEED_LEN]) // may be NULL
{
    uint8_t temp[SEED_LEN];
    psa_status_t status;

    // get random data from state and combine with input
    status = ctr_drbg_cipher(context, input, temp, SEED_LEN);
    if (status) return status;

    // update key and counter
    return ctr_drbg_reset(context, temp);
}

// instantiate the generator state
psa_status_t oberon_ctr_drbg_init(
    oberon_ctr_drbg_context_t *context)
{
    uint8_t data[SEED_LEN];
    psa_status_t status;

#ifdef OBERON_USE_MUTEX
    oberon_mutex_init(&context->mutex);
#endif

    // initialize generator state with key = 0 and v = 0
    memset(data, 0, sizeof data);
    status = ctr_drbg_reset(context, data);
    if (status) return status;

    // initial seeding
    status = get_entropy(data, PSA_BYTES_TO_BITS(SEED_LEN));
    if (status) return status;
    status = ctr_drbg_update(context, data);
    if (status) return status;

    context->reseed_counter = 1;
    return PSA_SUCCESS;
}

// generate random bytes
psa_status_t oberon_ctr_drbg_get_random(
    oberon_ctr_drbg_context_t *context,
    uint8_t *output,
    size_t output_size)
{
    uint8_t seed[SEED_LEN];
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
        // get new seed
        status = get_entropy(seed, PSA_BYTES_TO_BITS(KEY_LEN));
        if (status) return status;
        // update generator state with new seed
        status = ctr_drbg_update(context, seed);
        if (status) return status;
        context->reseed_counter = 1;
    }

    // generate output
    status = ctr_drbg_cipher(context, NULL, output, output_size);
    if (status) return status;

    // update generator state for backtracking resistance
    status = ctr_drbg_update(context, NULL);
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
psa_status_t oberon_ctr_drbg_free(
    oberon_ctr_drbg_context_t *context)
{
    psa_driver_wrapper_cipher_abort(&context->aes_op);
#ifdef OBERON_USE_MUTEX
    oberon_mutex_free(&context->mutex);
#endif
    memset(context, 0, sizeof *context);
    return PSA_SUCCESS;
}
