/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <cracen_psa.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>
#include <cracen/mem_helpers.h>
#include <string.h>
#include <util/util_macro.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* Ed25519 public key to check usage and ability to revoke */
#define KMU_KEY_ID_PUBKEY_ED25519_REVOKABLE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 4)

/* Ed25519 public key to check usage and ability to lock */
#define KMU_KEY_ID_PUBKEY_ED25519_READ_ONLY \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 6)

/* Ed25519 public key to check usage and ability to revoke */
#define KMU_KEY_ID_PUBKEY_ED25519PH_REVOKABLE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 8)

/* Ed25519 public key to check usage and ability to lock */
#define KMU_KEY_ID_PUBKEY_ED25519PH_READ_ONLY \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 10)

/* ECDSA secp256r1 public key to check usage and ability to revoke */
#define KMU_KEY_ID_PUBKEY_SECP256R1_REVOKABLE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 12)

/* ECDSA secp256r1 public key to check usage and ability to lock */
#define KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 16)

/* ECDSA secp384r1 public key to check usage and ability to revoke */
#define KMU_KEY_ID_PUBKEY_SECP384R1_REVOKABLE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 20)

/* ECDSA secp384r1 public key to check usage and ability to lock */
#define KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 26)

/* AES-256 key used to check usage and ability to revoke */
#define KMU_KEY_ID_AES_256_KEY_REVOKABLE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, 32)

/* AES-256 key used to check usage and ability to lock  */
#define KMU_KEY_ID_AES_256_KEY_READ_ONLY \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, 34)

#define KMU_GET_SLOT_ID(key_id) \
	CRACEN_PSA_GET_KMU_SLOT(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id))

/* Largest exportable size for KMU key (either buffer or kmu_opaque_key_buffer) */
#define KMU_MAX_EXPORTED_SIZE	(65)

/* Test material for Ed25519 (RFC8032, 7.1 Ed25519 TEST 3)*/
#define ED25519_PUBKEY_SIZE	(32)
#define ED25519_SIGNATURE_SIZE	(64)
#define SHA512_HASH_SIZE	(64)

static uint8_t ed25519_pubkey[ED25519_PUBKEY_SIZE] = {
	0xfc, 0x51, 0xcd, 0x8e, 0x62, 0x18, 0xa1, 0xa3,
	0x8d, 0xa4, 0x7e, 0xd0, 0x02, 0x30, 0xf0, 0x58,
	0x08, 0x16, 0xed, 0x13, 0xba, 0x33, 0x03, 0xac,
	0x5d, 0xeb, 0x91, 0x15, 0x48, 0x90, 0x80, 0x25
};

/* Ed25519 message (2 octets)*/
static uint8_t ed25519_msg[2] = {
	0xaf, 0x82
};

static uint8_t ed25519_signature[ED25519_SIGNATURE_SIZE] = {
	0x62, 0x91, 0xd6, 0x57, 0xde, 0xec, 0x24, 0x02, 0x48, 0x27, 0xe6, 0x9c,
	0x3a, 0xbe, 0x01, 0xa3, 0x0c, 0xe5, 0x48, 0xa2, 0x84, 0x74, 0x3a, 0x44,
	0x5e, 0x36, 0x80, 0xd7, 0xdb, 0x5a, 0xc3, 0xac, 0x18, 0xff, 0x9b, 0x53,
	0x8d, 0x16, 0xf2, 0x90, 0xae, 0x67, 0xf7, 0x60, 0x98, 0x4d, 0xc6, 0x59,
	0x4a, 0x7c, 0x15, 0xe9, 0x71, 0x6e, 0xd2, 0x8d, 0xc0, 0x27, 0xbe, 0xce,
	0xea, 0x1e, 0xc4, 0x0a
};

/* Test material for Ed25519ph (RFC8032, 7.3 Ed25519ph)*/
static uint8_t ed25519ph_pubkey[ED25519_PUBKEY_SIZE] = {
	0xec, 0x17, 0x2b, 0x93, 0xad, 0x5e, 0x56, 0x3b, 0xf4, 0x93, 0x2c, 0x70,
	0xe1, 0x24, 0x50, 0x34, 0xc3, 0x54, 0x67, 0xef, 0x2e, 0xfd, 0x4d, 0x64,
	0xeb, 0xf8, 0x19, 0x68, 0x34, 0x67, 0xe2, 0xbf
};

/* Ed25518ph test message (used only by hash test) */
static uint8_t ed25519ph_msg[3] = {
	0x61, 0x62, 0x63
};

/* SHA-512 Hash of message '0x61, 0x62, 0x63' (hex input, 3 octets)*/
static uint8_t ed25519ph_hash[SHA512_HASH_SIZE] = {
	0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49,
	0xae, 0x20, 0x41, 0x31, 0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
	0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a,
	0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
	0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f,
	0xa5, 0x4c, 0xa4, 0x9f
};

static uint8_t ed25519ph_signature[ED25519_SIGNATURE_SIZE] = {
	0x98, 0xa7, 0x02, 0x22, 0xf0, 0xb8, 0x12, 0x1a, 0xa9, 0xd3, 0x0f, 0x81,
	0x3d, 0x68, 0x3f, 0x80, 0x9e, 0x46, 0x2b, 0x46, 0x9c, 0x7f, 0xf8, 0x76,
	0x39, 0x49, 0x9b, 0xb9, 0x4e, 0x6d, 0xae, 0x41, 0x31, 0xf8, 0x50, 0x42,
	0x46, 0x3c, 0x2a, 0x35, 0x5a, 0x20, 0x03, 0xd0, 0x62, 0xad, 0xf5, 0xaa,
	0xa1, 0x0b, 0x8c, 0x61, 0xe6, 0x36, 0x06, 0x2a, 0xaa, 0xd1, 0x1c, 0x2a,
	0x26, 0x08, 0x34, 0x06
};

/* Test material for ECDSA secp256r1 SHA-256 (RFC6979 A.2.5) */
#define ECDSA_SECP256R1_PUBKEY_SIZE	(65)
#define ECDSA_SECP256R1_SIGNATURE_SIZE	(64)
#define SHA256_HASH_SIZE		(32)

static uint8_t ecdsa_secp256r1_pubkey[ECDSA_SECP256R1_PUBKEY_SIZE] = {
	0x04,
	/* Ux */
	0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61, 0xeb, 0x74,
	0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61, 0xfa, 0x6c,
	0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
	/* Uy*/
	0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9, 0xe9,
	0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e, 0x9f, 0x51,
	0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99
};

/* Test string without null-termination (6 octets)*/
static uint8_t ecdsa_secp256r1_msg[6] = "sample";

/* SHA-256 hash of the string "sample" (6 octets)*/
static uint8_t ecdsa_secp256r1_hash[SHA256_HASH_SIZE] = {
	0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1, 0xe2, 0xad, 0xe1, 0xd6,
	0x94, 0xf4, 0x1f, 0xc7, 0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
	0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
};

/* Signature is the same for hash/message */
static uint8_t ecdsa_secp256r1_signature[ECDSA_SECP256R1_SIGNATURE_SIZE] = {
	/* r */
	0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd, 0x11, 0x40, 0xdd, 0x9c,
	0xd4, 0x5e, 0x81, 0xd6, 0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91,
	0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16,
	/* s */
	0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41, 0xd4, 0x36, 0xc7, 0xa1,
	0xb6, 0xe2, 0x9f, 0x65, 0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
	0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
};

/* Test material for ECDSA secp384r1 SHA-384 (RFC6979 A.2.6) */
#define ECDSA_SECP384R1_PUBKEY_SIZE	(97)
#define ECDSA_SECP384R1_SIGNATURE_SIZE	(96)
#define SHA384_HASH_SIZE		(48)

static uint8_t ecdsa_secp384r1_pubkey[ECDSA_SECP384R1_PUBKEY_SIZE] = {
	0x04,
	/* Ux */
	0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86, 0x18, 0x02,
	0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
	0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64, 0xde, 0xf8, 0xf0, 0xea,
	0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13,

	/* Uy*/
	0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a,
	0xc0, 0xc6, 0x21, 0x89, 0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
	0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c,
	0x3a, 0xe0, 0xd4, 0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
};

/* Test string without null-termination (6 octets)*/
static uint8_t ecdsa_secp384r1_msg[6] = "sample";

/* SHA-384 hash of the string "sample" (6 octets)*/
static uint8_t ecdsa_secp384r1_hash[SHA384_HASH_SIZE] = {
	0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76, 0xae, 0xc4, 0xbe, 0x31,
	0x26, 0x96, 0xef, 0x7b, 0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
	0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12, 0x31, 0x3b, 0xca, 0x4a,
	0x9b, 0x5b, 0x89, 0x0e, 0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
};

/* Signature is the same for hash/message */
static uint8_t ecdsa_secp384r1_signature[ECDSA_SECP384R1_SIGNATURE_SIZE] = {
	/* r */
	0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa, 0xd4, 0x73, 0x6e, 0x56,
	0xc6, 0x91, 0x91, 0x6b, 0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
	0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c, 0x81, 0xa6, 0x48, 0x15,
	0x2e, 0x44, 0xac, 0xf9, 0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46,
	/* s */
	0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce, 0xa1, 0xfe, 0x40, 0xdb,
	0x26, 0x03, 0x13, 0x8f, 0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
	0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f, 0xa3, 0x29, 0xc1, 0x45,
	0x78, 0x6e, 0x67, 0x9e, 0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
};

/* Test material for AES 256-bits using a phony key */
static uint8_t aes_ctr_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

/* Nonce for the CTR operation (12 bytes)*/
static uint8_t aes_ctr_nonce[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x00, 0x00, 0x00, 0x00
};

static uint8_t aes_ctr_plaintext[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f
};

static uint8_t aes_ctr_ciphertext[16] = {
	0xbd, 0xdd, 0x4e, 0xc9, 0xb5, 0x55, 0x60, 0xcb, 0xc9, 0xf7, 0x61, 0x07,
	0xb6, 0x1e, 0x30, 0xb9
};

/* Not yet standard API for key locking */
psa_status_t psa_lock_key(mbedtls_svc_key_id_t key_id);

#if defined(CONFIG_PSA_CORE_LITE)
/** Give thin PSA core support for import/export for testing */

/* Forward declaration of internal API to provision KMU */
psa_status_t cracen_kmu_provision(const psa_key_attributes_t *key_attr, int slot_id,
				  const uint8_t *key_buffer, size_t key_buffer_size);

psa_status_t psa_import_key(const psa_key_attributes_t *attributes,
				   const uint8_t *data, size_t data_length,
				   mbedtls_svc_key_id_t *key)
{

	uint32_t slot_id = CRACEN_PSA_GET_KMU_SLOT(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));

	return cracen_kmu_provision(attributes, slot_id, data, data_length);

	*key = psa_get_key_id(attributes);
}

psa_status_t psa_export_key(mbedtls_svc_key_id_t key,
			   uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t err;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	uint8_t key_buffer[KMU_MAX_EXPORTED_SIZE] = {0};
	size_t key_buffer_size = data_size;
	size_t key_buffer_length;
	psa_drv_slot_number_t slot_number =
		CRACEN_PSA_GET_KMU_SLOT(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key));

	err = cracen_get_builtin_key(slot_number, &attributes,
				     key_buffer, key_buffer_size, &key_buffer_length);
	if (err != PSA_SUCCESS) {
		return err;
	}

	/* Update the key_buffer_size to the held key size (buffer or kmu_opaque_key_buffer) */
	key_buffer_size = key_buffer_length;

	err = cracen_export_key(&attributes, key_buffer, key_buffer_size,
				data, data_size, data_length);
	if (err != PSA_SUCCESS) {
		return err;
	}

	return PSA_SUCCESS;
}
#else
psa_status_t psa_lock_key(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	err = psa_get_key_attributes(key_id, &attributes);
	if (err != PSA_SUCCESS) {
		return err;
	}

	return cracen_kmu_block(&attributes);
}
#endif

static void set_kmu_key_attributes(psa_key_attributes_t *attributes, mbedtls_svc_key_id_t key_id,
			    psa_algorithm_t alg, psa_key_lifetime_t lifetime,
			    psa_key_usage_t usage, psa_key_type_t key_type, size_t key_bits)
{
	psa_set_key_id(attributes, key_id),
	psa_set_key_algorithm(attributes, alg);
	psa_set_key_lifetime(attributes, lifetime);
	psa_set_key_usage_flags(attributes, usage);
	psa_set_key_type(attributes, key_type),
	psa_set_key_bits(attributes, key_bits);
}

static void init_attributes_ed25519_public_key(mbedtls_svc_key_id_t key_id,
					       psa_key_persistence_t persistence,
					       psa_key_attributes_t *attributes)
{
	/* KMU currently doesn't support stating Ed25519ph, using Ed25519 for both */
	psa_algorithm_t alg = PSA_ALG_PURE_EDDSA;
	psa_key_type_t key_type = PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS);
	psa_key_lifetime_t lifetime =
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			persistence, PSA_KEY_LOCATION_CRACEN_KMU);
	psa_key_usage_t usage = PSA_KEY_USAGE_VERIFY_MESSAGE;
	size_t key_bits = 255;

	set_kmu_key_attributes(attributes, key_id, alg, lifetime, usage, key_type, key_bits);
}

static void init_attributes_ecdsa_secp256r1_public_key(mbedtls_svc_key_id_t key_id,
						       psa_key_persistence_t persistence,
						       psa_key_attributes_t *attributes)
{

	/* KMU currently doesn't support stating Deterministic ECDSA, using ECDSA for both */
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	psa_key_type_t key_type = PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1);
	psa_key_lifetime_t lifetime =
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			persistence, PSA_KEY_LOCATION_CRACEN_KMU);
	psa_key_usage_t usage = PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH;
	size_t key_bits = 256;

	set_kmu_key_attributes(attributes, key_id, alg, lifetime, usage, key_type, key_bits);
}

static void init_attributes_ecdsa_secp384r1_public_key(mbedtls_svc_key_id_t key_id,
						       psa_key_persistence_t persistence,
						       psa_key_attributes_t *attributes)
{
	/* KMU currently doesn't support stating Deterministic ECDSA, using ECDSA for both */
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384);
	psa_key_type_t key_type = PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1);
	psa_key_lifetime_t lifetime =
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			persistence, PSA_KEY_LOCATION_CRACEN_KMU);
	psa_key_usage_t usage = PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH;
	size_t key_bits = 384;

	set_kmu_key_attributes(attributes, key_id, alg, lifetime, usage, key_type, key_bits);
}

static void provision_ed25519_public_key(mbedtls_svc_key_id_t key_id,
					 psa_key_persistence_t persistence,
					 uint8_t key_buffer[ED25519_PUBKEY_SIZE])
{
	psa_status_t err;
	uint8_t temp_buffer[ED25519_PUBKEY_SIZE];
	const size_t pubkey_size = ED25519_PUBKEY_SIZE;
	size_t key_length;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	init_attributes_ed25519_public_key(key_id, persistence, &attributes);

	err = psa_import_key(&attributes, key_buffer, pubkey_size, &key_id);
	zassert_equal(err, PSA_SUCCESS, "Failed to import Ed25519 key. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	/* Check that imported key is correct */
	err = psa_export_key(key_id, temp_buffer, pubkey_size, &key_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to export imported Ed25519 key. slot_id %d, err %d",
		KMU_GET_SLOT_ID(key_id), err);

	zassert_equal(key_length, pubkey_size,
		      "Exported Ed25519 key size mismatch. Expected %d, got %d",
		      pubkey_size, key_length);

	if (constant_memcmp(key_buffer, temp_buffer, pubkey_size) != 0) {
		zassert_false(true, "Imported/exported Ed25519 key mismatch");
	}
}

static void provision_ecdsa_secp256r1_public_key(mbedtls_svc_key_id_t key_id,
						 psa_key_persistence_t persistence,
						 uint8_t key_buffer[65])
{
	psa_status_t err;
	uint8_t temp_buffer[ECDSA_SECP256R1_PUBKEY_SIZE];
	const size_t pubkey_size = ECDSA_SECP256R1_PUBKEY_SIZE;
	size_t key_length;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	init_attributes_ecdsa_secp256r1_public_key(key_id, persistence, &attributes);

	err = psa_import_key(&attributes, key_buffer, pubkey_size, &key_id);
	zassert_equal(err, PSA_SUCCESS,
		"Failed to import ECDSA secp256r1 key. slot_id: %d, err: %d",
		KMU_GET_SLOT_ID(key_id), err);

	/* Check that imported key is correct */
	err = psa_export_key(key_id, temp_buffer, pubkey_size, &key_length);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to export ECDSA secp256r1 key. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	zassert_equal(key_length, pubkey_size, "Exported ECDSA key size mismatch.");

	if (constant_memcmp(key_buffer, temp_buffer, pubkey_size) != 0) {
		zassert_false(true, "Imported/exported Ed25519 key mismatch");
	}
}

static void provision_ecdsa_secp384r1_public_key(mbedtls_svc_key_id_t key_id,
						 psa_key_persistence_t persistence,
						 uint8_t key_buffer[ECDSA_SECP384R1_PUBKEY_SIZE])
{
	psa_status_t err;
	uint8_t temp_buffer[ECDSA_SECP384R1_PUBKEY_SIZE];
	const size_t pubkey_size = ECDSA_SECP384R1_PUBKEY_SIZE;
	size_t key_length;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	init_attributes_ecdsa_secp384r1_public_key(key_id, persistence, &attributes);

	err = psa_import_key(&attributes, key_buffer, pubkey_size, &key_id);
	zassert_equal(err, PSA_SUCCESS,
		"Failed to import ECDSA secp384r1 key. slot_id: %d, err: %d",
		KMU_GET_SLOT_ID(key_id), err);

	/* Check that imported key is correct */
	err = psa_export_key(key_id, temp_buffer, pubkey_size, &key_length);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to export ECDSA secp384r1 key. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	zassert_equal(key_length, pubkey_size, "Exported ECDSA key size mismatch.");

	if (constant_memcmp(key_buffer, temp_buffer, pubkey_size) != 0) {
		zassert_false(true, "Imported/exported secp384r1 key mismatch");
	}
}

static void provision_aes_256_ctr_key(mbedtls_svc_key_id_t key_id,
				      psa_key_persistence_t persistence,
				      uint8_t key_buffer[32])
{
	psa_status_t err;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t temp_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_CTR;
	psa_key_type_t key_type = PSA_KEY_TYPE_AES;
	psa_key_usage_t usage = PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT;
	size_t key_bits = 256;
	psa_key_lifetime_t lifetime =
	PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
		persistence, PSA_KEY_LOCATION_CRACEN_KMU);

	set_kmu_key_attributes(&attributes, key_id, alg, lifetime, usage, key_type, key_bits);

	err = psa_import_key(&attributes, key_buffer, 32, &key_id);
	zassert_equal(err, PSA_SUCCESS, "Failed to import AES-256 ctr key. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	/* The key is protected, hence we can only do metadata-comparison here */
	err = psa_get_key_attributes(key_id, &temp_attributes);

	zassert_equal(err, PSA_SUCCESS,
		      "Failed to get AES-256 ctr attributes: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	if (constant_memcmp(&attributes, &temp_attributes, sizeof(psa_key_attributes_t)) != 0) {
		zassert_false(true, "Key attributes mismatch for AES-256 ctr key");
	}
}

static void provision_keys(void)
{
	bool ran_provisioning = false;

	/* Ed25519 public key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		provision_ed25519_public_key(KMU_KEY_ID_PUBKEY_ED25519_REVOKABLE,
					     CRACEN_KEY_PERSISTENCE_REVOKABLE,
					     ed25519_pubkey);

		provision_ed25519_public_key(KMU_KEY_ID_PUBKEY_ED25519_READ_ONLY,
					     CRACEN_KEY_PERSISTENCE_READ_ONLY,
					     ed25519_pubkey);

		ran_provisioning = true;
	}

	/* Ed25519ph public key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		provision_ed25519_public_key(KMU_KEY_ID_PUBKEY_ED25519PH_REVOKABLE,
					     CRACEN_KEY_PERSISTENCE_REVOKABLE,
					     ed25519ph_pubkey);

		provision_ed25519_public_key(KMU_KEY_ID_PUBKEY_ED25519PH_READ_ONLY,
					     CRACEN_KEY_PERSISTENCE_READ_ONLY,
					     ed25519ph_pubkey);

		ran_provisioning = true;
	}

	/* ECDSA using secp256r1 and SHA-256 */
	if (UTIL_CONCAT_AND(
		IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		IS_ENABLED_ALL(PSA_WANT_ALG_SHA_256, PSA_WANT_ECC_SECP_R1_256))) {

		provision_ecdsa_secp256r1_public_key(KMU_KEY_ID_PUBKEY_SECP256R1_REVOKABLE,
						     CRACEN_KEY_PERSISTENCE_REVOKABLE,
						     ecdsa_secp256r1_pubkey);

		provision_ecdsa_secp256r1_public_key(KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY,
						     CRACEN_KEY_PERSISTENCE_READ_ONLY,
						     ecdsa_secp256r1_pubkey);

		ran_provisioning = true;
	}

	/* ECDSA using secp384r1 and SHA-384 */
	if (UTIL_CONCAT_AND(
		IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		IS_ENABLED_ALL(PSA_WANT_ALG_SHA_384, PSA_WANT_ECC_SECP_R1_384))) {

		provision_ecdsa_secp384r1_public_key(KMU_KEY_ID_PUBKEY_SECP384R1_REVOKABLE,
						     CRACEN_KEY_PERSISTENCE_REVOKABLE,
						     ecdsa_secp384r1_pubkey);

		provision_ecdsa_secp384r1_public_key(KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY,
						     CRACEN_KEY_PERSISTENCE_READ_ONLY,
						     ecdsa_secp384r1_pubkey);

		ran_provisioning = true;
	}

	/* AES CTR using 256-bits key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_CTR, PSA_WANT_AES_KEY_SIZE_256)) {
		provision_aes_256_ctr_key(KMU_KEY_ID_AES_256_KEY_REVOKABLE,
					  CRACEN_KEY_PERSISTENCE_REVOKABLE,
					  aes_ctr_key);

		provision_aes_256_ctr_key(KMU_KEY_ID_AES_256_KEY_READ_ONLY,
					  CRACEN_KEY_PERSISTENCE_READ_ONLY,
					  aes_ctr_key);

		ran_provisioning = true;
	}

	zassert_true(ran_provisioning, "Did not run any valid provisioning, check configs!");
}

/**
 * @brief Revoke any revokable key
 *
 * @param key_id Key identifier for key to revoke
 */
static void revoke_key(mbedtls_svc_key_id_t key_id)
{
	/* Revoke the key*/
	psa_status_t err = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Check that the key exists */
	err = psa_get_key_attributes(key_id, &attributes);
	zassert_equal(err, PSA_SUCCESS, "Key not present, can't be revoked. slot_id: %d, err: %d",
		   KMU_GET_SLOT_ID(key_id), err);

	/* Revoke the key by using psa_destroy_key API */
	err = psa_destroy_key(key_id);
	zassert_equal(err, PSA_SUCCESS, "Key can't be revoked. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	/* Check that key is revoked */
	err = psa_get_key_attributes(key_id, &attributes);
	if (err != PSA_ERROR_NOT_PERMITTED) {
		zassert_false(true, "Key not revoked. slot_id: %d. err: %d",
			      KMU_GET_SLOT_ID(key_id), err);
	}
}

/**
 * @brief Lock any lockable key
 *
 * @param key_id Key identifier for key to lock
 */
static void lock_key(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Check that the key exists */
	err = psa_get_key_attributes(key_id, &attributes);
	zassert_equal(err, PSA_SUCCESS, "Key not present, can't be locked. slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_lock_key(key_id);
	zassert_equal(err, PSA_SUCCESS, "Key can't be locked: slot_id: %d. err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test Ed25519 verify (message)
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ed25519_verify(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_PURE_EDDSA;

	err = psa_verify_message(key_id, alg, ed25519_msg, ARRAY_SIZE(ed25519_msg),
				 ed25519_signature, ED25519_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS, "Failed to verify Ed25519 (pure). slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test Ed25519ph verify (message and hash)
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ed25519ph_verify(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_ED25519PH;

	err = psa_verify_hash(key_id, alg, ed25519ph_hash, SHA512_HASH_SIZE,
			      ed25519ph_signature, ED25519_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS, "Failed to verify Ed25519ph. slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test ECDSA verify message
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ecdsa_secp256r1_verify_message(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);

	err = psa_verify_message(key_id, alg, ecdsa_secp256r1_msg, ARRAY_SIZE(ecdsa_secp256r1_msg),
				 ecdsa_secp256r1_signature, ECDSA_SECP256R1_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify ECDSA message with secp256r1. slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test ECDSA verify hash
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ecdsa_secp256r1_verify_hash(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);

	err = psa_verify_hash(key_id, alg, ecdsa_secp256r1_hash, SHA256_HASH_SIZE,
			      ecdsa_secp256r1_signature, ECDSA_SECP256R1_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify ECDSA hash with secp256r1. slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test ECDSA verify message with secp384r1 key
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ecdsa_secp384r1_verify_message(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384);

	err = psa_verify_message(key_id, alg, ecdsa_secp384r1_msg, ARRAY_SIZE(ecdsa_secp384r1_msg),
				 ecdsa_secp384r1_signature, ECDSA_SECP384R1_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify ECDSA message with secp384r1. slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

/**
 * @brief Function to test ECDSA verify hash with secp384r1 key
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_ecdsa_secp384r1_verify_hash(mbedtls_svc_key_id_t key_id)
{
	psa_status_t err;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384);

	err = psa_verify_hash(key_id, alg, ecdsa_secp384r1_hash, SHA384_HASH_SIZE,
			      ecdsa_secp384r1_signature, ECDSA_SECP384R1_SIGNATURE_SIZE);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify ECDSA hash with secp384r1. slot_id: %d, Err: %d",
		      KMU_GET_SLOT_ID(key_id), err);
}

static void test_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      uint8_t *hash, size_t hash_size)
{
	psa_status_t err;
	size_t hash_length;
	uint8_t calculated_hash[SHA512_HASH_SIZE];

	err = psa_hash_compute(alg, input, input_length, calculated_hash, hash_size, &hash_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to hash Alg: %d, Err: %d", alg, err);
	zassert_equal(hash_size, hash_length, "Hash size mismatch. Got %d, expected %d",
		      hash_length, hash_size);

	if (constant_memcmp(calculated_hash, hash, hash_size) != 0) {
		zassert_false(true, "Hash calculate mismatched results");
	}
}

static void test_hash_incremental(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  uint8_t *hash, size_t hash_size)
{
	psa_status_t err;
	size_t hash_length;
	uint8_t calculated_hash[SHA512_HASH_SIZE];
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;

	err = psa_hash_setup(&operation, alg);
	zassert_equal(err, PSA_SUCCESS, "Failed to setup hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_update(&operation, input, input_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to update hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_finish(&operation, calculated_hash, hash_size, &hash_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to finish hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_abort(&operation);
	zassert_equal(err, PSA_SUCCESS, "Failed to abort hash Alg: %d, Err: %d", alg, err);

	zassert_equal(hash_size, hash_length, "Hash size mismatch. Got %d, expected %d",
		      hash_length, hash_size);

	if (constant_memcmp(calculated_hash, hash, hash_size) != 0) {
		zassert_false(true, "Hash incremental mismathed results");
	}
}

/**
 * @brief Function to test AES CTR encrypt/decrypt
 *
 * @param key_id Valid, revoked or locked key to use for signature check
 * @return PSA_SUCCESS if verify succeeded, otherwise a non-zero error code
 */
static void test_aes_ctr_crypt(mbedtls_svc_key_id_t key_id)
{
	/* Currently not supported, returning "no error" */
	psa_status_t err;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	uint8_t ciphertext[16];
	uint8_t plaintext[16];
	uint32_t out_length;

	err = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CTR);
	zassert_equal(err, PSA_SUCCESS, "Failed to setup AES CTR encryption: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_set_iv(&operation, aes_ctr_nonce, ARRAY_SIZE(aes_ctr_nonce));
	zassert_equal(err, PSA_SUCCESS, "Failed to setup AES CTR IV: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_update(&operation, aes_ctr_plaintext, ARRAY_SIZE(aes_ctr_plaintext),
				ciphertext, ARRAY_SIZE(ciphertext), &out_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to do AES CTR cipher update: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_finish(&operation, ciphertext + out_length,
				ARRAY_SIZE(ciphertext) - out_length, &out_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to do AES CTR cipher finish: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	/** NOTE: Abort cipher currently unsupported (size-optimization) */

	if (constant_memcmp(ciphertext, aes_ctr_ciphertext, ARRAY_SIZE(aes_ctr_ciphertext)) != 0) {
		zassert_false(true, "AES CTR encrypted ciphertext mismathed");
	}

	/* Test out decryption */
	err = psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_CTR);
	zassert_equal(err, PSA_SUCCESS, "Failed to setup AES CTR decryption: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_set_iv(&operation, aes_ctr_nonce, ARRAY_SIZE(aes_ctr_nonce));
	zassert_equal(err, PSA_SUCCESS, "Failed to setup AES CTR IV: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_update(&operation, aes_ctr_ciphertext, ARRAY_SIZE(aes_ctr_ciphertext),
				plaintext, ARRAY_SIZE(plaintext), &out_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to do AES CTR cipher update: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	err = psa_cipher_finish(&operation, ciphertext + out_length,
				ARRAY_SIZE(ciphertext) -  + out_length, &out_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to do AES CTR cipher finish: slot_id: %d, err: %d",
		      KMU_GET_SLOT_ID(key_id), err);

	/** NOTE: Abort cipher currently unsupported (size-optimization) */

	if (constant_memcmp(ciphertext, aes_ctr_ciphertext, ARRAY_SIZE(aes_ctr_ciphertext)) != 0) {
		zassert_false(true, "AES CTR encrypted ciphertext mismathed");
	}
}

static void test_verify(void)
{
	bool ran_verify = false;

	/* Ed25519 */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		test_ed25519_verify(KMU_KEY_ID_PUBKEY_ED25519_REVOKABLE);
		test_ed25519_verify(KMU_KEY_ID_PUBKEY_ED25519_READ_ONLY);
		ran_verify = true;
	}

	/* Ed25519ph */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		test_ed25519ph_verify(KMU_KEY_ID_PUBKEY_ED25519PH_REVOKABLE);
		test_ed25519ph_verify(KMU_KEY_ID_PUBKEY_ED25519PH_READ_ONLY);
		ran_verify = true;
	}

	/* ECDSA secp256r1 (message/hash) */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_256, PSA_WANT_ECC_SECP_R1_256))) {
		test_ecdsa_secp256r1_verify_message(KMU_KEY_ID_PUBKEY_SECP256R1_REVOKABLE);
		test_ecdsa_secp256r1_verify_message(KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY);
		test_ecdsa_secp256r1_verify_hash(KMU_KEY_ID_PUBKEY_SECP256R1_REVOKABLE);
		test_ecdsa_secp256r1_verify_hash(KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY);
		ran_verify = true;
	}

	/* ECDSA secp384r1 (message/hash) */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_384, PSA_WANT_ECC_SECP_R1_384))) {
		test_ecdsa_secp384r1_verify_message(KMU_KEY_ID_PUBKEY_SECP384R1_REVOKABLE);
		test_ecdsa_secp384r1_verify_message(KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY);
		test_ecdsa_secp384r1_verify_hash(KMU_KEY_ID_PUBKEY_SECP384R1_REVOKABLE);
		test_ecdsa_secp384r1_verify_hash(KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY);
		ran_verify = true;
	}

	zassert_true(ran_verify, "Did not run any signature verify, check configs!");

}

static void test_hash(void)
{
	bool ran_hash = false;

	if (IS_ENABLED(PSA_WANT_ALG_SHA_256)) {
		psa_algorithm_t alg = PSA_ALG_SHA_256;

		test_hash_compute(alg, ecdsa_secp256r1_msg, ARRAY_SIZE(ecdsa_secp256r1_msg),
				  ecdsa_secp256r1_hash, SHA256_HASH_SIZE);


		test_hash_incremental(alg, ecdsa_secp256r1_msg, ARRAY_SIZE(ecdsa_secp256r1_msg),
				  ecdsa_secp256r1_hash, SHA256_HASH_SIZE);

		ran_hash = true;
	}

	if (IS_ENABLED(PSA_WANT_ALG_SHA_384)) {
		psa_algorithm_t alg = PSA_ALG_SHA_384;

		test_hash_compute(alg, ecdsa_secp384r1_msg, ARRAY_SIZE(ecdsa_secp384r1_msg),
				  ecdsa_secp384r1_hash, SHA384_HASH_SIZE);


		test_hash_incremental(alg, ecdsa_secp384r1_msg, ARRAY_SIZE(ecdsa_secp384r1_msg),
				  ecdsa_secp384r1_hash, SHA384_HASH_SIZE);

		ran_hash = true;
	}

	if (IS_ENABLED(PSA_WANT_ALG_SHA_512)) {
		psa_algorithm_t alg = PSA_ALG_SHA_512;

		test_hash_compute(alg, ed25519ph_msg, ARRAY_SIZE(ed25519ph_msg),
				  ed25519ph_hash, SHA512_HASH_SIZE);

		test_hash_incremental(alg, ed25519ph_msg, ARRAY_SIZE(ed25519ph_msg),
				  ed25519ph_hash, SHA512_HASH_SIZE);

		ran_hash = true;
	}

	zassert_true(ran_hash, "Did not run any hash calculation, check configs!");
}

static void test_crypt(void)
{
	bool ran_encrypt = false;

	/* AES CTR using 256-bits key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_CTR, PSA_WANT_AES_KEY_SIZE_256)) {
		test_aes_ctr_crypt(KMU_KEY_ID_AES_256_KEY_REVOKABLE);
		test_aes_ctr_crypt(KMU_KEY_ID_AES_256_KEY_READ_ONLY);

		ran_encrypt = true;
	}

	zassert_true(ran_encrypt, "Did not run encrypt, check configs!");
}

static void test_generate_random(void)
{
	/**
	 * Note: These tests are just for sanity. psa_generate_random inside
	 * PSA core lite is just calling directly to the PSA crypto driver
	 * wrapper similar to oberon-psa-crypto
	 */
	#define BUFFER_SIZE (16)

	psa_status_t err;

	 /* 2 buffers with different content (not all zeros) */
	const uint8_t comp_buffer1[BUFFER_SIZE] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	const uint8_t comp_buffer2[BUFFER_SIZE] = {
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};

	uint8_t buffer1[BUFFER_SIZE];
	uint8_t buffer2[BUFFER_SIZE];

	memcpy(buffer1, comp_buffer1, BUFFER_SIZE);
	memcpy(buffer2, comp_buffer2, BUFFER_SIZE);

	err = psa_generate_random(buffer1, BUFFER_SIZE);
	zassert_equal(err, PSA_SUCCESS, "psa_generate_random failed. Err: %d", err);

	err = psa_generate_random(buffer2, BUFFER_SIZE);
	zassert_equal(err, PSA_SUCCESS, "psa_generate_random failed. Err: %d", err);

	/* Verify that content is filled */
	if (constant_memcmp(buffer1, comp_buffer1, BUFFER_SIZE) == 0) {
		zassert_false(true, "psa_genrate_random didn't fill output!");
	}
	if (constant_memcmp(buffer2, comp_buffer2, BUFFER_SIZE) == 0) {
		zassert_false(true, "psa_generate_random didn't fill output!");
	}

	/* Check generated output that it isn't the same */
	if (constant_memcmp(buffer1, buffer2, BUFFER_SIZE) == 0) {
		zassert_false(true, "psa_generate_random created same output!");
	}

	#undef BUFFER_SIZE
}

static void test_revoke_keys(void)
{
	bool ran_revoke = false;

	if (IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		/* Try to revoke the revokable Ed25519 key */
		revoke_key(KMU_KEY_ID_PUBKEY_ED25519_REVOKABLE);
		ran_revoke = true;
	}

	if (IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		/* Try to revoke the revokable Ed25519ph key */
		revoke_key(KMU_KEY_ID_PUBKEY_ED25519PH_REVOKABLE);
		ran_revoke = true;
	}

	/* ECDSA using secp256r1 and SHA-256 */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_256, PSA_WANT_ECC_SECP_R1_256))) {
		/* Try to revoke the revokable ECDSA key */
		revoke_key(KMU_KEY_ID_PUBKEY_SECP256R1_REVOKABLE);
		ran_revoke = true;
	}

	/* ECDSA using secp384r1 and SHA-384 */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_384, PSA_WANT_ECC_SECP_R1_384))) {
		/* Try to revoke the revokable ECDSA key */
		revoke_key(KMU_KEY_ID_PUBKEY_SECP384R1_REVOKABLE);
		ran_revoke = true;
	}

	/* Any AES using 256-bits key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_CTR, PSA_WANT_AES_KEY_SIZE_256)) {
		/* Try to revoke the revokable AES key */
		revoke_key(KMU_KEY_ID_AES_256_KEY_REVOKABLE);
		ran_revoke = true;
	}

	zassert_true(ran_revoke, "Did not run any valid revocation, check configs!");
}

static void test_lock_keys(void)
{
	bool ran_lock = false;

	if (IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		/* Try to lock the read-only Ed25519 key */
		lock_key(KMU_KEY_ID_PUBKEY_ED25519_READ_ONLY);
		ran_lock = true;
	}

	if (IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		/* Try to lock the read-only Ed25519ph key */
		lock_key(KMU_KEY_ID_PUBKEY_ED25519PH_READ_ONLY);
		ran_lock = true;
	}

	/* ECDSA using secp256r1 and SHA-256 */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_256, PSA_WANT_ECC_SECP_R1_256))) {
		/* Try to lock the read-only ECDSA key */
		lock_key(KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY);
		ran_lock = true;
	}

	/* ECDSA using secp384r1 and SHA-384 */
	if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
		     IS_ENABLED_ALL(PSA_WANT_ALG_SHA_384, PSA_WANT_ECC_SECP_R1_384))) {
		/* Try to lock the read-only ECDSA key */
		lock_key(KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY);
		ran_lock = true;
	}

	/* Any AES using 256-bits key */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_CTR, PSA_WANT_AES_KEY_SIZE_256)) {
		/* Try to lock the read-only AES key */
		lock_key(KMU_KEY_ID_AES_256_KEY_READ_ONLY);
		ran_lock = true;
	}

	zassert_true(ran_lock, "Did not run any valid locking, check configs!");
}

void test_invalid_kmu(void)
{
	psa_status_t err;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	mbedtls_svc_key_id_t key_id;
	mbedtls_svc_key_id_t imported_key_id;
	uint8_t *pubkey_buffer;
	size_t pubkey_size;

	if (IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		key_id = KMU_KEY_ID_PUBKEY_ED25519_READ_ONLY;
		pubkey_size = ED25519_PUBKEY_SIZE;
		pubkey_buffer = ed25519_pubkey;

		init_attributes_ed25519_public_key(key_id, CRACEN_KEY_PERSISTENCE_READ_ONLY,
						   &attributes);
	} else if (IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH, PSA_WANT_ECC_TWISTED_EDWARDS_255)) {
		key_id = KMU_KEY_ID_PUBKEY_ED25519PH_READ_ONLY;
		pubkey_size = ED25519_PUBKEY_SIZE;
		pubkey_buffer = ed25519ph_pubkey;

		init_attributes_ed25519_public_key(key_id, CRACEN_KEY_PERSISTENCE_READ_ONLY,
						   &attributes);
	} else if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
			    IS_ENABLED_ALL(PSA_WANT_ALG_SHA_256, PSA_WANT_ECC_SECP_R1_256))) {
		key_id = KMU_KEY_ID_PUBKEY_SECP256R1_READ_ONLY;
		pubkey_size = ECDSA_SECP256R1_PUBKEY_SIZE;
		pubkey_buffer = ecdsa_secp256r1_pubkey;

		init_attributes_ecdsa_secp256r1_public_key(key_id, CRACEN_KEY_PERSISTENCE_READ_ONLY,
							   &attributes);
	} else if (UTIL_AND(IS_ENABLED_ANY(PSA_WANT_ALG_ECDSA, PSA_WANT_ALG_DETERMINISTIC_ECDSA),
			    IS_ENABLED_ALL(PSA_WANT_ALG_SHA_384, PSA_WANT_ECC_SECP_R1_384))) {
		key_id = KMU_KEY_ID_PUBKEY_SECP384R1_READ_ONLY;
		pubkey_size = ECDSA_SECP384R1_PUBKEY_SIZE;
		pubkey_buffer = ecdsa_secp384r1_pubkey;

		init_attributes_ecdsa_secp384r1_public_key(key_id, CRACEN_KEY_PERSISTENCE_READ_ONLY,
							   &attributes);
	} else {
		zassert_false(true, "No valid public key for invalid KMU test");
		return;
	}

	/* Try to import on already existing key */
	err = psa_import_key(&attributes, pubkey_buffer, pubkey_size, &imported_key_id);
	zassert_equal(err, PSA_ERROR_ALREADY_EXISTS,
		"Failed on import on existing (expected PSA_ERROR_ALREADY_EXISTS) slot_id: %d, err: %d",
		KMU_GET_SLOT_ID(key_id), err);

	/* Try to destroy an existing read-only key */
	err = psa_destroy_key(key_id);
	zassert_equal(err, PSA_ERROR_NOT_PERMITTED,
		"Failed on erase of read-only-key (expected PSA_ERROR_ALREADY_EXISTS) slot_id: %d, err: %d",
		KMU_GET_SLOT_ID(key_id), err);
}

void test_main(void)
{
	/* Provisioning key(s) is a requirement for running all tests*/
	provision_keys();

	/* Verify is required to be run in these tests */
	if (IS_ENABLED_ANY(PSA_WANT_ALG_PURE_EDDSA, PSA_WANT_ALG_ED25519PH,
			   PSA_WANT_ALG_DETERMINISTIC_ECDSA, PSA_WANT_ALG_ECDSA)) {
		test_verify();
	} else {
		zassert_false(true, "No configuration to run verify signature!");
		return;
	}

	/* + Hashing if verify-hash strategy ECDSA or Ed25519ph */
	if (IS_ENABLED_ANY(PSA_WANT_ALG_SHA_256, PSA_WANT_ALG_SHA_512,
			   PSA_WANT_ALG_SHA_384)) {
		test_hash();
	}

	/* + Test any encryption (optional added feature) */
	if (IS_ENABLED_ANY(PSA_WANT_ALG_CTR)) {
		test_crypt();
	}

	/* + Test Generate random (optional added feature )*/
	if (IS_ENABLED(PSA_WANT_GENERATE_RANDOM)) {
		test_generate_random();
	}

	/* Test revocation (required feature) */
	test_revoke_keys();

	/* Test locking (on read-only keys, required feature) */
	test_lock_keys();

	/* Test invalid key operations*/
	test_invalid_kmu();

	zassert_true(true, "");
}
