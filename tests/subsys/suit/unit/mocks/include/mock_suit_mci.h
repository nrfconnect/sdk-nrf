/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_MCI_H__
#define MOCK_SUIT_MCI_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_mci.h>

/* suit_generic_ids.c */
#ifdef CONFIG_MOCK_SUIT_MCI_GENERIC_IDS
FAKE_VALUE_FUNC(int, suit_mci_nordic_vendor_id_get, const suit_uuid_t **);
#endif /* CONFIG_MOCK_SUIT_MCI_GENERIC_IDS */

/* utils.c */
#ifdef CONFIG_MOCK_SUIT_MCI_UTILS
FAKE_VALUE_FUNC(int, suit_metadata_uuid_compare, const suit_uuid_t *, const suit_uuid_t *);
FAKE_VALUE_FUNC(int, suit_mci_manifest_parent_child_validate, const suit_manifest_class_id_t *,
		const suit_manifest_class_id_t *);
#endif /* CONFIG_MOCK_SUIT_MCI_UTILS */

/* mci_<soc>.c */
FAKE_VALUE_FUNC(int, suit_mci_supported_manifest_class_ids_get, suit_manifest_class_info_t *,
		size_t *);
FAKE_VALUE_FUNC(int, suit_mci_invoke_order_get, const suit_manifest_class_id_t **, size_t *);
FAKE_VALUE_FUNC(int, suit_mci_downgrade_prevention_policy_get, const suit_manifest_class_id_t *,
		suit_downgrade_prevention_policy_t *);
FAKE_VALUE_FUNC(int, suit_mci_independent_update_policy_get, const suit_manifest_class_id_t *,
		suit_independent_updateability_policy_t *);
FAKE_VALUE_FUNC(int, suit_mci_manifest_class_id_validate, const suit_manifest_class_id_t *);
FAKE_VALUE_FUNC(int, suit_mci_signing_key_id_validate, const suit_manifest_class_id_t *, uint32_t);
FAKE_VALUE_FUNC(int, suit_mci_processor_start_rights_validate, const suit_manifest_class_id_t *,
		int);
FAKE_VALUE_FUNC(int, suit_mci_memory_access_rights_validate, const suit_manifest_class_id_t *,
		void *, size_t);
FAKE_VALUE_FUNC(int, suit_mci_platform_specific_component_rights_validate,
		const suit_manifest_class_id_t *, int);
FAKE_VALUE_FUNC(int, suit_mci_vendor_id_for_manifest_class_id_get, const suit_manifest_class_id_t *,
		const suit_uuid_t **);
FAKE_VALUE_FUNC(int, suit_mci_manifest_parent_child_declaration_validate,
		const suit_manifest_class_id_t *, const suit_manifest_class_id_t *);
FAKE_VALUE_FUNC(int, suit_mci_manifest_process_dependency_validate,
		const suit_manifest_class_id_t *, const suit_manifest_class_id_t *);
FAKE_VALUE_FUNC(int, suit_mci_init);

static inline void mock_suit_mci_reset(void)
{
#ifdef CONFIG_MOCK_SUIT_MCI_GENERIC_IDS
	RESET_FAKE(suit_mci_nordic_vendor_id_get);
#endif /* CONFIG_MOCK_SUIT_MCI_GENERIC_IDS */

#ifdef CONFIG_MOCK_SUIT_MCI_UTILS
	RESET_FAKE(suit_metadata_uuid_compare);
	RESET_FAKE(suit_mci_manifest_parent_child_validate);
#endif /* CONFIG_MOCK_SUIT_MCI_UTILS */

	RESET_FAKE(suit_mci_supported_manifest_class_ids_get);
	RESET_FAKE(suit_mci_invoke_order_get);
	RESET_FAKE(suit_mci_downgrade_prevention_policy_get);
	RESET_FAKE(suit_mci_independent_update_policy_get);
	RESET_FAKE(suit_mci_manifest_class_id_validate);
	RESET_FAKE(suit_mci_signing_key_id_validate);
	RESET_FAKE(suit_mci_processor_start_rights_validate);
	RESET_FAKE(suit_mci_memory_access_rights_validate);
	RESET_FAKE(suit_mci_platform_specific_component_rights_validate);
	RESET_FAKE(suit_mci_vendor_id_for_manifest_class_id_get);
	RESET_FAKE(suit_mci_manifest_process_dependency_validate);
	RESET_FAKE(suit_mci_manifest_parent_child_declaration_validate);
	RESET_FAKE(suit_mci_init);
}

#endif /* MOCK_SUIT_MCI_H__ */
