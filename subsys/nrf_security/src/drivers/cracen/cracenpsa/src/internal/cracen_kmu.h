/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <stdint.h>
#include <cracen_psa.h>

#define CRACEN_KMU_PUSH_AREA_SIZE 96u
#define CRACEN_KMU_MAX_KEY_SIZE   48u
#define CRACEN_KMU_SLOT_KEY_SIZE  16u

enum kmu_metadata_key_bits {
	METADATA_ALG_KEY_BITS_128 = 1,
	METADATA_ALG_KEY_BITS_192 = 2,
	METADATA_ALG_KEY_BITS_255 = 3,
	METADATA_ALG_KEY_BITS_256 = 4,
	METADATA_ALG_KEY_BITS_384_SEED = 5,
	METADATA_ALG_KEY_BITS_384 = 6,
	METADATA_ALG_KEY_BITS_RESERVED_2 = 7,
};

typedef struct {
	uint8_t key_usage_scheme: 2; /* value of @ref cracen_kmu_metadata_key_usage_scheme. */
	uint8_t number_of_slots: 3;  /* Number of slots to push. */
	uint8_t slot_id;	     /* KMU slot number. */
} kmu_opaque_key_buffer;

/* Two KMU slots are reserved for storing the invalidation data for the protected RAM.
 * These are meant to have random data that can pushed to the protected RAM after
 * an actual key is being used so that the key material does not reside in the protected
 * RAM for more than the required time.
 */
#define PROTECTED_RAM_INVALIDATION_DATA_SLOT1 248
#define PROTECTED_RAM_INVALIDATION_DATA_SLOT2 249

extern uint8_t kmu_push_area[CRACEN_KMU_PUSH_AREA_SIZE];

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
 */
psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length);

/**
 * @brief Provision a key in the KMU.
 */
psa_status_t cracen_kmu_provision(const psa_key_attributes_t *key_attr, int slot_id,
				  const uint8_t *key_buffer, size_t key_buffer_size);

/**
 * @brief Destroy a key stored in the KMU.
 */
psa_status_t cracen_kmu_destroy_key(const psa_key_attributes_t *attributes);

/**
 * @brief Provision the protected RAM invalidation data.
 *
 * This function provisions two KMU slots with random data that can be used to invalidate
 * the protected RAM after a key is used.
 *
 * @return PSA status code.
 */
psa_status_t cracen_provision_prot_ram_inv_slots(void);

/**
 * @brief Push the protected RAM invalidation slots to protected RAM.
 *
 * This function pushes the two KMU slots with random data to the protected RAM,
 * thereby invalidating any existing key material.
 *
 * @return PSA status code.
 */
psa_status_t cracen_push_prot_ram_inv_slots(void);
