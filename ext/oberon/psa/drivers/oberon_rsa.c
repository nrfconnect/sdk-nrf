/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_rsa.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"

#include "ocrypto_rsa_primitives.h"


#define SEQ_TAG  0x30
#define INT_TAG  0x02
#define BIT_TAG  0x03


typedef struct {
    uint16_t index;
    uint16_t length;
    uint16_t bits;
} integer_info_t;

typedef struct {
    uint32_t e;
    integer_info_t n;
    integer_info_t p;
    integer_info_t q;
    integer_info_t dp;
    integer_info_t dq;
    integer_info_t qi;
    psa_key_type_t type;
    size_t start, length; // public part
    size_t data_size;
} key_info_t;


static const uint8_t RSA_ALGORITHM_IDENTIFIER[] = {
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00};


static psa_status_t oberon_parse_length(
    const uint8_t *data, size_t data_length,
    size_t *index, size_t *length)
{
    size_t len, idx = *index;
    if (idx >= data_length) return PSA_ERROR_INVALID_ARGUMENT;
    len = data[idx++];
    if (len == 0x81) {
        if (idx >= data_length) return PSA_ERROR_INVALID_ARGUMENT;
        len = data[idx++];
    } else if (len == 0x82) {
        if (idx >= data_length - 1) return PSA_ERROR_INVALID_ARGUMENT;
        len = data[idx++];
        len = len * 256 + data[idx++];
    } else if (len >= 0x80) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (idx + len > data_length) return PSA_ERROR_INVALID_ARGUMENT;
    *index = idx;
    *length = len;
    return PSA_SUCCESS;
}

static psa_status_t oberon_parse_integer(
    integer_info_t *info,
    const uint8_t *data, size_t length,
    size_t *index,
    uint32_t *value) // may be NULL
{
    psa_status_t status;
    size_t idx = *index;
    size_t len, bits, i;
    uint8_t first;
    uint32_t val;

    if (idx >= length) return PSA_ERROR_INVALID_ARGUMENT;
    if (data[idx++] != INT_TAG) return PSA_ERROR_INVALID_ARGUMENT;
    status = oberon_parse_length(data, length, &idx, &len);
    if (status) return status;
    if (len == 0) return PSA_ERROR_INVALID_ARGUMENT;
    info->index = (uint16_t)idx;
    info->length = (uint16_t)len;
    *index = idx + len;

    first = data[idx++];
    if (first >= 0x80) return PSA_ERROR_INVALID_ARGUMENT; // negative
    if (len >= 2 && first == 0) { // leading zero byte
        len--;
        first = data[idx++];
        if (first == 0) return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (value) {
        if (len >= 4) return PSA_ERROR_INVALID_ARGUMENT;
        val = first;
        for (i = 1; i < len; i++) {
            val = val * 256 + data[idx++];
        }
        *value = val;
    }

    bits = PSA_BYTES_TO_BITS(len);
    while (bits > 0 && (first & 0x80) == 0) { // leading zero bits
        bits--;
        first <<= 1;
    }
    info->bits = (uint16_t)bits;

    return PSA_SUCCESS;
}

static psa_status_t oberon_parse_rsa_key(
    key_info_t *info,
    const uint8_t *key, size_t key_length)
{
    psa_status_t status;
    size_t index = 0;
    size_t seq_len, seq_end;
    uint32_t value;
    size_t i;

    // sequence
    if (key_length < 20) return PSA_ERROR_INVALID_ARGUMENT;
    if (key[index++] != SEQ_TAG) return PSA_ERROR_INVALID_ARGUMENT;
    status = oberon_parse_length(key, key_length, &index, &seq_len);
    if (status) return status;
    seq_end = index + seq_len;

    if (info->type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        // version
        status = oberon_parse_integer(&info->n, key, key_length, &index, &value);
        if (status) return status;
        if (value != 0) return PSA_ERROR_INVALID_ARGUMENT; // wrong version

        info->start = index;
    } else {
        if (key[index] == SEQ_TAG) { // skip AlgorithmIdentifier prefix
            index++;
            status = oberon_parse_length(key, key_length, &index, &seq_len);
            if (status) return status;
            if (seq_len != sizeof RSA_ALGORITHM_IDENTIFIER) return PSA_ERROR_INVALID_ARGUMENT;
            for (i = 0; i < sizeof RSA_ALGORITHM_IDENTIFIER; i++) {
                if (key[index++] != RSA_ALGORITHM_IDENTIFIER[i]) return PSA_ERROR_INVALID_ARGUMENT;
            }
            if (key[index++] != BIT_TAG) return PSA_ERROR_INVALID_ARGUMENT;
            status = oberon_parse_length(key, key_length, &index, &seq_len);
            if (status) return status;
            if (index + seq_len != seq_end) return PSA_ERROR_INVALID_ARGUMENT;
            if (key[index++] != 0) return PSA_ERROR_INVALID_ARGUMENT;
            if (key[index++] != SEQ_TAG) return PSA_ERROR_INVALID_ARGUMENT;
            status = oberon_parse_length(key, key_length, &index, &seq_len);
            if (status) return status;
            if (index + seq_len != seq_end) return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    // n
    status = oberon_parse_integer(&info->n, key, key_length, &index, NULL);
    if (status) return status;

    // e
    status = oberon_parse_integer(&info->p, key, key_length, &index, &info->e);
    if (status) return status;

    if (info->type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        info->length = index - info->start;

        // d
        status = oberon_parse_integer(&info->p, key, key_length, &index, NULL);
        if (status) return status;
        if (info->p.bits > info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;

        // p
        status = oberon_parse_integer(&info->p, key, key_length, &index, NULL);
        if (status) return status;
        if (info->p.bits * 2 != info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;

        // q
        status = oberon_parse_integer(&info->q, key, key_length, &index, NULL);
        if (status) return status;
        if (info->q.bits * 2 != info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;

        // d mod (p-1)
        status = oberon_parse_integer(&info->dp, key, key_length, &index, NULL);
        if (status) return status;
        if (info->dp.bits * 2 > info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;

        // d mod (q-1)
        status = oberon_parse_integer(&info->dq, key, key_length, &index, NULL);
        if (status) return status;
        if (info->dq.bits * 2 > info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;

        // q^-1 mod p
        status = oberon_parse_integer(&info->qi, key, key_length, &index, NULL);
        if (status) return status;
        if (info->qi.bits * 2 > info->n.bits) return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (index != seq_end) return PSA_ERROR_INVALID_ARGUMENT;
    info->data_size = index;
    return PSA_SUCCESS;
}


psa_status_t oberon_export_rsa_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    uint8_t *data, size_t data_size, size_t *data_length)
{
    psa_status_t status;
    key_info_t key_info;
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
    case PSA_KEY_TYPE_RSA_PUBLIC_KEY:
        if (key_length > data_size) return PSA_ERROR_BUFFER_TOO_SMALL;
        memcpy(data, key, key_length);
        *data_length = key_length;
        return PSA_SUCCESS;
    case PSA_KEY_TYPE_RSA_KEY_PAIR:
        // extract public key (n & e) from secret key
        key_info.type = type;
        status = oberon_parse_rsa_key(&key_info, key, key_length);
        if (status) return status;
        if (key_info.length >= 256) {
            if (key_info.length + 4 > data_size) return PSA_ERROR_BUFFER_TOO_SMALL;
            data[0] = SEQ_TAG;
            data[1] = 0x82;
            data[2] = (uint8_t)(key_info.length >> 8);
            data[3] = (uint8_t)key_info.length;
            memcpy(&data[4], &key[key_info.start], key_info.length);
            *data_length = key_info.length + 4;
        } else {
            if (key_info.length + 3 > data_size) return PSA_ERROR_BUFFER_TOO_SMALL;
            data[0] = SEQ_TAG;
            data[1] = 0x81;
            data[2] = (uint8_t)key_info.length;
            memcpy(&data[3], &key[key_info.start], key_info.length);
            *data_length = key_info.length + 3;
        }
        return PSA_SUCCESS;
    default:
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_import_rsa_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key, size_t key_size, size_t *key_length,
    size_t *key_bits)
{
    psa_status_t status;
    key_info_t key_info;
    psa_key_type_t type = psa_get_key_type(attributes);

    if (!PSA_KEY_TYPE_IS_RSA(type)) return PSA_ERROR_NOT_SUPPORTED;

    key_info.type = type;
    status = oberon_parse_rsa_key(&key_info, data, data_length);
    if (status) return status;

    if (key_info.data_size > key_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(key, data, key_info.data_size);
    *key_length = key_info.data_size;
    *key_bits = key_info.n.bits;

    return PSA_SUCCESS;
}


// constant time zero test
// 0x00 -> 1, 0x01..0xFF -> 0
#define IS_ZERO(x) (((uint32_t)(x) - 1) >> 31)


#ifdef PSA_NEED_OBERON_RSA_PSS
static const uint8_t zero[8] = {0};
#endif

#if defined(PSA_NEED_OBERON_RSA_PSS) || defined(PSA_NEED_OBERON_RSA_OAEP)
// mask generation function and xor
// data ^= MGF(in, data_len)
static psa_status_t oberon_apply_mgf(
    psa_hash_operation_t *hash_op,
    psa_algorithm_t hash_alg,
    const uint8_t *in, size_t in_len,
    uint8_t *data, size_t data_len)
{
    psa_status_t status;
    uint8_t counter[4] = {0, 0, 0, 0};
    uint8_t mask[PSA_HASH_MAX_SIZE];
    size_t block_len, length;
    size_t hash_len = PSA_HASH_LENGTH(hash_alg);

    while (data_len > 0) {
        status = psa_driver_wrapper_hash_setup(hash_op, hash_alg);
        if (status) return status;
        status = psa_driver_wrapper_hash_update(hash_op, in, in_len);
        if (status) return status;
        status = psa_driver_wrapper_hash_update(hash_op, counter, sizeof counter);
        if (status) return status;
        status = psa_driver_wrapper_hash_finish(hash_op, mask, hash_len, &length);
        if (status) return status;
        block_len = hash_len;
        if (block_len > data_len) block_len = data_len;
        oberon_xor(data, data, mask, block_len);
        data += block_len;
        data_len -= block_len;
        counter[3]++;
    }

    return PSA_SUCCESS;
}
#endif /* PSA_NEED_OBERON_RSA_PSS || PSA_NEED_OBERON_RSA_OAEP */

// PSS padding

#ifdef PSA_NEED_OBERON_RSA_PSS
static psa_status_t emsa_pss_encode(
    psa_algorithm_t hash_alg,
    const uint8_t *hash, size_t hash_len,
    uint8_t *em, size_t em_len)
{
    psa_status_t status;
    size_t db_len = em_len - hash_len - 1;
    uint8_t *h = &em[db_len]; // em = db : hash : 0xbc
    uint8_t *salt;
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    size_t salt_len, length;

    // salt length = max length <= hash length
    salt_len = hash_len;
    if (salt_len >= db_len) salt_len = db_len - 1;
    memset(em, 0, db_len); // db
    salt = &em[db_len - salt_len]; // db = 0..0 : 1 : salt

    // salt
    status = psa_generate_random(salt, salt_len);
    if (status) return status;

    status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, zero, 8); // 8 zero bytes
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, hash, hash_len); // mHash
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, salt, salt_len); // salt
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, h, hash_len, &length); // H
    if (status) goto exit;

    em[db_len - salt_len - 1] = 1;
    status = oberon_apply_mgf(&hash_op, hash_alg, h, hash_len, em, db_len); // db^mgf(H)
    if (status) goto exit;
    em[em_len - 1] = 0xBC;
    em[0] &= 0x7F; // truncate to key_bits - 1

    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_hash_abort(&hash_op);
    return status;
}

static psa_status_t emsa_pss_verify(
    psa_algorithm_t hash_alg,
    const uint8_t *hash, size_t hash_len,
    size_t salt_len,
    uint8_t *em, size_t em_len)
{
    psa_status_t status;
    size_t db_len = em_len - hash_len - 1;
    uint8_t *h = &em[db_len]; // em = db : H : 0xbc
    uint8_t h1[PSA_HASH_MAX_SIZE];
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    size_t z, length;
    int diff;

    // verify signature
    diff = (int)(em[em_len - 1] ^ 0xBC); // check em[last] == 0xBC
    diff |= (int)(em[0] & 0x80);         // check (em[0] & 0x80) == 0

    status = oberon_apply_mgf(&hash_op, hash_alg, h, hash_len, em, db_len); // db^mgf(H)
    if (status) goto exit;
    em[0] &= 0x7F; // truncate to key_bits - 1

    if (salt_len == 0) {
        // get salt length from input
        for (z = 0; z < db_len - 1 && em[z] == 0; z++);
        salt_len = db_len - 1 - z;
    } else {
        // reduce salt length if needed
        if (salt_len >= db_len) salt_len = db_len - 1;
        z = db_len - salt_len - 1;
    }

    if (z > 0) {
        diff |= oberon_ct_compare_zero(em, z);
    }
    diff |= (int)(em[z] ^ 1); // check em[db_len - s_len - 1] == 1

    status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, zero, 8); // 8 zero bytes
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, hash, hash_len); // mHash
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, &em[db_len - salt_len], salt_len); // salt
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, h1, hash_len, &length); // H
    if (status) goto exit;

    diff |= oberon_ct_compare(h1, h, hash_len); // H == H'
    if (diff) return PSA_ERROR_INVALID_SIGNATURE;
    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_hash_abort(&hash_op);
    return status;
}
#endif /* PSA_NEED_OBERON_RSA_PSS */

// OAEP padding

#ifdef PSA_NEED_OBERON_RSA_OAEP
static psa_status_t eme_oaep_encode(
    psa_algorithm_t hash_alg,
    const uint8_t *m, size_t m_len,
    const uint8_t *label, size_t label_len,
    uint8_t *em, size_t em_len)
{
    size_t hash_len = PSA_HASH_LENGTH(hash_alg);
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    psa_status_t status;
    size_t length;

    uint8_t *seed = &em[1];
    uint8_t *db = &em[1 + hash_len];
    size_t db_len = em_len - 1 - hash_len;

    if (m_len > em_len - 2 * hash_len - 2) return PSA_ERROR_INVALID_ARGUMENT;

    status = psa_generate_random(seed, hash_len);
    if (status) return status;

    // db = lhash : {0} : 1 : m
    memset(em, 0, em_len);
    status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, label, label_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, db, hash_len, &length); // lhash
    if (status) goto exit;
    em[em_len - 1 - m_len] = 1; // 1
    memcpy(&em[em_len - m_len], m, m_len); // m

    // em = 0 : seed^mgf : db^mgf
    status = oberon_apply_mgf(&hash_op, hash_alg, seed, hash_len, db, db_len); // db ^= mgf(seed)
    if (status) goto exit;
    status = oberon_apply_mgf(&hash_op, hash_alg, db, db_len, seed, hash_len); // seed ^= mgf(db)
    if (status) goto exit;
    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_hash_abort(&hash_op);
    return status;
}

static psa_status_t eme_oaep_decode(
    psa_algorithm_t hash_alg,
    uint8_t *em, size_t em_len,
    const uint8_t *label, size_t label_len,
    uint8_t *m, size_t m_size, size_t *m_len)
{
    size_t hash_len = PSA_HASH_LENGTH(hash_alg);
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    psa_status_t status;
    size_t i, inc, length;
    int diff;

    uint8_t *seed = &em[1];
    uint8_t *db = &em[1 + hash_len];
    size_t db_len = em_len - 1 - hash_len;

    if (em_len < 2 * hash_len + 2) return PSA_ERROR_INVALID_ARGUMENT;
    diff = (int)em[0]; // check em[0] == 0

    status = oberon_apply_mgf(&hash_op, hash_alg, db, db_len, seed, hash_len); // seed ^= mgf(db)
    if (status) goto exit;
    status = oberon_apply_mgf(&hash_op, hash_alg, seed, hash_len, db, db_len); // db ^= mgf(seed)
    if (status) goto exit;

    status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, label, label_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, seed, hash_len, &length); // lhash
    if (status) goto exit;
    diff |= oberon_ct_compare(seed, db, hash_len); // lhash == lhash' ?

    // db = lhash' : {0} : 1 : m
    length = db_len - hash_len - 1;
    inc = 1;
    for (i = hash_len; i < db_len - 1; i++) {
        inc &= IS_ZERO(db[i]);
        length -= inc;
    }
    diff |= (int)(db[db_len - length - 1] ^ 1); // check db[i] == 1
    if (diff) return PSA_ERROR_INVALID_PADDING;

    if (length > m_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    *m_len = length;
    memcpy(m, db + db_len - length, length);
    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_hash_abort(&hash_op);
    return status;
}
#endif /* PSA_NEED_OBERON_RSA_OAEP */


// PKCS#1 v1.5 padding

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_SIGN

// PKCS#1 v1.5 hash digest
#ifdef PSA_WANT_ALG_SHA_1
static const uint8_t DIGEST_INFO_SHA1[] = {
    0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
    0x1a, 0x05, 0x00, 0x04, 0x14};
#endif
#ifdef PSA_WANT_ALG_SHA_224
static const uint8_t DIGEST_INFO_SHA224[] = {
    0x30, 0x2d, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x04, 0x05, 0x00, 0x04, 0x1c};
#endif
#ifdef PSA_WANT_ALG_SHA_256
static const uint8_t DIGEST_INFO_SHA256[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
#endif
#ifdef PSA_WANT_ALG_SHA_384
static const uint8_t DIGEST_INFO_SHA384[] = {
    0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30};
#endif
#ifdef PSA_WANT_ALG_SHA_512
static const uint8_t DIGEST_INFO_SHA512[] = {
    0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40};
#endif

static psa_status_t emsa_pkcs1_v15_get_digest(
    psa_algorithm_t hash_alg,
    const uint8_t **digest, size_t *d_len)
{
    switch (hash_alg) {
#ifdef PSA_WANT_ALG_SHA_1
    case PSA_ALG_SHA_1:
        *digest = DIGEST_INFO_SHA1;
        *d_len = sizeof DIGEST_INFO_SHA1;
        return PSA_SUCCESS;
#endif
#ifdef PSA_WANT_ALG_SHA_224
    case PSA_ALG_SHA_224:
        *digest = DIGEST_INFO_SHA224;
        *d_len = sizeof DIGEST_INFO_SHA224;
        return PSA_SUCCESS;
#endif
#ifdef PSA_WANT_ALG_SHA_256
    case PSA_ALG_SHA_256:
        *digest = DIGEST_INFO_SHA256;
        *d_len = sizeof DIGEST_INFO_SHA256;
        return PSA_SUCCESS;
#endif
#ifdef PSA_WANT_ALG_SHA_384
    case PSA_ALG_SHA_384:
        *digest = DIGEST_INFO_SHA384;
        *d_len = sizeof DIGEST_INFO_SHA384;
        return PSA_SUCCESS;
#endif
#ifdef PSA_WANT_ALG_SHA_512
    case PSA_ALG_SHA_512:
        *digest = DIGEST_INFO_SHA512;
        *d_len = sizeof DIGEST_INFO_SHA512;
        return PSA_SUCCESS;
#endif
    default:
        (void)digest;
        (void)d_len;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

static psa_status_t emsa_pkcs1_v15_encode(
    psa_algorithm_t hash_alg,
    const uint8_t *input, size_t input_len,
    uint8_t *em, size_t em_len)
{
    psa_status_t status;
    size_t n, d_len = 0, t_len;
    const uint8_t *digest = NULL;

    if (hash_alg != PSA_ALG_NONE) {
        status = emsa_pkcs1_v15_get_digest(hash_alg, &digest, &d_len);
        if (status) return status;
    }

    t_len = d_len + input_len;
    if (em_len < t_len + 11) return PSA_ERROR_INVALID_ARGUMENT;
    n = em_len - t_len;
    em[0] = 0;
    em[1] = 1;
    memset(&em[2], 0xFF, n - 3);
    em[n - 1] = 0;
    if (d_len) memcpy(&em[n], digest, d_len);
    memcpy(&em[n + d_len], input, input_len);

    return PSA_SUCCESS;
}

static psa_status_t emsa_pkcs1_v15_verify(
    psa_algorithm_t hash_alg,
    const uint8_t *hash, size_t hash_len,
    uint8_t *em, size_t em_len)
{
    psa_status_t status;
    size_t i, n, d_len = 0, t_len;
    const uint8_t *digest = NULL;
    int diff;

    if (hash_alg != PSA_ALG_NONE) {
        status = emsa_pkcs1_v15_get_digest(hash_alg, &digest, &d_len);
        if (status) return status;
    }

    t_len = d_len + hash_len;
    if (em_len < t_len + 11) return PSA_ERROR_INVALID_SIGNATURE;
    n = em_len - t_len;

    diff = (int)em[0]; // check em[0] == 0
    diff |= (int)(em[1] ^ 1); // check em[1] == 1
    for (i = 2; i < n - 1; i++) diff |= (int)(em[i] ^ 0xFF); // check em[i] == 0xFF
    diff |= (int)em[n - 1]; // check em[n-1] == 0
    if (d_len) diff |= oberon_ct_compare(&em[n], digest, d_len);
    diff |= oberon_ct_compare(&em[n + d_len], hash, hash_len);
    if (diff) return PSA_ERROR_INVALID_SIGNATURE;

    return PSA_SUCCESS;
}
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_SIGN */

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT
static psa_status_t eme_pkcs1_v15_encode(
    const uint8_t *m, size_t m_len,
    uint8_t *em, size_t em_len)
{
    psa_status_t status;
    uint32_t seed;
    size_t i;

    if (em_len < m_len + 11) return PSA_ERROR_INVALID_ARGUMENT;

    // generate non-zero random bytes
    status = psa_generate_random(em, em_len - m_len - 1);
    if (status) return status;
    seed = (em[0] << 16) + (em[1] << 8);
    for (i = 2; i < em_len - m_len - 1; i++) {
        seed ^= em[i]; // 0 .. 0xFFFFFF
        seed *= 255; // 0 .. 0xFEFFFF01
        em[i] = (uint8_t)((seed >> 24) + 1); // 1 .. 255
        seed &= 0xFFFFFF;
    }

    // em = 0 : 2 : {rnd} : 0 : m
    em[0] = 0;
    em[1] = 2;
    em[em_len - m_len - 1] = 0;
    memcpy(&em[em_len - m_len], m, m_len);

    return PSA_SUCCESS;
}

static psa_status_t eme_pkcs1_v15_decode(
    const uint8_t *em, size_t em_len,
    uint8_t *m, size_t m_size, size_t *m_len)
{
    size_t i, inc, length;
    int diff;

    // em = 0 : 2 : {rnd} : 0 : m
    diff = (int)em[0]; // check em[0] == 0
    diff |= (int)(em[1] ^ 2); // check em[1] == 2
    for (i = 2; i < 10; i++) {
        diff |= IS_ZERO(em[i]); // check em[i] != 0
    }
    length = 0;
    inc = 0;
    for (i = 10; i < em_len - 1; i++) {
        inc |= IS_ZERO(em[i]);
        length += inc;
    }
    diff |= (int)(em[em_len - length - 1]); // check em[i] == 0
    if (diff) return PSA_ERROR_INVALID_PADDING;

    if (length > m_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    *m_len = length;
    memcpy(m, &em[em_len - length], length);

    return PSA_SUCCESS;
}
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT */


#if PSA_MAX_RSA_KEY_BITS > 0
static psa_status_t rsa_key_setup(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    ocrypto_rsa_crt_key *crt_key,
    ocrypto_rsa_pub_key *pub_key,
    uint32_t *key_mem)
{
    int res = 0;
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    key_info_t key_info;

    key_info.type = psa_get_key_type(attributes);
    status = oberon_parse_rsa_key(&key_info, key, key_length);
    if (status) return status;

    switch (bits) {
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_1024
    case 1024:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_1536
    case 1536:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_2048
    case 2048:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_3072
    case 3072:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_4096
    case 4096:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_6144
    case 6144:
#endif
#ifdef PSA_NEED_OBERON_RSA_KEY_SIZE_8192
    case 8192:
#endif
        if (crt_key) {
            res = ocrypto_rsa_init_crt_key(
                crt_key, key_mem,
                key + key_info.p.index, key_info.p.length,
                key + key_info.q.index, key_info.q.length,
                key + key_info.dp.index, key_info.dp.length,
                key + key_info.dq.index, key_info.dq.length,
                key + key_info.qi.index, key_info.qi.length);
        }
        if (pub_key) {
            res = ocrypto_rsa_init_pub_key(
                pub_key, key_mem,
                key + key_info.n.index, key_info.n.length,
                key_info.e);
        }
        break;
    default:
        return PSA_ERROR_NOT_SUPPORTED;
    }
    if (res) return PSA_ERROR_INVALID_ARGUMENT;
    return PSA_SUCCESS;
}
#endif

psa_status_t oberon_rsa_sign_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length)
{
    int res;
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
    size_t key_size = PSA_BITS_TO_BYTES(bits);
#if PSA_MAX_RSA_KEY_BITS > 0
    ocrypto_rsa_crt_key crt_key;
    uint32_t key_mem[OCRYPTO_RSA_CRT_KEY_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint32_t mem[OCRYPTO_RSA_CRT_MEM_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint8_t *em = (uint8_t*)mem;
#endif

    if (type != PSA_KEY_TYPE_RSA_KEY_PAIR) return PSA_ERROR_NOT_SUPPORTED;
    psa_algorithm_t hash_alg = PSA_ALG_SIGN_GET_HASH(alg);
    if (hash_alg != 0 && hash_length != PSA_HASH_LENGTH(hash_alg)) return PSA_ERROR_INVALID_ARGUMENT;
    if (signature_size < key_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    *signature_length = key_size;

    // Get secret key

#if PSA_MAX_RSA_KEY_BITS > 0
    status = rsa_key_setup(attributes, key, key_length, &crt_key, NULL, key_mem);
    if (status) return status;
#endif

    // Apply padding

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_SIGN
    if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
        status = emsa_pkcs1_v15_encode(hash_alg, hash, hash_length, em, key_size);
        if (status) return status;
    } else
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_SIGN */
#ifdef PSA_NEED_OBERON_RSA_PSS
    if (PSA_ALG_IS_RSA_PSS(alg)) {
        status = emsa_pss_encode(hash_alg, hash, hash_length, em, key_size);
        if (status) return status;
    } else
#endif /* PSA_NEED_OBERON_RSA_PSS */
    {
        (void)key;
        (void)key_length;
        (void)signature;
        (void)hash;
        (void)res;
        (void)status;
        return PSA_ERROR_INVALID_ARGUMENT; //  PSA_ERROR_NOT_SUPPORTED; // ???
    }

    // RSA decryption primitive

#if PSA_MAX_RSA_KEY_BITS > 0
    res = ocrypto_rsa_crt_exp(signature, key_size, em, key_size, &crt_key, mem);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;
#endif

    return PSA_SUCCESS;
}

psa_status_t oberon_rsa_verify_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length)
{
    int res;
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
    size_t key_size = PSA_BITS_TO_BYTES(bits);
#if PSA_MAX_RSA_KEY_BITS > 0
    ocrypto_rsa_pub_key pub_key;
    uint32_t key_mem[OCRYPTO_RSA_PUB_KEY_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint32_t mem[OCRYPTO_RSA_PUB_MEM_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint8_t *em = (uint8_t*)mem;
#endif

    if (!PSA_KEY_TYPE_IS_RSA(type)) return PSA_ERROR_NOT_SUPPORTED;
    psa_algorithm_t hash_alg = PSA_ALG_SIGN_GET_HASH(alg);
    if (hash_alg != 0 && hash_length != PSA_HASH_LENGTH(hash_alg)) return PSA_ERROR_INVALID_ARGUMENT;
    if (signature_length != key_size) return PSA_ERROR_INVALID_SIGNATURE;

    // Get public key

#if PSA_MAX_RSA_KEY_BITS > 0
    status = rsa_key_setup(attributes, key, key_length, NULL, &pub_key, key_mem);
    if (status) return status;

    // RSA encryption primitive

    res = ocrypto_rsa_pub_exp(em, key_size, signature, key_size, &pub_key, mem);
    if (res != 0) return PSA_ERROR_INVALID_SIGNATURE;
#endif

    // Check padding

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_SIGN
    if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
        return emsa_pkcs1_v15_verify(hash_alg, hash, hash_length, em, key_size);
    } else
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_SIGN */
#ifdef PSA_NEED_OBERON_RSA_PSS
    if (PSA_ALG_IS_RSA_PSS(alg)) {
        if (PSA_ALG_IS_RSA_PSS_ANY_SALT(alg)) {
            return emsa_pss_verify(hash_alg, hash, hash_length, 0, em, key_size);
        } else {
            return emsa_pss_verify(hash_alg, hash, hash_length, hash_length, em, key_size);
        }
    } else
#endif /* PSA_NEED_OBERON_RSA_PSS */
    {
        (void)key;
        (void)key_length;
        (void)signature;
        (void)hash;
        (void)res;
        (void)status;
        return PSA_ERROR_INVALID_ARGUMENT; // PSA_ERROR_NOT_SUPPORTED; // ???
    }
}

psa_status_t oberon_rsa_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key,
				size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *salt, size_t salt_length,
				uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
    size_t key_size = PSA_BITS_TO_BYTES(bits);
#if PSA_MAX_RSA_KEY_BITS > 0
    ocrypto_rsa_pub_key pub_key;
    uint32_t key_mem[OCRYPTO_RSA_PUB_KEY_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint32_t mem[OCRYPTO_RSA_PUB_MEM_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint8_t *em = (uint8_t*)mem;
#endif

    if (!PSA_KEY_TYPE_IS_RSA(type)) return PSA_ERROR_NOT_SUPPORTED;
    if (output_size < key_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    *output_length = key_size;

    // Get public key

#if PSA_MAX_RSA_KEY_BITS > 0
    status = rsa_key_setup(attributes, key, key_length, NULL, &pub_key, key_mem);
    if (status) return status;
#endif

    // Apply padding

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT
    if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
        status = eme_pkcs1_v15_encode(input, input_length, em, key_size);
        if (status) return status;
    } else
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT */
#ifdef PSA_NEED_OBERON_RSA_OAEP
    if (PSA_ALG_IS_RSA_OAEP(alg)) {
        psa_algorithm_t hash_alg = PSA_ALG_RSA_OAEP_GET_HASH(alg);
        status = eme_oaep_encode(hash_alg, input, input_length, salt, salt_length, em, key_size);
        if (status) return status;
    } else
#endif /* PSA_NEED_OBERON_RSA_OAEP */
    {
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)salt;
        (void)salt_length;
        (void)output;
        (void)status;
        return PSA_ERROR_INVALID_ARGUMENT; //  PSA_ERROR_NOT_SUPPORTED; // ???
    }

    // RSA encryption primitive

#if PSA_MAX_RSA_KEY_BITS > 0
    ocrypto_rsa_pub_exp(output, key_size, em, key_size, &pub_key, mem);
#endif

    return PSA_SUCCESS;
}

psa_status_t oberon_rsa_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key,
				size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *salt, size_t salt_length,
				uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
    size_t key_size = PSA_BITS_TO_BYTES(bits);
#if PSA_MAX_RSA_KEY_BITS > 0
    ocrypto_rsa_crt_key crt_key;
    uint32_t key_mem[OCRYPTO_RSA_CRT_KEY_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint32_t mem[OCRYPTO_RSA_CRT_MEM_SIZE(PSA_MAX_RSA_KEY_BITS)];
    uint8_t *em = (uint8_t*)mem;
#endif

    if (type != PSA_KEY_TYPE_RSA_KEY_PAIR) return PSA_ERROR_NOT_SUPPORTED;

    if (input_length != key_size) return PSA_ERROR_INVALID_ARGUMENT;


    // Get secret key

#if PSA_MAX_RSA_KEY_BITS > 0
    status = rsa_key_setup(attributes, key, key_length, &crt_key, NULL, key_mem);
    if (status) return status;

    // RSA decryption primitive

    res = ocrypto_rsa_crt_exp(em, key_size, input, key_size, &crt_key, mem);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;
#endif

    // Check padding

#ifdef PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT
    if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
        return eme_pkcs1_v15_decode(em, key_size, output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT */
#ifdef PSA_NEED_OBERON_RSA_OAEP
    if (PSA_ALG_IS_RSA_OAEP(alg)) {
        psa_algorithm_t hash_alg = PSA_ALG_RSA_OAEP_GET_HASH(alg);
        return eme_oaep_decode(hash_alg, em, key_size, salt, salt_length, output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_RSA_OAEP */
    {
        (void)alg;
        (void)key;
        (void)key_length;
        (void)input;
        (void)salt;
        (void)salt_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)res;
        (void)status;
        return PSA_ERROR_INVALID_ARGUMENT; // PSA_ERROR_NOT_SUPPORTED; // ???
    }

    return PSA_SUCCESS;
}
