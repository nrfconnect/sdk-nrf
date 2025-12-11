/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <zephyr/sys/util.h>
#include <stddef.h>
#include <string.h>

#include <psa_manifest/pid.h>
#include <tfm_builtin_key_ids.h>
#include <cracen_psa_builtin_key_policy.h>
#include <drivers/nrfx_utils.h>

#define MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID (-0x3c000000)

static const cracen_builtin_ikg_key_policy_t g_builtin_ikg_policy[] = {
#ifdef TFM_SP_ITS
	{.owner = TFM_SP_ITS, .key_slot = TFM_BUILTIN_KEY_ID_HUK},
#endif /* TFM_SP_ITS */
#ifdef TFM_SP_PS
	{.owner = TFM_SP_PS, .key_slot = TFM_BUILTIN_KEY_ID_HUK},
#endif /* TFM_SP_PS */
#ifdef TFM_SP_PS_TEST
	{.owner = TFM_SP_PS_TEST, .key_slot = TFM_BUILTIN_KEY_ID_HUK},
#endif /* TFM_SP_PS_TEST */
#ifdef TFM_SP_INITIAL_ATTESTATION
	{.owner = TFM_SP_INITIAL_ATTESTATION, .key_slot = TFM_BUILTIN_KEY_ID_IAK}
#endif /* TFM_SP_INITIAL_ATTESTATION */
};

#ifdef PSA_NEED_CRACEN_KMU_DRIVER

static const cracen_builtin_kmu_key_policy_t g_builtin_kmu_policy[] = {
	/* 0x0 is used by libraries like the hw_unique_key library when manually generating psa
	 * key_ids. Allow access to all slots for these libraries since they don't have the logic to
	 * have an owner id and reside inside TF-M.
	 */
	{.owner = 0x0,
	 .key_slot_start = 0,
	 .key_slot_end = 255,
	 .kmu_entry_type = KMU_ENTRY_SLOT_RANGE},
#ifdef TFM_SP_ITS
	{.owner = TFM_SP_ITS,
	 .key_slot_start = 0,
	 .key_slot_end = 255,
	 .kmu_entry_type = KMU_ENTRY_SLOT_RANGE},
#endif /* TFM_SP_ITS */
#ifdef TFM_SP_CRYPTO
	{.owner = TFM_SP_CRYPTO,
	 .key_slot_start = 0,
	 .key_slot_end = 255,
	 .kmu_entry_type = KMU_ENTRY_SLOT_RANGE},
#endif /* TFM_SP_CRYPTO */
	/* The docs have KMU slots >= 180 reserved so don't allow NS users to access them */
	{.owner = MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID,
	 .key_slot_start = 0,
	 .key_slot_end = 179,
	 .kmu_entry_type = KMU_ENTRY_SLOT_RANGE}};

static bool cracen_builtin_kmu_user_allowed(mbedtls_key_owner_id_t owner,
					    psa_drv_slot_number_t slot_number,
					    const psa_key_attributes_t *attributes)
{
	for (uint32_t i = 0; i < NRFX_ARRAY_SIZE(g_builtin_kmu_policy); i++) {

		switch (g_builtin_kmu_policy[i].kmu_entry_type) {
		case KMU_ENTRY_SLOT_SINGLE:
			if (g_builtin_kmu_policy[i].owner == owner &&
			    g_builtin_kmu_policy[i].key_slot_start == slot_number) {
				return true;
			}
			break;
		case KMU_ENTRY_SLOT_RANGE:
			if (g_builtin_kmu_policy[i].owner == owner &&
			    (slot_number >= g_builtin_kmu_policy[i].key_slot_start &&
			     slot_number <= g_builtin_kmu_policy[i].key_slot_end)) {
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

static bool cracen_builtin_ikg_user_allowed(mbedtls_key_owner_id_t owner,
					    psa_drv_slot_number_t slot_number,
					    const psa_key_attributes_t *attributes)
{
	for (uint32_t i = 0; i < NRFX_ARRAY_SIZE(g_builtin_ikg_policy); i++) {
		if (g_builtin_ikg_policy[i].owner == owner &&
		    g_builtin_ikg_policy[i].key_slot == slot_number) {
			return true;
		}
	}

	return false;
}

bool cracen_builtin_key_user_allowed(const psa_key_attributes_t *attributes)
{
	mbedtls_key_owner_id_t owner = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
	psa_drv_slot_number_t slot_id;

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {

		slot_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes));
		return cracen_builtin_ikg_user_allowed(owner, slot_id, attributes);
	}

#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {

		slot_id = CRACEN_PSA_GET_KMU_SLOT(
			MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
		return cracen_builtin_kmu_user_allowed(owner, slot_id, attributes);
	}
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	return true;
}
