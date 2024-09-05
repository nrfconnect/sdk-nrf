/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_CRYPTO_DEFS_H__
#define __TFM_CRYPTO_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psa/crypto.h"
#ifdef PLATFORM_DEFAULT_CRYPTO_KEYS
#include "crypto_keys/tfm_builtin_key_ids.h"
#else
#include "tfm_builtin_key_ids.h"
#endif /* PLATFORM_DEFAULT_CRYPTO_KEYS */

/**
 * \brief The maximum supported length of a nonce through the TF-M
 *        interfaces
 */
#define TFM_CRYPTO_MAX_NONCE_LENGTH (16u)

/**
 * \brief This type is used to overcome a limitation in the number of maximum
 *        IOVECs that can be used especially in psa_aead_encrypt and
 *        psa_aead_decrypt. By using this type we pack the nonce and the actual
 *        nonce_length at part of the same structure
 */
struct tfm_crypto_aead_pack_input {
    uint8_t nonce[TFM_CRYPTO_MAX_NONCE_LENGTH];
    uint32_t nonce_length;
};

/**
 * \brief Structure used to pack non-pointer types in a call to PSA Crypto APIs
 *
 */
struct tfm_crypto_pack_iovec {
    psa_key_id_t key_id;     /*!< Key id */
    psa_algorithm_t alg;     /*!< Algorithm */
    uint32_t op_handle;      /*!< Client context handle associated to a
                              *   multipart operation
                              */
    size_t ad_length;        /*!< Additional Data length for multipart AEAD */
    size_t plaintext_length; /*!< Plaintext length for multipart AEAD */

    struct tfm_crypto_aead_pack_input aead_in; /*!< Packs AEAD-related inputs */

    uint16_t function_id;    /*!< Used to identify the function in the
                              *   API dispatcher to the service backend
                              *   See tfm_crypto_func_sid for detail
                              */
    uint16_t step;           /*!< Key derivation step */
    union {
        size_t capacity;     /*!< Key derivation capacity */
        uint64_t value;      /*!< Key derivation integer for update*/
    };
    psa_pake_role_t role;    /*!< PAKE role */
};

/**
 * \brief Type associated to the group of a function encoding. There can be
 *        nine groups (Random, Key management, Hash, MAC, Cipher, AEAD,
 *        Asym sign, Asym encrypt, Key derivation).
 */
enum tfm_crypto_group_id_t {
    TFM_CRYPTO_GROUP_ID_RANDOM          = UINT8_C(1),
    TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT  = UINT8_C(2),
    TFM_CRYPTO_GROUP_ID_HASH            = UINT8_C(3),
    TFM_CRYPTO_GROUP_ID_MAC             = UINT8_C(4),
    TFM_CRYPTO_GROUP_ID_CIPHER          = UINT8_C(5),
    TFM_CRYPTO_GROUP_ID_AEAD            = UINT8_C(6),
    TFM_CRYPTO_GROUP_ID_ASYM_SIGN       = UINT8_C(7),
    TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT    = UINT8_C(8),
    TFM_CRYPTO_GROUP_ID_KEY_DERIVATION  = UINT8_C(9),
    TFM_CRYPTO_GROUP_ID_PAKE            = UINT8_C(10)
};

/* Set of X macros describing each of the available PSA Crypto APIs */
#define RANDOM_FUNCS                               \
    X(TFM_CRYPTO_GENERATE_RANDOM)

#define KEY_MANAGEMENT_FUNCS                       \
    X(TFM_CRYPTO_GET_KEY_ATTRIBUTES)               \
    X(TFM_CRYPTO_OPEN_KEY)                         \
    X(TFM_CRYPTO_CLOSE_KEY)                        \
    X(TFM_CRYPTO_IMPORT_KEY)                       \
    X(TFM_CRYPTO_DESTROY_KEY)                      \
    X(TFM_CRYPTO_EXPORT_KEY)                       \
    X(TFM_CRYPTO_EXPORT_PUBLIC_KEY)                \
    X(TFM_CRYPTO_PURGE_KEY)                        \
    X(TFM_CRYPTO_COPY_KEY)                         \
    X(TFM_CRYPTO_GENERATE_KEY)

#define HASH_FUNCS                                 \
    X(TFM_CRYPTO_HASH_COMPUTE)                     \
    X(TFM_CRYPTO_HASH_COMPARE)                     \
    X(TFM_CRYPTO_HASH_SETUP)                       \
    X(TFM_CRYPTO_HASH_UPDATE)                      \
    X(TFM_CRYPTO_HASH_CLONE)                       \
    X(TFM_CRYPTO_HASH_FINISH)                      \
    X(TFM_CRYPTO_HASH_VERIFY)                      \
    X(TFM_CRYPTO_HASH_ABORT)

#define MAC_FUNCS                                  \
    X(TFM_CRYPTO_MAC_COMPUTE)                      \
    X(TFM_CRYPTO_MAC_VERIFY)                       \
    X(TFM_CRYPTO_MAC_SIGN_SETUP)                   \
    X(TFM_CRYPTO_MAC_VERIFY_SETUP)                 \
    X(TFM_CRYPTO_MAC_UPDATE)                       \
    X(TFM_CRYPTO_MAC_SIGN_FINISH)                  \
    X(TFM_CRYPTO_MAC_VERIFY_FINISH)                \
    X(TFM_CRYPTO_MAC_ABORT)

#define CIPHER_FUNCS                               \
    X(TFM_CRYPTO_CIPHER_ENCRYPT)                   \
    X(TFM_CRYPTO_CIPHER_DECRYPT)                   \
    X(TFM_CRYPTO_CIPHER_ENCRYPT_SETUP)             \
    X(TFM_CRYPTO_CIPHER_DECRYPT_SETUP)             \
    X(TFM_CRYPTO_CIPHER_GENERATE_IV)               \
    X(TFM_CRYPTO_CIPHER_SET_IV)                    \
    X(TFM_CRYPTO_CIPHER_UPDATE)                    \
    X(TFM_CRYPTO_CIPHER_FINISH)                    \
    X(TFM_CRYPTO_CIPHER_ABORT)

#define AEAD_FUNCS                                 \
    X(TFM_CRYPTO_AEAD_ENCRYPT)                     \
    X(TFM_CRYPTO_AEAD_DECRYPT)                     \
    X(TFM_CRYPTO_AEAD_ENCRYPT_SETUP)               \
    X(TFM_CRYPTO_AEAD_DECRYPT_SETUP)               \
    X(TFM_CRYPTO_AEAD_GENERATE_NONCE)              \
    X(TFM_CRYPTO_AEAD_SET_NONCE)                   \
    X(TFM_CRYPTO_AEAD_SET_LENGTHS)                 \
    X(TFM_CRYPTO_AEAD_UPDATE_AD)                   \
    X(TFM_CRYPTO_AEAD_UPDATE)                      \
    X(TFM_CRYPTO_AEAD_FINISH)                      \
    X(TFM_CRYPTO_AEAD_VERIFY)                      \
    X(TFM_CRYPTO_AEAD_ABORT)

#define ASYM_SIGN_FUNCS                            \
    X(TFM_CRYPTO_ASYMMETRIC_SIGN_MESSAGE)          \
    X(TFM_CRYPTO_ASYMMETRIC_VERIFY_MESSAGE)        \
    X(TFM_CRYPTO_ASYMMETRIC_SIGN_HASH)             \
    X(TFM_CRYPTO_ASYMMETRIC_VERIFY_HASH)

#define ASYM_ENCRYPT_FUNCS                         \
    X(TFM_CRYPTO_ASYMMETRIC_ENCRYPT)               \
    X(TFM_CRYPTO_ASYMMETRIC_DECRYPT)

#define KEY_DERIVATION_FUNCS                       \
    X(TFM_CRYPTO_RAW_KEY_AGREEMENT)                \
    X(TFM_CRYPTO_KEY_DERIVATION_SETUP)             \
    X(TFM_CRYPTO_KEY_DERIVATION_GET_CAPACITY)      \
    X(TFM_CRYPTO_KEY_DERIVATION_SET_CAPACITY)      \
    X(TFM_CRYPTO_KEY_DERIVATION_INPUT_BYTES)       \
    X(TFM_CRYPTO_KEY_DERIVATION_INPUT_KEY)         \
    X(TFM_CRYPTO_KEY_DERIVATION_INPUT_INTEGER)     \
    X(TFM_CRYPTO_KEY_DERIVATION_KEY_AGREEMENT)     \
    X(TFM_CRYPTO_KEY_DERIVATION_OUTPUT_BYTES)      \
    X(TFM_CRYPTO_KEY_DERIVATION_OUTPUT_KEY)        \
    X(TFM_CRYPTO_KEY_DERIVATION_ABORT)

#define BASE__VALUE(x) ((uint16_t)((((uint16_t)(x)) << 8) & 0xFF00))

#define PAKE_FUNCS                                  \
    X(TFM_CRYPTO_PAKE_SETUP)                        \
    X(TFM_CRYPTO_PAKE_SET_ROLE)                     \
    X(TFM_CRYPTO_PAKE_SET_USER)                     \
    X(TFM_CRYPTO_PAKE_SET_PEER)                     \
    X(TFM_CRYPTO_PAKE_SET_CONTEXT)                  \
    X(TFM_CRYPTO_PAKE_OUTPUT)                       \
    X(TFM_CRYPTO_PAKE_INPUT)                        \
    X(TFM_CRYPTO_PAKE_GET_SHARED_KEY)               \
    X(TFM_CRYPTO_PAKE_ABORT)

/**
 * \brief This type defines numerical progressive values identifying a function API
 *        exposed through the interfaces (S or NS). It's used to dispatch the requests
 *        from S/NS to the corresponding API implementation in the Crypto service backend.
 *
 * \note Each function SID is encoded as uint16_t.
 *        +------------+------------+
 *        |  Group ID  |  Func ID   |
 *        +------------+------------+
 *   (MSB)15         8 7          0(LSB)
 *
 */
enum tfm_crypto_func_sid_t {
#define X(FUNCTION_NAME) FUNCTION_NAME ## _SID,
    BASE__RANDOM         = BASE__VALUE(TFM_CRYPTO_GROUP_ID_RANDOM) - 1,
    RANDOM_FUNCS
    BASE__KEY_MANAGEMENT = BASE__VALUE(TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT) - 1,
    KEY_MANAGEMENT_FUNCS
    BASE__HASH           = BASE__VALUE(TFM_CRYPTO_GROUP_ID_HASH) - 1,
    HASH_FUNCS
    BASE__MAC            = BASE__VALUE(TFM_CRYPTO_GROUP_ID_MAC) - 1,
    MAC_FUNCS
    BASE__CIPHER         = BASE__VALUE(TFM_CRYPTO_GROUP_ID_CIPHER) - 1,
    CIPHER_FUNCS
    BASE__AEAD           = BASE__VALUE(TFM_CRYPTO_GROUP_ID_AEAD) - 1,
    AEAD_FUNCS
    BASE__ASYM_SIGN      = BASE__VALUE(TFM_CRYPTO_GROUP_ID_ASYM_SIGN) - 1,
    ASYM_SIGN_FUNCS
    BASE__ASYM_ENCRYPT   = BASE__VALUE(TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT) - 1,
    ASYM_ENCRYPT_FUNCS
    BASE__KEY_DERIVATION = BASE__VALUE(TFM_CRYPTO_GROUP_ID_KEY_DERIVATION) - 1,
    KEY_DERIVATION_FUNCS
    BASE__PAKE           = BASE__VALUE(TFM_CRYPTO_GROUP_ID_PAKE) - 1,
    PAKE_FUNCS
#undef X
};

/**
 * \brief This macro is used to extract the group_id from an encoded function id
 *        by accessing the upper 8 bits. A \a _function_id is uint16_t type
 */
#define TFM_CRYPTO_GET_GROUP_ID(_function_id) \
    ((enum tfm_crypto_group_id_t)(((uint16_t)(_function_id) >> 8) & 0xFF))

#ifdef __cplusplus
}
#endif

#endif /* __TFM_CRYPTO_DEFS_H__ */
