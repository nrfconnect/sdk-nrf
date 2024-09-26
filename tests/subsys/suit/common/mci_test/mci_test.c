/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>
#include <suit_execution_mode.h>
#if defined(CONFIG_MBEDTLS) || defined(CONFIG_NRF_SECURITY)
#include <psa/crypto.h>
#endif /* CONFIG_MBEDTLS || CONFIG_NRF_SECURITY*/

/* Test topology: Root Manifest orchestrating application domain.
 * Summarizing - Single Root Manifest and one local manifest.
 */

/* RFC4122 uuid5(nordic_vid, 'test_sample_root')
 */
static const suit_manifest_class_id_t nordic_root_manifest_class_id = {
	{0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1, 0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b,
	 0x0a}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_app')
 */
static const suit_manifest_class_id_t nordic_app_manifest_class_id = {
	{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c, 0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e,
	 0x36}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_recovery')
 */
static const suit_manifest_class_id_t nordic_recovery_manifest_class_id = {
	{0x74, 0xa0, 0xc6, 0xe7, 0xa9, 0x2a, 0x56, 0x00, 0x9c, 0x5d, 0x30, 0xee, 0x87, 0x8b, 0x06,
	 0xba}};

typedef struct {
	const suit_manifest_class_id_t *manifest_class_id;
	const suit_manifest_class_id_t *parent_manifest_class_id;
	suit_downgrade_prevention_policy_t downgrade_prevention_policy;
	suit_independent_updateability_policy_t independent_update_policy;
	uint32_t signing_key_bits;
	uint32_t signing_key_mask;
} manifest_config_t;

static manifest_config_t supported_manifests[] = {
	{&nordic_root_manifest_class_id, NULL, SUIT_DOWNGRADE_PREVENTION_DISABLED,
	 SUIT_INDEPENDENT_UPDATE_ALLOWED,
	 /* signing_key_mask equal to -1 means signing with specified key is required
	  */
	 0x00000000, 0xFFFFFFFF},
	{&nordic_app_manifest_class_id, &nordic_root_manifest_class_id,
	 SUIT_DOWNGRADE_PREVENTION_DISABLED, SUIT_INDEPENDENT_UPDATE_DENIED,
	 /* signing_key_mask equal to -1 means signing with specified key is required
	  */
	 0x00000000, 0xFFFFFFFF},
	{&nordic_recovery_manifest_class_id, NULL, SUIT_DOWNGRADE_PREVENTION_DISABLED,
	 SUIT_INDEPENDENT_UPDATE_ALLOWED,
	 /* signing_key_mask equal to -1 means signing with specified key is required
	  */
	 0x00000000, 0xFFFFFFFF}};

static const manifest_config_t *
find_manifest_config(const suit_manifest_class_id_t *manifest_class_id)
{
	for (int i = 0; i < sizeof(supported_manifests) / sizeof(manifest_config_t); ++i) {
		const manifest_config_t *manifest_config = &supported_manifests[i];

		if (SUIT_PLAT_SUCCESS ==
		    suit_metadata_uuid_compare(manifest_config->manifest_class_id,
					       manifest_class_id)) {
			return manifest_config;
		}
	}
	return NULL;
}

#if defined(CONFIG_MBEDTLS) || defined(CONFIG_NRF_SECURITY)
static suit_plat_err_t load_keys(uint32_t *key_id)
{
	const uint8_t public_key[] = {
		0x04, /* POINT_CONVERSION_UNCOMPRESSED */
		0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08, 0xe7, 0x47, 0x09,
		0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33, 0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e,
		0x8c, 0x05, 0x2d, 0x02, 0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f,
		0xc1, 0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5, 0xda, 0x28,
		0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb, 0xef, 0x38, 0x55, 0x13};
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	/* Initialize PSA Crypto */
	psa_status = psa_crypto_init();
	if (psa_status != PSA_SUCCESS) {
		return SUIT_PLAT_ERR_CRASH;
	}

	/* Add keys */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

	psa_status = psa_import_key(&key_attributes, public_key, sizeof(public_key), key_id);
	if (psa_status != PSA_SUCCESS) {
		return SUIT_PLAT_ERR_CRASH;
	}

	return SUIT_PLAT_SUCCESS;
}
#endif /* CONFIG_MBEDTLS || CONFIG_NRF_SECURITY */

int suit_mci_supported_manifest_class_ids_get(suit_manifest_class_info_t *class_info, size_t *size)
{
	if (NULL == class_info || NULL == size) {
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t output_max_size = *size;
	size_t output_size = sizeof(supported_manifests) / sizeof(manifest_config_t);

	if (output_size > output_max_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	for (int i = 0; i < output_size; ++i) {
		class_info[i].class_id = supported_manifests[i].manifest_class_id;
		suit_mci_nordic_vendor_id_get(&class_info[i].vendor_id);
	}

	*size = output_size;
	return SUIT_PLAT_SUCCESS;
}

int suit_mci_invoke_order_get(const suit_manifest_class_id_t **class_id, size_t *size)
{
	if (NULL == class_id || NULL == size) {
		return SUIT_PLAT_ERR_INVAL;
	}
	size_t output_max_size = *size;

	/* In this implementation the only manifest which shall be utilized to initiate
	 *  system bringup is a root manifest
	 */
	size_t output_size = 1;

	if (output_size > output_max_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	switch (suit_execution_mode_get()) {
	case EXECUTION_MODE_INVOKE:
		class_id[0] = &nordic_root_manifest_class_id;
		*size = output_size;
		break;
	case EXECUTION_MODE_INVOKE_RECOVERY:
		class_id[0] = &nordic_recovery_manifest_class_id;
		*size = output_size;
		break;
	default:
		*size = 0;
		break;
	}

	return SUIT_PLAT_SUCCESS;
}

int suit_mci_downgrade_prevention_policy_get(const suit_manifest_class_id_t *class_id,
					     suit_downgrade_prevention_policy_t *policy)
{
	if (NULL == class_id || NULL == policy) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}
	*policy = manifest_config->downgrade_prevention_policy;
	return SUIT_PLAT_SUCCESS;
}

mci_err_t suit_mci_independent_update_policy_get(const suit_manifest_class_id_t *class_id,
						 suit_independent_updateability_policy_t *policy)
{
	if (NULL == class_id || NULL == policy) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}
	*policy = manifest_config->independent_update_policy;

	/* Override independent updateability policy in recovery scenarios:
	 * If the update candidate was delivered by the recovery firmware,
	 * it must not update the recovery firmware.
	 * Invoke modes included, so the recovery firmware may reject the incorrect
	 * update candidate before resetting the SoC.
	 */
	switch (suit_execution_mode_get()) {
	case EXECUTION_MODE_INVOKE_RECOVERY:
	case EXECUTION_MODE_INSTALL_RECOVERY:
	case EXECUTION_MODE_POST_INVOKE_RECOVERY:
		if (SUIT_PLAT_SUCCESS ==
		    suit_metadata_uuid_compare(&nordic_recovery_manifest_class_id, class_id)) {
			*policy = SUIT_INDEPENDENT_UPDATE_DENIED;
		}
		break;
	default:
		break;
	}

	return SUIT_PLAT_SUCCESS;
}

int suit_mci_manifest_class_id_validate(const suit_manifest_class_id_t *class_id)
{
	if (NULL == class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}
	return SUIT_PLAT_SUCCESS;
}

int suit_mci_signing_key_id_validate(const suit_manifest_class_id_t *class_id, uint32_t key_id)
{
	if (NULL == class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (key_id == 0)
	{
		return SUIT_PLAT_SUCCESS;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if ((manifest_config->signing_key_bits & manifest_config->signing_key_mask) !=
	    (key_id & manifest_config->signing_key_mask)) {
		return MCI_ERR_WRONGKEYID;
	}

	return SUIT_PLAT_SUCCESS;
}

int suit_mci_processor_start_rights_validate(const suit_manifest_class_id_t *class_id,
					     int processor_id)
{
	if (NULL == class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if (SUIT_PLAT_SUCCESS ==
	    suit_metadata_uuid_compare(&nordic_root_manifest_class_id, class_id)) {
		/* Root manifest - ability to start any cpu are intentionally blocked
		 */
		return MCI_ERR_NOACCESS;

	} else if (SUIT_PLAT_SUCCESS ==
		   suit_metadata_uuid_compare(&nordic_app_manifest_class_id, class_id)) {
		/* Application manifest. Use "0" as CPU ID in tests
		 */
		if (0 == processor_id) {
			return SUIT_PLAT_SUCCESS;
		}

		return MCI_ERR_NOACCESS;
	} else if (SUIT_PLAT_SUCCESS ==
		   suit_metadata_uuid_compare(&nordic_recovery_manifest_class_id, class_id)) {
		/* Application recovery manifest. Use "0" as CPU ID in tests
		 */
		if (0 == processor_id) {
			return SUIT_PLAT_SUCCESS;
		}

		return MCI_ERR_NOACCESS;
	}

	return MCI_ERR_NOACCESS;
}

int suit_mci_memory_access_rights_validate(const suit_manifest_class_id_t *class_id, void *address,
					   size_t mem_size)
{
	if (NULL == class_id || NULL == address || 0 == mem_size) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if (SUIT_PLAT_SUCCESS ==
	    suit_metadata_uuid_compare(&nordic_root_manifest_class_id, class_id)) {
		/* Root manifest - ability to operate on memory ranges intentionally blocked
		 */
		return MCI_ERR_NOACCESS;

	} else if (SUIT_PLAT_SUCCESS ==
		   suit_metadata_uuid_compare(&nordic_app_manifest_class_id, class_id)) {
		/* Application manifest - allow to overwrite any address
		 */
		return SUIT_PLAT_SUCCESS;
	} else if (SUIT_PLAT_SUCCESS ==
		   suit_metadata_uuid_compare(&nordic_recovery_manifest_class_id, class_id)) {
		/* Application recovery manifest - allow to overwrite any address
		 */
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}

int suit_mci_platform_specific_component_rights_validate(const suit_manifest_class_id_t *class_id,
							 int platform_specific_component_number)
{
	if (NULL == class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	return MCI_ERR_NOACCESS;
}

int suit_mci_vendor_id_for_manifest_class_id_get(const suit_manifest_class_id_t *class_id,
						 const suit_uuid_t **vendor_id)
{
	if (NULL == class_id || NULL == vendor_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	return suit_mci_nordic_vendor_id_get(vendor_id);
}

int suit_mci_manifest_parent_child_declaration_validate(
	const suit_manifest_class_id_t *parent_class_id,
	const suit_manifest_class_id_t *child_class_id)
{
	if (NULL == parent_class_id || NULL == child_class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_root_manifest_class_id, parent_class_id)) &&
	    (SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_app_manifest_class_id, child_class_id))) {
		return SUIT_PLAT_SUCCESS;
	}

	if ((SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_recovery_manifest_class_id, parent_class_id)) &&
	    (SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_app_manifest_class_id, child_class_id))) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t
suit_mci_manifest_process_dependency_validate(const suit_manifest_class_id_t *parent_class_id,
					      const suit_manifest_class_id_t *child_class_id)
{
	if (NULL == parent_class_id || NULL == child_class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_root_manifest_class_id, parent_class_id)) &&
	    (SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_app_manifest_class_id, child_class_id))) {
		return SUIT_PLAT_SUCCESS;
	}

	if ((SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_recovery_manifest_class_id, parent_class_id)) &&
	    (SUIT_PLAT_SUCCESS ==
	     suit_metadata_uuid_compare(&nordic_app_manifest_class_id, child_class_id))) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}

int suit_mci_init(void)
{
#if defined(CONFIG_MBEDTLS) || defined(CONFIG_NRF_SECURITY)
	if (supported_manifests[0].signing_key_bits == 0) {
		int ret = load_keys(&supported_manifests[0].signing_key_bits);

		if (ret != SUIT_PLAT_SUCCESS) {
			return ret;
		}

		supported_manifests[1].signing_key_bits = supported_manifests[0].signing_key_bits;
		supported_manifests[2].signing_key_bits = supported_manifests[0].signing_key_bits;
	}
#endif /* CONFIG_MBEDTLS || CONFIG_NRF_SECURITY */

	return SUIT_PLAT_SUCCESS;
}
