/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_BUILTIN_KEY_POLICY_H
#define CRACEN_PSA_BUILTIN_KEY_POLICY_H

#include <psa/crypto.h>
#include <psa/crypto_values.h>

#if defined(__NRF_TFM__)

typedef struct {
	mbedtls_key_owner_id_t owner;
	psa_drv_slot_number_t key_slot;
} cracen_builtin_ikg_key_policy_t;

typedef enum {
	KMU_ENTRY_SLOT_SINGLE,
	KMU_ENTRY_SLOT_RANGE,
} cracen_kmu_entry_type_t;

/* When defining a range of KMU slots both the start and end slot numbers are inclusive. */
typedef struct {
	mbedtls_key_owner_id_t owner;
	psa_drv_slot_number_t key_slot_start;
	psa_drv_slot_number_t key_slot_end;
	cracen_kmu_entry_type_t kmu_entry_type;
} cracen_builtin_kmu_key_policy_t;

bool cracen_builtin_key_user_allowed(const psa_key_attributes_t *attributes);

#else

static inline bool cracen_builtin_key_user_allowed(const psa_key_attributes_t *attributes)
{
	(void)attributes;
	return true;
}

#endif /* __NRF_TFM__ */

#endif /* CRACEN_PSA_BUILTIN_KEY_POLICY_H */
