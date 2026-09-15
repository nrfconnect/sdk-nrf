/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUITES_H__
#define SUITES_H__

#include "crypto_benchmarks.h"

extern const struct suite suite_aes_cbc_128;
extern const struct suite suite_aes_cbc_192;
extern const struct suite suite_aes_ctr_256;
extern const struct suite suite_aes_cbc_256;
extern const struct suite suite_aes_cbc_pkcs7_128;
extern const struct suite suite_aes_cbc_pkcs7_192;
extern const struct suite suite_aes_cbc_pkcs7_256;
extern const struct suite suite_aes_ecb_128;
extern const struct suite suite_aes_ecb_192;
extern const struct suite suite_aes_ecb_256;
extern const struct suite suite_chacha20;
extern const struct suite suite_sha1;
extern const struct suite suite_sha224;
extern const struct suite suite_sha256;
extern const struct suite suite_sha384;
extern const struct suite suite_sha512;
extern const struct suite suite_sha3_224;
extern const struct suite suite_sha3_256;
extern const struct suite suite_sha3_384;
extern const struct suite suite_sha3_512;
extern const struct suite suite_shake256_512;
extern const struct suite suite_shake128;
extern const struct suite suite_shake256;
extern const struct suite suite_rng;
extern const struct suite suite_aes_ctr_128;
extern const struct suite suite_aes_gcm_192;
extern const struct suite suite_aes_gcm_256;
extern const struct suite suite_aes_ctr_192;
extern const struct suite suite_aes_ccm_128;
extern const struct suite suite_aes_ccm_192;
extern const struct suite suite_aes_ccm_256;
extern const struct suite suite_aes_gcm_128;
extern const struct suite suite_aes_gcm_192;
extern const struct suite suite_aes_gcm_256;
extern const struct suite suite_chachapoly;
extern const struct suite suite_aes_gcm_128;
extern const struct suite suite_aes_gcm_192;
extern const struct suite suite_chachapoly;
extern const struct suite suite_hmac;
extern const struct suite suite_hmac_sha1;
extern const struct suite suite_hmac_sha224;
extern const struct suite suite_hmac_sha384;
extern const struct suite suite_hmac_sha512;
extern const struct suite suite_cmac;
extern const struct suite suite_cmac_aes192;
extern const struct suite suite_cmac_aes256;
extern const struct suite suite_hkdf;
extern const struct suite suite_pbkdf2;
extern const struct suite suite_tls12_prf;
extern const struct suite suite_tls12_psk_to_ms;
extern const struct suite suite_aes_kw_128;
extern const struct suite suite_aes_kw_192;
extern const struct suite suite_aes_kw_256;
extern const struct suite suite_aes_kwp_128;
extern const struct suite suite_ecdh_brainpoolp256r1;
extern const struct suite suite_ecdh_brainpoolp384r1;
extern const struct suite suite_ecdh_brainpoolp512r1;
extern const struct suite suite_ecdh_secp224r1;
extern const struct suite suite_ecdh;
extern const struct suite suite_ecdh_secp384r1;
extern const struct suite suite_ecdh_secp521r1;
extern const struct suite suite_ecdh_x25519;
extern const struct suite suite_ecdh_x448;
extern const struct suite suite_ecdh_secp256k1;

extern const struct suite suite_ecdsa_brainpoolp256r1_sha1;
extern const struct suite suite_ecdsa_brainpoolp256r1_sha224;
extern const struct suite suite_ecdsa_brainpoolp256r1_sha256;
extern const struct suite suite_ecdsa_brainpoolp256r1_sha384;
extern const struct suite suite_ecdsa_brainpoolp256r1_sha512;
extern const struct suite suite_ecdsa_brainpoolp384r1_sha1;
extern const struct suite suite_ecdsa_brainpoolp384r1_sha224;
extern const struct suite suite_ecdsa_brainpoolp384r1_sha256;
extern const struct suite suite_ecdsa_brainpoolp384r1_sha384;
extern const struct suite suite_ecdsa_brainpoolp384r1_sha512;
extern const struct suite suite_ecdsa_brainpoolp512r1_sha1;
extern const struct suite suite_ecdsa_brainpoolp512r1_sha224;
extern const struct suite suite_ecdsa_brainpoolp512r1_sha256;
extern const struct suite suite_ecdsa_brainpoolp512r1_sha384;
extern const struct suite suite_ecdsa_brainpoolp512r1_sha512;
extern const struct suite suite_ecdsa_secp224r1_sha1;
extern const struct suite suite_ecdsa_secp224r1_sha224;
extern const struct suite suite_ecdsa_secp224r1_sha256;
extern const struct suite suite_ecdsa_secp224r1_sha384;
extern const struct suite suite_ecdsa_secp224r1_sha512;
extern const struct suite suite_ecdsa;
extern const struct suite suite_ecdsa_secp256r1_sha1;
extern const struct suite suite_ecdsa_secp256r1_sha224;
extern const struct suite suite_ecdsa_secp256r1_sha256;
extern const struct suite suite_ecdsa_secp256r1_sha384;
extern const struct suite suite_ecdsa_secp256r1_sha512;
extern const struct suite suite_ecdsa_secp384r1_sha1;
extern const struct suite suite_ecdsa_secp384r1_sha224;
extern const struct suite suite_ecdsa_secp384r1_sha256;
extern const struct suite suite_ecdsa_secp384r1_sha384;
extern const struct suite suite_ecdsa_secp384r1_sha512;
extern const struct suite suite_ecdsa_secp521r1_sha1;
extern const struct suite suite_ecdsa_secp521r1_sha224;
extern const struct suite suite_ecdsa_secp521r1_sha256;
extern const struct suite suite_ecdsa_secp521r1_sha384;
extern const struct suite suite_ecdsa_secp521r1_sha512;
extern const struct suite suite_ecdsa_secp256k1_sha1;
extern const struct suite suite_ecdsa_secp256k1_sha224;
extern const struct suite suite_ecdsa_secp256k1_sha256;
extern const struct suite suite_ecdsa_secp256k1_sha384;
extern const struct suite suite_ecdsa_secp256k1_sha512;
extern const struct suite suite_eddsa;
extern const struct suite suite_rsa;
extern const struct suite suite_rsa_pss;
extern const struct suite suite_ml_dsa_65;
extern const struct suite suite_rsa_oaep;
extern const struct suite suite_rsa_pkcs1v15_crypt;
extern const struct suite suite_ecjpake;
extern const struct suite suite_spake2p;
extern const struct suite suite_srp6;
extern const struct suite suite_persistent;
#endif /* SUITES_H__ */
