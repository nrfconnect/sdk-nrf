/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The registry, written out rather than assembled by the linker so that this
 * file reads as what the sample covers. One array per src/suites folder, whose
 * name appears once in suite_groups[] and is what results are grouped by.
 */

#include "crypto_benchmarks.h"
#include "suites/suites.h"

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_CIPHER)
static const struct suite *const group_psa_cipher[] = {
	&suite_aes_cbc_128,
	&suite_aes_cbc_192,
	&suite_aes_cbc_256,
	&suite_aes_cbc_pkcs7_128,
	&suite_aes_cbc_pkcs7_192,
	&suite_aes_cbc_pkcs7_256,
	&suite_aes_ctr_128,
	&suite_aes_ctr_192,
	&suite_aes_ctr_256,
	&suite_aes_ecb_128,
	&suite_aes_ecb_192,
	&suite_aes_ecb_256,
	&suite_chacha20,
	&suite_persistent,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_HASH)
static const struct suite *const group_psa_hash[] = {
	&suite_sha1,
	&suite_sha224,
	&suite_sha256,
	&suite_sha384,
	&suite_sha512,
	&suite_sha3_224,
	&suite_sha3_256,
	&suite_sha3_384,
	&suite_sha3_512,
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_XOF)
	/* Defined with the hashes, but needs the XOF option's SHAKE support. */
	&suite_shake256_512,
#endif
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_XOF)
static const struct suite *const group_psa_xof[] = {
	&suite_shake128,
	&suite_shake256,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_RNG)
static const struct suite *const group_psa_rng[] = {
	&suite_rng,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_AEAD)
static const struct suite *const group_psa_aead[] = {
	&suite_aes_ccm_128,
	&suite_aes_ccm_192,
	&suite_aes_ccm_256,
	&suite_aes_gcm_128,
	&suite_aes_gcm_192,
	&suite_aes_gcm_256,
	&suite_chachapoly,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_MAC)
static const struct suite *const group_psa_mac[] = {
	&suite_hmac_sha1,
	&suite_hmac_sha224,
	&suite_hmac,
	&suite_hmac_sha384,
	&suite_hmac_sha512,
	&suite_cmac,
	&suite_cmac_aes192,
	&suite_cmac_aes256,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_DERIVATION)
static const struct suite *const group_psa_key_derivation[] = {
	&suite_hkdf,
	&suite_pbkdf2,
	&suite_tls12_prf,
	&suite_tls12_psk_to_ms,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_WRAP)
static const struct suite *const group_psa_key_wrap[] = {
	&suite_aes_kw_128,
	&suite_aes_kw_192,
	&suite_aes_kw_256,
	&suite_aes_kwp_128,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_AGREEMENT)
static const struct suite *const group_psa_key_agreement[] = {
	&suite_ecdh_brainpoolp256r1,
	&suite_ecdh_brainpoolp384r1,
	&suite_ecdh_brainpoolp512r1,
	&suite_ecdh_secp224r1,
	&suite_ecdh,
	&suite_ecdh_secp384r1,
	&suite_ecdh_secp521r1,
	&suite_ecdh_x25519,
	&suite_ecdh_x448,
	&suite_ecdh_secp256k1,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_SIGNATURE)
static const struct suite *const group_psa_signature[] = {
	&suite_ecdsa_brainpoolp256r1_sha1,
	&suite_ecdsa_brainpoolp256r1_sha224,
	&suite_ecdsa_brainpoolp256r1_sha256,
	&suite_ecdsa_brainpoolp256r1_sha384,
	&suite_ecdsa_brainpoolp256r1_sha512,
	&suite_ecdsa_brainpoolp384r1_sha1,
	&suite_ecdsa_brainpoolp384r1_sha224,
	&suite_ecdsa_brainpoolp384r1_sha256,
	&suite_ecdsa_brainpoolp384r1_sha384,
	&suite_ecdsa_brainpoolp384r1_sha512,
	&suite_ecdsa_brainpoolp512r1_sha1,
	&suite_ecdsa_brainpoolp512r1_sha224,
	&suite_ecdsa_brainpoolp512r1_sha256,
	&suite_ecdsa_brainpoolp512r1_sha384,
	&suite_ecdsa_brainpoolp512r1_sha512,
	&suite_ecdsa_secp224r1_sha1,
	&suite_ecdsa_secp224r1_sha224,
	&suite_ecdsa_secp224r1_sha256,
	&suite_ecdsa_secp224r1_sha384,
	&suite_ecdsa_secp224r1_sha512,
	&suite_ecdsa,
	&suite_ecdsa_secp256r1_sha1,
	&suite_ecdsa_secp256r1_sha224,
	&suite_ecdsa_secp256r1_sha384,
	&suite_ecdsa_secp256r1_sha512,
	&suite_ecdsa_secp384r1_sha1,
	&suite_ecdsa_secp384r1_sha224,
	&suite_ecdsa_secp384r1_sha256,
	&suite_ecdsa_secp384r1_sha384,
	&suite_ecdsa_secp384r1_sha512,
	&suite_ecdsa_secp521r1_sha1,
	&suite_ecdsa_secp521r1_sha224,
	&suite_ecdsa_secp521r1_sha256,
	&suite_ecdsa_secp521r1_sha384,
	&suite_ecdsa_secp521r1_sha512,
	&suite_ecdsa_secp256k1_sha1,
	&suite_ecdsa_secp256k1_sha224,
	&suite_ecdsa_secp256k1_sha256,
	&suite_ecdsa_secp256k1_sha384,
	&suite_ecdsa_secp256k1_sha512,
	&suite_eddsa,
	&suite_rsa,
	&suite_rsa_pss,
	&suite_ml_dsa_65,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_ASYMMETRIC_ENCRYPTION)
static const struct suite *const group_psa_asymmetric_encryption[] = {
	&suite_rsa_oaep,
	&suite_rsa_pkcs1v15_crypt,
};
#endif

#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_PAKE)
static const struct suite *const group_psa_pake[] = {
	&suite_ecjpake,
	&suite_spake2p,
	&suite_srp6,
};
#endif

/* One token names both the array and the group, so they cannot drift apart. The
 * group_ prefix avoids the PSA namespace: psa_key_agreement() is a PSA function.
 */
#define SUITE_GROUP(_folder) {#_folder, group_##_folder, ARRAY_SIZE(group_##_folder)}

const struct suite_group suite_groups[] = {
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_CIPHER)
	SUITE_GROUP(psa_cipher),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_HASH)
	SUITE_GROUP(psa_hash),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_XOF)
	SUITE_GROUP(psa_xof),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_RNG)
	SUITE_GROUP(psa_rng),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_AEAD)
	SUITE_GROUP(psa_aead),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_MAC)
	SUITE_GROUP(psa_mac),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_DERIVATION)
	SUITE_GROUP(psa_key_derivation),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_WRAP)
	SUITE_GROUP(psa_key_wrap),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_AGREEMENT)
	SUITE_GROUP(psa_key_agreement),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_SIGNATURE)
	SUITE_GROUP(psa_signature),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_ASYMMETRIC_ENCRYPTION)
	SUITE_GROUP(psa_asymmetric_encryption),
#endif
#if defined(CONFIG_CRYPTO_BENCHMARKS_PSA_PAKE)
	SUITE_GROUP(psa_pake),
#endif
};

const size_t suite_group_cnt = ARRAY_SIZE(suite_groups);
