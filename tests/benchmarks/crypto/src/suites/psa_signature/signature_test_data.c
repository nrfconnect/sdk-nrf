/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "signature_test_logic.h"

const struct suite suite_ecdsa_brainpoolp256r1_sha1 = ECDSA_SUITE_ENTRY(
	"brainpoolp256r1-sha1", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_brainpoolp256r1_sha224 = ECDSA_SUITE_ENTRY(
	"brainpoolp256r1-sha224", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_brainpoolp256r1_sha256 = ECDSA_SUITE_ENTRY(
	"brainpoolp256r1-sha256", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_brainpoolp256r1_sha384 = ECDSA_SUITE_ENTRY(
	"brainpoolp256r1-sha384", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_brainpoolp256r1_sha512 = ECDSA_SUITE_ENTRY(
	"brainpoolp256r1-sha512", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_brainpoolp384r1_sha1 = ECDSA_SUITE_ENTRY(
	"brainpoolp384r1-sha1", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 384, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_brainpoolp384r1_sha224 = ECDSA_SUITE_ENTRY(
	"brainpoolp384r1-sha224", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 384, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_brainpoolp384r1_sha256 = ECDSA_SUITE_ENTRY(
	"brainpoolp384r1-sha256", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 384, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_brainpoolp384r1_sha384 = ECDSA_SUITE_ENTRY(
	"brainpoolp384r1-sha384", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 384, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_brainpoolp384r1_sha512 = ECDSA_SUITE_ENTRY(
	"brainpoolp384r1-sha512", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 384, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_brainpoolp512r1_sha1 = ECDSA_SUITE_ENTRY(
	"brainpoolp512r1-sha1", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 512, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_brainpoolp512r1_sha224 = ECDSA_SUITE_ENTRY(
	"brainpoolp512r1-sha224", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 512, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_brainpoolp512r1_sha256 = ECDSA_SUITE_ENTRY(
	"brainpoolp512r1-sha256", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 512, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_brainpoolp512r1_sha384 = ECDSA_SUITE_ENTRY(
	"brainpoolp512r1-sha384", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 512, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_brainpoolp512r1_sha512 = ECDSA_SUITE_ENTRY(
	"brainpoolp512r1-sha512", PSA_ECC_FAMILY_BRAINPOOL_P_R1, 512, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_secp224r1_sha1 = ECDSA_SUITE_ENTRY(
	"secp224r1-sha1", PSA_ECC_FAMILY_SECP_R1, 224, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_secp224r1_sha224 = ECDSA_SUITE_ENTRY(
	"secp224r1-sha224", PSA_ECC_FAMILY_SECP_R1, 224, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_secp224r1_sha256 = ECDSA_SUITE_ENTRY(
	"secp224r1-sha256", PSA_ECC_FAMILY_SECP_R1, 224, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_secp224r1_sha384 = ECDSA_SUITE_ENTRY(
	"secp224r1-sha384", PSA_ECC_FAMILY_SECP_R1, 224, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_secp224r1_sha512 = ECDSA_SUITE_ENTRY(
	"secp224r1-sha512", PSA_ECC_FAMILY_SECP_R1, 224, PSA_ALG_SHA_512);

const struct suite suite_ecdsa = ECDSA_SUITE_ENTRY(
	"secp256r1-sha256", PSA_ECC_FAMILY_SECP_R1, 256, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_secp256r1_sha1 = ECDSA_SUITE_ENTRY(
	"secp256r1-sha1", PSA_ECC_FAMILY_SECP_R1, 256, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_secp256r1_sha224 = ECDSA_SUITE_ENTRY(
	"secp256r1-sha224", PSA_ECC_FAMILY_SECP_R1, 256, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_secp256r1_sha384 = ECDSA_SUITE_ENTRY(
	"secp256r1-sha384", PSA_ECC_FAMILY_SECP_R1, 256, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_secp256r1_sha512 = ECDSA_SUITE_ENTRY(
	"secp256r1-sha512", PSA_ECC_FAMILY_SECP_R1, 256, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_secp384r1_sha1 = ECDSA_SUITE_ENTRY(
	"secp384r1-sha1", PSA_ECC_FAMILY_SECP_R1, 384, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_secp384r1_sha224 = ECDSA_SUITE_ENTRY(
	"secp384r1-sha224", PSA_ECC_FAMILY_SECP_R1, 384, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_secp384r1_sha256 = ECDSA_SUITE_ENTRY(
	"secp384r1-sha256", PSA_ECC_FAMILY_SECP_R1, 384, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_secp384r1_sha384 = ECDSA_SUITE_ENTRY(
	"secp384r1-sha384", PSA_ECC_FAMILY_SECP_R1, 384, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_secp384r1_sha512 = ECDSA_SUITE_ENTRY(
	"secp384r1-sha512", PSA_ECC_FAMILY_SECP_R1, 384, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_secp521r1_sha1 = ECDSA_SUITE_ENTRY(
	"secp521r1-sha1", PSA_ECC_FAMILY_SECP_R1, 521, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_secp521r1_sha224 = ECDSA_SUITE_ENTRY(
	"secp521r1-sha224", PSA_ECC_FAMILY_SECP_R1, 521, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_secp521r1_sha256 = ECDSA_SUITE_ENTRY(
	"secp521r1-sha256", PSA_ECC_FAMILY_SECP_R1, 521, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_secp521r1_sha384 = ECDSA_SUITE_ENTRY(
	"secp521r1-sha384", PSA_ECC_FAMILY_SECP_R1, 521, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_secp521r1_sha512 = ECDSA_SUITE_ENTRY(
	"secp521r1-sha512", PSA_ECC_FAMILY_SECP_R1, 521, PSA_ALG_SHA_512);

const struct suite suite_ecdsa_secp256k1_sha1 = ECDSA_SUITE_ENTRY(
	"secp256k1-sha1", PSA_ECC_FAMILY_SECP_K1, 256, PSA_ALG_SHA_1);
const struct suite suite_ecdsa_secp256k1_sha224 = ECDSA_SUITE_ENTRY(
	"secp256k1-sha224", PSA_ECC_FAMILY_SECP_K1, 256, PSA_ALG_SHA_224);
const struct suite suite_ecdsa_secp256k1_sha256 = ECDSA_SUITE_ENTRY(
	"secp256k1-sha256", PSA_ECC_FAMILY_SECP_K1, 256, PSA_ALG_SHA_256);
const struct suite suite_ecdsa_secp256k1_sha384 = ECDSA_SUITE_ENTRY(
	"secp256k1-sha384", PSA_ECC_FAMILY_SECP_K1, 256, PSA_ALG_SHA_384);
const struct suite suite_ecdsa_secp256k1_sha512 = ECDSA_SUITE_ENTRY(
	"secp256k1-sha512", PSA_ECC_FAMILY_SECP_K1, 256, PSA_ALG_SHA_512);

const struct suite suite_eddsa = SIGNATURE_SUITE_ENTRY(
	"eddsa", "ed25519", PSA_ALG_PURE_EDDSA,
	PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS), 255, 0);

const struct suite suite_rsa = SIGNATURE_SUITE_ENTRY(
	"rsa", "rsa4096", PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256), PSA_KEY_TYPE_RSA_KEY_PAIR,
	4096, PSA_ALG_SHA_256);

const struct suite suite_rsa_pss = RSA_PSS_SUITE_ENTRY("rsa2048", PSA_ALG_SHA_256);

/* Verify only, over a known-answer vector: see ml_dsa_import_key(). */
const struct suite suite_ml_dsa_65 = ML_DSA_VERIFY_SUITE_ENTRY("ml_dsa_65");
