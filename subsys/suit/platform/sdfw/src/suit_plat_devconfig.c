/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit.h>
#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_storage.h>
#include <suit_mci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(plat_devconfig, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_sequence_completed(enum suit_command_sequence seq_name,
				 struct zcbor_string *manifest_component_id,
				 const uint8_t *envelope_str, size_t envelope_len)
{
	suit_manifest_class_id_t *class_id = NULL;
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	if ((manifest_component_id == NULL) || (manifest_component_id->value == NULL)) {
		LOG_ERR("Manifest class ID not specified");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to decode manifest class ID");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	err = suit_mci_manifest_class_id_validate(class_id);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to validate manifest class ID (MCI err: %d)", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (seq_name == SUIT_SEQ_INSTALL) {
		err = suit_storage_install_envelope(class_id, (uint8_t *)envelope_str,
						    envelope_len);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to save envelope (platform err: %d)", err);
		} else {
			LOG_DBG("Envelope saved");
		}
	}

	return suit_plat_err_to_processor_err_convert(err);
}

int suit_plat_authorize_sequence_num(enum suit_command_sequence seq_name,
				     struct zcbor_string *manifest_component_id,
				     unsigned int seq_num)
{
	const uint8_t *envelope_addr;
	size_t envelope_size;
	uint32_t current_seq_num;
	suit_manifest_class_id_t *class_id = NULL;
	suit_downgrade_prevention_policy_t policy;
	suit_plat_err_t ret = SUIT_PLAT_ERR_CRASH;

	if ((manifest_component_id == NULL) || (manifest_component_id->value == NULL)) {
		LOG_ERR("Manifest class ID not specified");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to decode manifest class ID");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	ret = suit_mci_manifest_class_id_validate(class_id);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unsupported manifest class ID (MCI err: %d)", ret);
		return SUIT_ERR_AUTHENTICATION;
	}

	ret = suit_storage_installed_envelope_get(class_id, &envelope_addr, &envelope_size);
	if ((ret == SUIT_PLAT_ERR_NOT_FOUND) || (ret == SUIT_PLAT_ERR_OUT_OF_BOUNDS)) {
		LOG_ERR("Unable to find a role or slot for envelope (ret: %d)", ret);
		return SUIT_ERR_AUTHENTICATION;
	}

	if (ret != SUIT_PLAT_SUCCESS) {
		if ((seq_name == SUIT_SEQ_VALIDATE) || (seq_name == SUIT_SEQ_LOAD) ||
		    (seq_name == SUIT_SEQ_INVOKE)) {
			/* It is not allowed to boot from update candidate. */
			LOG_ERR("Unable to get installed envelope (ret: %d)", ret);
			return SUIT_ERR_AUTHENTICATION;
		}

		LOG_DBG("Envelope with given class ID not installed. Continue update.");
		return SUIT_SUCCESS;
	}

	ret = suit_processor_get_manifest_metadata(envelope_addr, envelope_size, false, NULL, NULL,
						   NULL, &current_seq_num);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Unable to read manifest metadata (ret: %d)", ret);
		return SUIT_ERR_AUTHENTICATION;
	}

	if ((seq_name == SUIT_SEQ_VALIDATE) || (seq_name == SUIT_SEQ_LOAD) ||
	    (seq_name == SUIT_SEQ_INVOKE)) {
		if (current_seq_num == seq_num) {
			/* Allow to use installed manifest during boot procedure. */
			LOG_DBG("Manifest sequence number %d authorized to boot", seq_num);
			return SUIT_SUCCESS;
		}

		/* It is not allowed to boot from update candidate. */
		LOG_ERR("Manifest sequence number %d unauthorized to boot (current: %d)",
			seq_num, current_seq_num);
		return SUIT_ERR_AUTHENTICATION;
	}

	ret = suit_mci_downgrade_prevention_policy_get(class_id, &policy);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to get downgrade prevention policy (MCI err: %d)", ret);
		return SUIT_ERR_AUTHENTICATION;
	}

	if (policy == SUIT_DOWNGRADE_PREVENTION_DISABLED) {
		LOG_DBG("Manifest sequence number %d for sequence %d authorized (current: %d, "
			"policy: %d)",
			seq_num, seq_name, current_seq_num, policy);
		return SUIT_SUCCESS;
	} else if (policy != SUIT_DOWNGRADE_PREVENTION_ENABLED) {
		LOG_ERR("Unsupported downgrade prevention policy value: %d", policy);
		return SUIT_ERR_AUTHENTICATION;
	} else if (current_seq_num <= seq_num) {
		return SUIT_SUCCESS;
	}

	LOG_ERR("Manifest sequence number %d for sequence %d unauthorized (current: %d, policy: "
		"%d)",
		seq_num, seq_name, current_seq_num, policy);

	return SUIT_ERR_AUTHENTICATION;
}
