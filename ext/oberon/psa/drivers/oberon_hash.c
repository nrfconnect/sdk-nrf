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
#include "oberon_hash.h"

#ifdef PSA_NEED_OBERON_SHA_1
#include "ocrypto_sha1.h"
#endif
#ifdef PSA_NEED_OBERON_SHA_224
#include "ocrypto_sha224.h"
#endif
#ifdef PSA_NEED_OBERON_SHA_256
#include "ocrypto_sha256.h"
#endif
#ifdef PSA_NEED_OBERON_SHA_384
#include "ocrypto_sha384.h"
#endif
#ifdef PSA_NEED_OBERON_SHA_512
#include "ocrypto_sha512.h"
#endif
#ifdef PSA_NEED_OBERON_SHA3
#include "ocrypto_sha3.h"
#endif
#ifdef PSA_NEED_OBERON_SHAKE
#include "ocrypto_shake.h"
#endif


psa_status_t oberon_hash_setup(
    oberon_hash_operation_t *operation,
    psa_algorithm_t alg)
{
    switch (alg) {
#ifdef PSA_NEED_OBERON_SHA_1
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha1_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA_1:
        ocrypto_sha1_init((ocrypto_sha1_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_224
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha224_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA_224:
        ocrypto_sha224_init((ocrypto_sha224_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_256
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha256_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA_256:
        ocrypto_sha256_init((ocrypto_sha256_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_384
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha384_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA_384:
        ocrypto_sha384_init((ocrypto_sha384_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_512
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha512_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA_512:
        ocrypto_sha512_init((ocrypto_sha512_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_sha3_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHA3_224:
    case PSA_ALG_SHA3_256:
    case PSA_ALG_SHA3_384:
    case PSA_ALG_SHA3_512:
        ocrypto_sha3_init((ocrypto_sha3_ctx*)operation->ctx);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHAKE
    _Static_assert(sizeof operation->ctx >= sizeof(ocrypto_shake_ctx), "oberon_hash_operation_t.ctx too small");
    case PSA_ALG_SHAKE256_512:
        ocrypto_shake_init((ocrypto_shake_ctx*)operation->ctx);
        break;
#endif
    default:
        return PSA_ERROR_NOT_SUPPORTED;
    }

    operation->alg = alg;
    return PSA_SUCCESS;
}

psa_status_t oberon_hash_clone(
    const oberon_hash_operation_t *source_operation,
    oberon_hash_operation_t *target_operation)
{
    memcpy(target_operation, source_operation, sizeof *target_operation);
    return PSA_SUCCESS;
}

psa_status_t oberon_hash_update(
    oberon_hash_operation_t *operation,
    const uint8_t *input, size_t input_length)
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_SHA_1
    case PSA_ALG_SHA_1:
        ocrypto_sha1_update((ocrypto_sha1_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_224
    case PSA_ALG_SHA_224:
        ocrypto_sha224_update((ocrypto_sha224_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_256
    case PSA_ALG_SHA_256:
        ocrypto_sha256_update((ocrypto_sha256_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_384
    case PSA_ALG_SHA_384:
        ocrypto_sha384_update((ocrypto_sha384_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_512
    case PSA_ALG_SHA_512:
        ocrypto_sha512_update((ocrypto_sha512_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_224
    case PSA_ALG_SHA3_224:
        ocrypto_sha3_224_update((ocrypto_sha3_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_256
    case PSA_ALG_SHA3_256:
        ocrypto_sha3_256_update((ocrypto_sha3_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_384
    case PSA_ALG_SHA3_384:
        ocrypto_sha3_384_update((ocrypto_sha3_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_512
    case PSA_ALG_SHA3_512:
        ocrypto_sha3_512_update((ocrypto_sha3_ctx*)operation->ctx, input, input_length);
        break;
#endif
#ifdef PSA_NEED_OBERON_SHAKE256_512
    case PSA_ALG_SHAKE256_512:
        ocrypto_shake256_update((ocrypto_shake_ctx*)operation->ctx, input, input_length);
        break;
#endif
    default:
        (void)input;
        (void)input_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_hash_finish(
    oberon_hash_operation_t *operation,
    uint8_t *hash, size_t hash_size, size_t *hash_length)
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_SHA_1
    case PSA_ALG_SHA_1:
        if (hash_size < ocrypto_sha1_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha1_final((ocrypto_sha1_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha1_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_224
    case PSA_ALG_SHA_224:
        if (hash_size < ocrypto_sha224_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha224_final((ocrypto_sha224_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha224_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_256
    case PSA_ALG_SHA_256:
        if (hash_size < ocrypto_sha256_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha256_final((ocrypto_sha256_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha256_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_384
    case PSA_ALG_SHA_384:
        if (hash_size < ocrypto_sha384_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha384_final((ocrypto_sha384_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha384_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_512
    case PSA_ALG_SHA_512:
        if (hash_size < ocrypto_sha512_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha512_final((ocrypto_sha512_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha512_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_224
    case PSA_ALG_SHA3_224:
        if (hash_size < ocrypto_sha3_224_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_224_final((ocrypto_sha3_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha3_224_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_256
    case PSA_ALG_SHA3_256:
        if (hash_size < ocrypto_sha3_256_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_256_final((ocrypto_sha3_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha3_256_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_384
    case PSA_ALG_SHA3_384:
        if (hash_size < ocrypto_sha3_384_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_384_final((ocrypto_sha3_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha3_384_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_512
    case PSA_ALG_SHA3_512:
        if (hash_size < ocrypto_sha3_512_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_512_final((ocrypto_sha3_ctx*)operation->ctx, hash);
        *hash_length = ocrypto_sha3_512_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHAKE256_512
    case PSA_ALG_SHAKE256_512:
        if (hash_size < PSA_BITS_TO_BYTES(512)) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_shake256_final((ocrypto_shake_ctx*)operation->ctx, hash, PSA_BITS_TO_BYTES(512));
        *hash_length = PSA_BITS_TO_BYTES(512);
        break;
#endif
    default:
        (void)hash;
        (void)hash_size;
        (void)hash_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}

psa_status_t oberon_hash_abort(
    oberon_hash_operation_t *operation)
{
    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}


psa_status_t oberon_hash_compute(
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *hash, size_t hash_size, size_t *hash_length)
{
    switch (alg) {
#ifdef PSA_NEED_OBERON_SHA_1
    case PSA_ALG_SHA_1:
        if (hash_size < ocrypto_sha1_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha1(hash, input, input_length);
        *hash_length = ocrypto_sha1_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_224
    case PSA_ALG_SHA_224:
        if (hash_size < ocrypto_sha224_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha224(hash, input, input_length);
        *hash_length = ocrypto_sha224_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_256
    case PSA_ALG_SHA_256:
        if (hash_size < ocrypto_sha256_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha256(hash, input, input_length);
        *hash_length = ocrypto_sha256_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_384
    case PSA_ALG_SHA_384:
        if (hash_size < ocrypto_sha384_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha384(hash, input, input_length);
        *hash_length = ocrypto_sha384_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA_512
    case PSA_ALG_SHA_512:
        if (hash_size < ocrypto_sha512_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha512(hash, input, input_length);
        *hash_length = ocrypto_sha512_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_224
    case PSA_ALG_SHA3_224:
        if (hash_size < ocrypto_sha3_224_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_224(hash, input, input_length);
        *hash_length = ocrypto_sha3_224_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_256
    case PSA_ALG_SHA3_256:
        if (hash_size < ocrypto_sha3_256_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_256(hash, input, input_length);
        *hash_length = ocrypto_sha3_256_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_384
    case PSA_ALG_SHA3_384:
        if (hash_size < ocrypto_sha3_384_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_384(hash, input, input_length);
        *hash_length = ocrypto_sha3_384_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHA3_512
    case PSA_ALG_SHA3_512:
        if (hash_size < ocrypto_sha3_512_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_sha3_512(hash, input, input_length);
        *hash_length = ocrypto_sha3_512_BYTES;
        break;
#endif
#ifdef PSA_NEED_OBERON_SHAKE256_512
    case PSA_ALG_SHAKE256_512:
        if (hash_size < PSA_BITS_TO_BYTES(512)) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_shake256(hash, PSA_BITS_TO_BYTES(512), input, input_length);
        *hash_length = PSA_BITS_TO_BYTES(512);
        break;
#endif
    default:
        (void)input;
        (void)input_length;
        (void)hash;
        (void)hash_size;
        (void)hash_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}
