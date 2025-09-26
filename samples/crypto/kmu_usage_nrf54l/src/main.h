/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#ifndef __MAIN_H_
#define __MAIN_H_

#define NRF_CRYPTO_EXAMPLE_KMU_USAGE_KEY_MAX_TEXT_SIZE (100)

/**
 * @brief Callback function called to generate specific key.
 *
 * @param key_id A pointer to the key ID.
 *
 * @return psa_status_t
 */
typedef int (*generate_key_callback_t)(psa_key_id_t *key_id);

/**
 * @brief Callback function called to execute key usage functionality.
 *
 * @param key_id A pointer to the key ID.
 *
 * @return psa_status_t
 */
typedef int (*use_key_callback_t)(psa_key_id_t *key_id);

typedef struct {
	const generate_key_callback_t gen_key_cb;
	const use_key_callback_t use_key_cb;
} sample_key_operations_t;

typedef struct {
	psa_key_id_t key_id;
	sample_key_operations_t supported_operations;
} sample_key_entry_t;

#endif
