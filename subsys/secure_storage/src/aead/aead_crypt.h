/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SECURE_STORAGE_AUTH_CRYPT_PSA_H_
#define __SECURE_STORAGE_AUTH_CRYPT_PSA_H_

#include <psa/error.h>
#include <psa/storage_common.h>

psa_status_t secure_storage_aead_init(void);

size_t secure_storage_aead_get_encrypted_size(size_t data_size);

psa_status_t secure_storage_aead_encrypt(const void *key_buf, size_t key_len, const void *nonce_buf,
					 size_t nonce_len, const void *add_buf, size_t add_len,
					 const void *input_buf, size_t input_len, void *output_buf,
					 size_t output_size, size_t *output_len);

psa_status_t secure_storage_aead_decrypt(const void *key_buf, size_t key_len, const void *nonce_buf,
					 size_t nonce_len, const void *add_buf, size_t add_len,
					 const void *input_buf, size_t input_len, void *output_buf,
					 size_t output_size, size_t *output_len);

#endif /* __SECURE_STORAGE_AUTH_CRYPT_PSA_H_ */
