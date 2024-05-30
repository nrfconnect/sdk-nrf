/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <stdint.h>
#include "cracen_psa.h"

#define CRACEN_KMU_MAX_KEY_SIZE	 32
#define CRACEN_KMU_SLOT_KEY_SIZE 16

enum kmu_metadata_key_bits {
	METADATA_ALG_KEY_BITS_128 = 1,
	METADATA_ALG_KEY_BITS_192 = 2,
	METADATA_ALG_KEY_BITS_255 = 3,
	METADATA_ALG_KEY_BITS_256 = 4,
	METADATA_ALG_KEY_BITS_384_SEED = 5,
	METADATA_ALG_KEY_BITS_RESERVED_1 = 6,
	METADATA_ALG_KEY_BITS_RESERVED_2 = 7,
};

typedef struct {
	uint8_t key_usage_scheme: 2; /* value of @ref kmu_metadata_key_usage_scheme. */
	uint8_t number_of_slots: 3;  /* Number of slots to push. */
	uint8_t slot_id;	     /* KMU slot number. */
} kmu_opaque_key_buffer;

enum kmu_metadata_key_usage_scheme {
	/**
	 * These keys can only be pushed to Cracen's protected RAM.
	 * The keys are not encrypted. Only AES supported.
	 */
	KMU_METADATA_SCHEME_PROTECTED,
	/**
	 * These keys use 3 key slots. Pushed to the seed register.
	 */
	KMU_METADATA_SCHEME_SEED,
	/**
	 * These keys are stored in encrypted form. They will be decrypted
	 * to @ref kmu_push_area for usage.
	 */
	KMU_METADATA_SCHEME_ENCRYPTED,
	/**
	 * These keys are not encrypted. Pushed to @ref kmu_push_area.
	 */
	KMU_METADATA_SCHEME_RAW
};

/* NCSDK-25121: Ensure address of this array is at a fixed address. */
extern uint8_t kmu_push_area[64];

/**
 * @brief Callback function that prepares a key for usage by Cracen.
 *
 * @param[in] user_data
 * @return sxsymcrypt status code.
 */
int cracen_kmu_prepare_key(const uint8_t *user_data);

/**
 * @brief Callback function that clears transient buffers related to key handling.
 *
 * @param[in] user_data
 * @return sxsymcrypt status code.
 */
int cracen_kmu_clean_key(const uint8_t *user_data);

/**
 * @brief Retrieves attributes and opaque key buffer for key.
 *
 * If the key is available the key_buffer will be filled with a valid
 * instance of @ref kmu_opaque_key_buffer.
 *
 * @return psa_status_t
 */
psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length);

/**
 * @brief Provision a key in the KMU.
 *
 * @return psa_status_t
 */
psa_status_t cracen_kmu_provision(const psa_key_attributes_t *key_attr, int slot_id,
				  const uint8_t *key_buffer, size_t key_buffer_size);

/**
 * @brief Revokes key stored in KMU.
 *
 * @return psa_status_t
 */
psa_status_t cracen_kmu_revoke_key_slot(int slot_id);
