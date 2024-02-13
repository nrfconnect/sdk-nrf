/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TRUSTED_STORAGE_AUTH_CRYPT_KEY_H_
#define __TRUSTED_STORAGE_AUTH_CRYPT_KEY_H_

#include <psa/error.h>
#include <psa/storage_common.h>

#define AEAD_KEY_SIZE (32)

psa_status_t trusted_storage_get_key(psa_storage_uid_t uid, uint8_t *key_buf, size_t key_length);

#endif /* __TRUSTED_STORAGE_AUTH_CRYPT_KEY_H_ */
