/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(PSA_CRYPTO_DRIVER_HAS_CIPHER_SUPPORT_OBERON)
#define PSA_NEED_OBERON_CIPHER_DRIVER                          1
#define PSA_NEED_OBERON_CHACHA20                               1
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_AEAD_SUPPORT_OBERON)
#define PSA_NEED_OBERON_AEAD_DRIVER                            1
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_HASH_SUPPORT_OBERON)
#define PSA_NEED_OBERON_HASH_DRIVER                            1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CBC_NO_PADDING_OBERON)
#define PSA_NEED_OBERON_AES_CBC_NO_PADDING                     1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CBC_PKCS7_OBERON)
#define PSA_NEED_OBERON_AES_CBC_PKCS7                          1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CCM_OBERON)
#define PSA_NEED_OBERON_AES_CCM                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CHACHA20_POLY1305_OBERON)
#define PSA_NEED_OBERON_CHACHA20_POLY1305                      1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CMAC_OBERON)
#define PSA_NEED_OBERON_CMAC                                   1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CTR_OBERON)
#define PSA_NEED_OBERON_AES_CTR                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_DETERMINISTIC_ECDSA_OBERON)
#define PSA_NEED_OBERON_ECDSA_DRIVER                           1
#define PSA_NEED_OBERON_ECDSA_P224                             1
#define PSA_NEED_OBERON_ECDSA_P256                             1
#define PSA_NEED_OBERON_ECDSA_P384                             1
#define PSA_NEED_OBERON_ECDSA_ED25519                          1
#define PSA_NEED_OBERON_DETERMINISTIC_ECDSA                    1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_ECB_NO_PADDING_OBERON)
#define PSA_NEED_OBERON_AES_ECB_NO_PADDING                     1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_ECDH_OBERON)
#define PSA_NEED_OBERON_ECDH_DRIVER                            1
#define PSA_NEED_OBERON_ECDH_P224                              1
#define PSA_NEED_OBERON_ECDH_P256                              1
#define PSA_NEED_OBERON_ECDH_P384                              1
#define PSA_NEED_OBERON_ECDH_X25519                            1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_ECDSA_OBERON)
#define PSA_NEED_OBERON_ECDSA_DRIVER                           1
#define PSA_NEED_OBERON_ECDSA_P224                             1
#define PSA_NEED_OBERON_ECDSA_P256                             1
#define PSA_NEED_OBERON_ECDSA_P384                             1
#define PSA_NEED_OBERON_ECDSA_ED25519                          1
#define PSA_NEED_OBERON_RANDOMIZED_ECDSA                       1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_GCM_OBERON)
#define PSA_NEED_OBERON_AES_GCM                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_KDF_SUPPORT_OBERON)
#define PSA_NEED_OBERON_KDF_DRIVER                             1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_HKDF_OBERON)
#define PSA_NEED_OBERON_HKDF                                   1
#define PSA_NEED_OBERON_HKDF_EXTRACT                           1
#define PSA_NEED_OBERON_HKDF_EXPAND                            1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_PBKDF2_HMAC_OBERON)
#define PSA_NEED_OBERON_PBKDF2_HMAC                            1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_PBKDF2_AES_CMAC_PRF_128_OBERON)
#define PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_HMAC_OBERON)
#define PSA_NEED_OBERON_HMAC                                   1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SHA_1_OBERON)
#define PSA_NEED_OBERON_SHA_1                                  1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SHA_224_OBERON)
#define PSA_NEED_OBERON_SHA_224                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SHA_256_OBERON)
#define PSA_NEED_OBERON_SHA_256                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SHA_384_OBERON)
#define PSA_NEED_OBERON_SHA_384                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SHA_512_OBERON)
#define PSA_NEED_OBERON_SHA_512                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ECC_SECP_R1_224_OBERON)
#define PSA_NEED_OBERON_KEY_PAIR_DRIVER                        1
#define PSA_NEED_OBERON_KEY_PAIR_P224                          1
#define PSA_NEED_OBERON_KEY_PAIR_SECP                          1
#endif

#if defined(PSA_CRYPTO_DRIVER_ECC_SECP_R1_256_OBERON)
#define PSA_NEED_OBERON_KEY_PAIR_DRIVER                        1
#define PSA_NEED_OBERON_KEY_PAIR_P256                          1
#define PSA_NEED_OBERON_KEY_PAIR_SECP                          1
#endif

#if defined(PSA_CRYPTO_DRIVER_ECC_SECP_R1_384_OBERON)
#define PSA_NEED_OBERON_KEY_PAIR_DRIVER                        1
#define PSA_NEED_OBERON_KEY_PAIR_P384                          1
#define PSA_NEED_OBERON_KEY_PAIR_SECP                          1
#endif

#if defined(PSA_CRYPTO_DRIVER_ECC_MONTGOMERY_255_OBERON)
#define PSA_NEED_OBERON_KEY_PAIR_DRIVER                        1
#define PSA_NEED_OBERON_KEY_PAIR_X25519                        1
#define PSA_NEED_OBERON_KEY_PAIR_25519                         1
#endif

#if defined(PSA_CRYPTO_DRIVER_ECC_TWISTED_EDWARDS_255_OBERON)
#define PSA_NEED_OBERON_KEY_PAIR_DRIVER                        1
#define PSA_NEED_OBERON_KEY_PAIR_ED25519                       1
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_RSA_SUPPORT_OBERON)
#define PSA_NEED_OBERON_RSA_DRIVER                             1

#if defined(PSA_CRYPTO_DRIVER_RSA_KEY_SIZE_1024_OBERON)
/* Only enable RSA key size 1024 for testing */
#define PSA_NEED_OBERON_RSA_KEY_SIZE_1024                      1
#endif

#define PSA_NEED_OBERON_RSA_KEY_SIZE_1536                      1
#define PSA_NEED_OBERON_RSA_KEY_SIZE_2048                      1
#define PSA_NEED_OBERON_RSA_KEY_SIZE_3072                      1
/* Increasing this value has consequences on callers stack usage. */
#define PSA_MAX_RSA_KEY_BITS 3072
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_RSA_CRYPT_SUPPORT_OBERON)
#define PSA_NEED_OBERON_RSA_CRYPT                              1
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_RSA_SIGN_SUPPORT_OBERON)
#define PSA_NEED_OBERON_RSA_SIGN                               1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_CRYPT_OBERON)
#define PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT                     1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_SIGN_OBERON)
#define PSA_NEED_OBERON_RSA_PKCS1V15_SIGN                      1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_RSA_OAEP_OBERON)
#define PSA_NEED_OBERON_RSA_OAEP                               1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_RSA_PSS_OBERON)
#define PSA_NEED_OBERON_RSA_PSS                                1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_TLS12_PRF_OBERON)
#define PSA_NEED_OBERON_KDF_DRIVER                             1
#define PSA_NEED_OBERON_TLS12_PRF                              1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_TLS12_PSK_TO_MS_OBERON)
#define PSA_NEED_OBERON_KDF_DRIVER                             1
#define PSA_NEED_OBERON_TLS12_PSK_TO_MS                        1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_JPAKE_OBERON)
#define PSA_NEED_OBERON_JPAKE_DRIVER                           1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SPAKE2P_OBERON)
#define PSA_NEED_OBERON_SPAKE2P_DRIVER                         1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SRP_OBERON)
#define PSA_NEED_OBERON_SRP_DRIVER                             1
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_TLS12_ECJPAKE_TO_PMS_OBERON)
#define PSA_NEED_OBERON_KDF_DRIVER                             1
#define PSA_NEED_OBERON_ECJPAKE_TO_PMS                         1
#endif
