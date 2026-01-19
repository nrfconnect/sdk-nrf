/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_pf_oob_msg_ble_cs, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fhn/pf/ble_cs.h>

#include "oob_msg_defs.h"

bool bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
	uint8_t bm, enum bt_fast_pair_fhn_pf_ble_cs_security_level level)
{
	__ASSERT_NO_MSG(level <= BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR);

	return ((bm & BIT(level)) > 0);
}

void bt_fast_pair_fhn_pf_ble_cs_security_level_bm_write(
	uint8_t *bm, enum bt_fast_pair_fhn_pf_ble_cs_security_level level, bool value)
{
	__ASSERT_NO_MSG(bm);
	__ASSERT_NO_MSG(level <= BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR);

	WRITE_BIT(*bm, level, value);
}

int bt_fast_pair_fhn_pf_ble_cs_ranging_capability_encode(
	const struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability *capability_desc,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payload)
{
	uint8_t pos = 0;
	static const uint8_t expected_data_len =
		(OOB_MSG_RANGING_CAPABILITY_RSP_BLE_CS_PAYLOAD_LEN - OOB_MSG_RANGING_TECH_HDR_LEN);

	if (!capability_desc || !capability_payload) {
		LOG_ERR("Precision Finding: BLE CS: encoder: input parameters cannot be NULL");
		return -EINVAL;
	}

	if (!capability_payload->data || (capability_payload->data_len < expected_data_len)) {
		LOG_ERR("Precision Finding: BLE CS: encoder: buffer invalid (%p) or "
			"too small (%u) to contain ranging capability",
			capability_payload->data, capability_payload->data_len);
		return -ENOMEM;
	}


	capability_payload->id = BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS;

	capability_payload->data_len = expected_data_len;
	capability_payload->data[pos++] = capability_desc->supported_security_level_bm;

	/* Encode address in big-endian order. */
	for (int8_t i = sizeof(capability_desc->addr.val) - 1; i >= 0; i--) {
		capability_payload->data[pos++] = capability_desc->addr.val[i];
	}

	__ASSERT_NO_MSG(pos == expected_data_len);

	return 0;
}

int bt_fast_pair_fhn_pf_ble_cs_ranging_config_decode(
	const struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payload,
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_config *config_desc)
{
	uint8_t pos = 0;
	static const uint8_t expected_data_len =
		(OOB_MSG_RANGING_CONFIG_REQ_BLE_CS_PAYLOAD_LEN - OOB_MSG_RANGING_TECH_HDR_LEN);

	if (!config_desc || !config_payload) {
		LOG_ERR("Precision Finding: BLE CS: decoder: input parameters cannot be NULL");
		return -EINVAL;
	}

	if (config_payload->id != BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS) {
		LOG_ERR("Precision Finding: BLE CS: decoder: invalid ranging tech ID: %d",
			config_payload->id);
		return -EINVAL;
	}

	if (config_payload->data_len != expected_data_len) {
		LOG_ERR("Precision Finding: BLE CS: decoder: invalid data length (%d != %d)",
			config_payload->data_len, expected_data_len);
		return -EINVAL;
	}

	config_desc->selected_security_level = config_payload->data[pos++];
	if (config_desc->selected_security_level > BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR) {
		LOG_ERR("Precision Finding: BLE CS: decoder: invalid value for the security level "
			"(%d)", config_desc->selected_security_level);
		return -EINVAL;
	}

	/* Decode address that is encoded in the big-endian order. */
	for (int8_t i = sizeof(config_desc->addr.val) - 1; i >= 0; i--) {
		config_desc->addr.val[i] = config_payload->data[pos++];
	}

	__ASSERT_NO_MSG(pos == expected_data_len);

	return 0;
}
