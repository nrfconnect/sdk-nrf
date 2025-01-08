/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECRYPT_TEST_UTILS_H_
#define DECRYPT_TEST_UTILS_H_

#include <psa/crypto.h>
#include <suit_metadata.h>

/* This module holds common utilities for SUIT decrypt filter tests.
 * It defines default encryption key data, plaintext and ciphertext along with other
 * suit_encryption_info struct members that can be used while testing.
 *
 * WARNING: All of the const values defined in this module CAN be changed freely.
 * This also means that any test depending on them should expect it.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Default initalization for suit_encryption_info struct.
 * 'cek_key_id_cbor' - should be taken from init_encryption_key() call.
 */
#define DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor) \
	{ \
		.enc_alg_id = suit_cose_aes256_gcm, \
		.IV = { \
				 .value = decrypt_test_iv_direct, \
				 .len = sizeof(decrypt_test_iv_direct), \
			  }, \
		.aad = { \
				  .value = decrypt_test_aad, \
				  .len = strlen(decrypt_test_aad), \
			   }, \
		.kw_alg_id = suit_cose_direct, \
		.kw_key.direct = {.key_id = {.value = (cek_key_id_cbor), \
		.len = sizeof((cek_key_id_cbor))},} \
	}

#define DECRYPT_TEST_KEY_LENGTH	32
extern const uint8_t decrypt_test_key_data[DECRYPT_TEST_KEY_LENGTH];

#define DECRYPT_TEST_PLAINTEXT_LENGTH	61
extern const uint8_t decrypt_test_plaintext[DECRYPT_TEST_PLAINTEXT_LENGTH];

#define DECRYPT_TEST_AAD_LENGTH	11
extern const uint8_t decrypt_test_aad[DECRYPT_TEST_AAD_LENGTH];

#define DECRYPT_TEST_WRAPPED_CEK_LENGTH	40
extern const uint8_t decrypt_test_wrapped_cek[DECRYPT_TEST_WRAPPED_CEK_LENGTH];

#define DECRYPT_TEST_CIPHERTEXT_AES_KW_LENGTH	(DECRYPT_TEST_PLAINTEXT_LENGTH + 16)
extern const uint8_t decrypt_test_ciphertext_aes_kw[DECRYPT_TEST_CIPHERTEXT_AES_KW_LENGTH];

#define DECRYPT_TEST_IV_AES_KW_LENGTH	12
extern const uint8_t decrypt_test_iv_aes_kw[DECRYPT_TEST_IV_AES_KW_LENGTH];

#define DECRYPT_TEST_CIPHERTEXT_DIRECT_LENGTH	(DECRYPT_TEST_PLAINTEXT_LENGTH + 16)
extern const uint8_t decrypt_test_ciphertext_direct[DECRYPT_TEST_CIPHERTEXT_DIRECT_LENGTH];

#define DECRYPT_TEST_IV_DIRECT_LENGTH 12
extern const uint8_t decrypt_test_iv_direct[DECRYPT_TEST_IV_DIRECT_LENGTH];

extern const suit_manifest_class_id_t decrypt_test_sample_class_id;

/**
 * @brief		Initialize encryption key with given binary data.
 *			The key is initialized as PSA_KEY_TYPE_AES.
 *
 * @param[in]  data	binary key data.
 * @param[in]  size  size of @a data.
 * @param[out] key_id key ID of imported key.
 * @param[in]  alg encryption algorithm.
 * @param[out] cbor_key_id key ID of imported key in cbor format.
 *
 * @retval		PSA_SUCCESS successfully imported given key.
 * @retval		PSA_ERROR_* returned error from dependent PSA calls.
 */
psa_status_t decrypt_test_init_encryption_key(const uint8_t *data, size_t size,
		psa_key_id_t *key_id, psa_key_id_t alg, uint8_t *cbor_key_id);

#ifdef __cplusplus
}
#endif

#endif /* DECRYPT_TEST_UTILS_H_ */
