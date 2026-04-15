/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef PSA_CORE_LITE_VOLATILE_KEY_STORAGE_H_
#define PSA_CORE_LITE_VOLATILE_KEY_STORAGE_H_

#include <util/util_macro.h>
#include <psa/crypto.h>
#include "psa_core_lite.h"

#if defined(PSA_WANT_ALG_CTR)
/* Ensure that the largest key size is supported */
#if PSA_WANT_AES_KEY_SIZE_256
#define PSA_LITE_KEY_MAX_SIZE	(32)
#elif PSA_WANT_AES_KEY_SIZE_128
#define PSA_LITE_KEY_MAX_SIZE	(16)
#else
#error "FW encryption requires either AES-256 or AES-128 being enabled"
#endif /* PSA_WANT_AES_KEY_SIZE_XXX */

#define PSA_LITE_MAX_KEYS_SUPPORTED	1u
#define PSA_LITE_KEY_ID_NULL		PSA_KEY_ID_NULL
#define PSA_LITE_KEY_ID_MIN		PSA_KEY_ID_VENDOR_MIN
#define PSA_LITE_KEY_ID_MAX		PSA_LITE_KEY_ID_MIN + PSA_LITE_MAX_KEYS_SUPPORTED - 1u

typedef struct {
	psa_key_attributes_t key_attributes;
	uint8_t key[PSA_LITE_KEY_MAX_SIZE];
	size_t key_size;
} psa_lite_key_slot_t;

/**
 * @brief Check if provided key_id corresponds to volatile key.
 *
 * @param[in] key_id	Key id to check.
 *
 * @retval true		Provided key is a volatile key.
 * @retval false	Provided key is not a volatile key.
 */
static inline bool psa_lite_key_id_is_volatile(mbedtls_svc_key_id_t key_id)
{
	return (key_id >= PSA_LITE_KEY_ID_MIN) && (key_id <= PSA_LITE_KEY_ID_MAX);
}

/**
 * @brief Returns a pointer to the previously allocated volatile key slot or
 *	  allocates a new slot.
 *
 * @note If the provided key id is set to PSA_LITE_KEY_ID_NULL, a new
 *	 slot will be allocated and key id will be set to the value
 *	 corresponding to this slot.
 *
 * @param[in, out] key_id	Key id corresponding to the slot or id
 *				with PSA_LITE_KEY_ID_NULL value.
 * @param[out] slot		A pointer to the existing or newly allocated
 *				volatile key slot.
 *
 * @retval PSA_SUCCESS
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_DOES_NOT_EXIST
 * @retval PSA_ERROR_INSUFFICIENT_MEMORY
 */
psa_status_t psa_lite_get_key_slot(mbedtls_svc_key_id_t *key_id, psa_lite_key_slot_t **slot);

/**
 * @brief Clears key slot that has been allocated for the specified key id.
 *
 * @param[in] key_id	Volatile key id that corresponds to the slot that must be cleared.
 */
void psa_lite_free_key_slot(mbedtls_svc_key_id_t key_id);

#endif /* PSA_WANT_ALG_CTR */

#endif /* PSA_CORE_LITE_VOLATILE_KEY_STORAGE_H_ */
