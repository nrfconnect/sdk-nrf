/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "internal/common.h"

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <zephyr/sys/util.h>
#include <stddef.h>
#include <string.h>
#include <psa_manifest/pid.h>
#include <tfm_builtin_key_ids.h>
#include <cracen_psa_builtin_key_policy.h>
#include <drivers/nrfx_utils.h>

/* 0x0 is used by internal code like the hw_unique_key library.
 * It doesn't have the logic to have an owner ID and resides inside TF-M.
 */
#define INTERNAL_CLIENT_ID 0x0

#define MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID -0x3c000000

#define HUK_USAGES (PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_VERIFY_DERIVATION)

static const cracen_builtin_ikg_key_policy_t g_builtin_ikg_policy[] = {
	{.owner = INTERNAL_CLIENT_ID, .key_slot = TFM_BUILTIN_KEY_ID_HUK, .usage = HUK_USAGES},
#ifdef TFM_SP_ITS
	{.owner = TFM_SP_ITS, .key_slot = TFM_BUILTIN_KEY_ID_HUK, .usage = HUK_USAGES},
#endif /* TFM_SP_ITS */
#ifdef TFM_SP_PS
	{.owner = TFM_SP_PS, .key_slot = TFM_BUILTIN_KEY_ID_HUK, .usage = HUK_USAGES},
#endif /* TFM_SP_PS */
#ifdef TFM_SP_PS_TEST
	{.owner = TFM_SP_PS_TEST, .key_slot = TFM_BUILTIN_KEY_ID_HUK, .usage = HUK_USAGES},
#endif /* TFM_SP_PS_TEST */
#ifdef TFM_SP_INITIAL_ATTESTATION
	{.owner = TFM_SP_INITIAL_ATTESTATION,
	 .key_slot = TFM_BUILTIN_KEY_ID_IAK,
	 .usage = PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH |
		  PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH},
#endif /* TFM_SP_INITIAL_ATTESTATION */
	{.owner = MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID,
	 .key_slot = TFM_BUILTIN_KEY_ID_IAK,
	 .usage = PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH},

	{.owner = MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID,
	 .key_slot = TFM_BUILTIN_KEY_ID_HUK,
	 .usage = HUK_USAGES},
};

#ifdef PSA_NEED_CRACEN_KMU_DRIVER

static const cracen_builtin_kmu_key_policy_t g_builtin_kmu_policy[] = {
	{.owner = INTERNAL_CLIENT_ID,
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

bool cracen_kmu_key_user_allowed(const psa_key_attributes_t *attributes)
{
	mbedtls_svc_key_id_t svc_key_id = psa_get_key_id(attributes);
	mbedtls_key_owner_id_t owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(svc_key_id);
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(svc_key_id);
	psa_drv_slot_number_t slot_number = CRACEN_PSA_GET_KMU_SLOT(key_id);

	for (uint32_t i = 0; i < NRFX_ARRAY_SIZE(g_builtin_kmu_policy); i++) {

		switch (g_builtin_kmu_policy[i].kmu_entry_type) {
		case KMU_ENTRY_SLOT_SINGLE:
			if (g_builtin_kmu_policy[i].owner == owner_id &&
			    g_builtin_kmu_policy[i].key_slot_start == slot_number) {
				return true;
			}
			break;
		case KMU_ENTRY_SLOT_RANGE:
			if (g_builtin_kmu_policy[i].owner == owner_id &&
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

psa_key_usage_t cracen_ikg_key_user_get_usage(const psa_key_attributes_t *attributes)
{
	mbedtls_svc_key_id_t svc_key_id = psa_get_key_id(attributes);
	mbedtls_key_owner_id_t owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(svc_key_id);
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(svc_key_id);

	for (uint32_t i = 0; i < NRFX_ARRAY_SIZE(g_builtin_ikg_policy); i++) {
		if (g_builtin_ikg_policy[i].owner == owner_id &&
		    g_builtin_ikg_policy[i].key_slot == key_id) {
			return g_builtin_ikg_policy[i].usage;
		}
	}

	return 0; /* No usage allowed. */
}
