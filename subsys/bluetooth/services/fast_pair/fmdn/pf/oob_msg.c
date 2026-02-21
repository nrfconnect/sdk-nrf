/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_pf_oob_msg, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fhn/pf/pf.h>

#include "fp_fmdn_pf_oob_msg.h"

#include "oob_msg_defs.h"

/* The minimum value of the message version. */
#define MSG_VERSION_MIN (0x01)

/* Validate if the internal API define for denoting maximum number of ranging technology
 * payloads is aligned with the number of ranging technology enumerator from the public API
 */
BUILD_ASSERT(FP_FMDN_PF_OOB_MSG_RANGING_TECH_PAYLOAD_MAX_NUM ==
	(BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI + 1));

static int pf_oob_msg_ranging_capability_req_decode(struct net_buf_simple *msg_buf,
						    struct fp_fmdn_pf_oob_msg *msg_desc)
{
	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_REQ);

	if (net_buf_simple_max_len(msg_buf) != OOB_MSG_RANGING_CAPABILITY_REQ_PAYLOAD_LEN) {
		LOG_WRN("Precision Finding: OOB message: Ranging Capability Request: "
			"invalid payload length");

		return -EINVAL;
	}

	msg_desc->ranging_capability_req.ranging_tech_bm = net_buf_simple_pull_le16(msg_buf);
	if (msg_desc->ranging_capability_req.ranging_tech_bm == 0) {
		LOG_WRN("Precision Finding: OOB message: Ranging Capability Request: "
			"ranging technologies bitmask cannot be equal to zero");

		return -EINVAL;
	}

	return 0;
}

static int pf_oob_msg_ranging_config_payload_decode(
	struct net_buf_simple *msg_buf,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload)
{
	uint8_t payload_len;

	__ASSERT_NO_MSG(tech_payload);

	if (net_buf_simple_max_len(msg_buf) < OOB_MSG_RANGING_TECH_HDR_LEN) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"Configuration Payload: invalid length");

		return -EINVAL;
	}

	tech_payload->id = net_buf_simple_pull_u8(msg_buf);
	if (tech_payload->id > BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"Configuration Payload: unknown ranging technology id: %d",
			tech_payload->id);

		return -EINVAL;
	}

	payload_len = net_buf_simple_pull_u8(msg_buf);
	if ((payload_len < OOB_MSG_RANGING_TECH_HDR_LEN) ||
	    (payload_len > (net_buf_simple_max_len(msg_buf) + OOB_MSG_RANGING_TECH_HDR_LEN))) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"Configuration Payload: invalid size field: %u", payload_len);

		return -EINVAL;
	}
	payload_len -= OOB_MSG_RANGING_TECH_HDR_LEN;

	tech_payload->data = (payload_len == 0) ?
		NULL : net_buf_simple_pull_mem(msg_buf, payload_len);
	tech_payload->data_len = payload_len;

	return 0;
}

static int pf_oob_msg_ranging_config_req_decode(struct net_buf_simple *msg_buf,
						struct fp_fmdn_pf_oob_msg *msg_desc)
{
	int err;
	uint8_t config_payload_num;
	uint16_t rfu;

	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_REQ);

	if (!msg_desc->ranging_config_req.config_payloads ||
	    !msg_desc->ranging_config_req.config_payload_num) {
		LOG_ERR("Precision Finding: OOB message: Ranging Configuration: "
			"missing configuration of the configuration payloads");

		return -EINVAL;
	}

	if (net_buf_simple_max_len(msg_buf) <
		(OOB_MSG_RANGING_CONFIG_REQ_BM_LEN + OOB_MSG_RANGING_CONFIG_REQ_RFU_LEN)) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"invalid initial payload length");

		return -EINVAL;
	}

	msg_desc->ranging_config_req.ranging_tech_bm = net_buf_simple_pull_le16(msg_buf);
	rfu = net_buf_simple_pull_le16(msg_buf);

	if (msg_desc->ranging_config_req.ranging_tech_bm == 0) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"ranging technologies bitmask cannot be equal to zero");

		return -EINVAL;
	}

	if (msg_desc->ranging_config_req.ranging_tech_bm != rfu) {
		LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
			"RFU does not match the ranging technologies bitmask (0x%02X != 0x%02X)",
			msg_desc->ranging_config_req.ranging_tech_bm, rfu);

		return -EINVAL;
	}

	config_payload_num = 0;
	do {
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload;

		if (msg_desc->ranging_config_req.config_payload_num == 0) {
			LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
				"the number of configuration payload descriptors is insufficient");

			return -EINVAL;
		}

		tech_payload = &msg_desc->ranging_config_req.config_payloads[config_payload_num];
		err = pf_oob_msg_ranging_config_payload_decode(msg_buf, tech_payload);
		if (err) {
			return -EINVAL;
		}

		if (!bt_fast_pair_fhn_pf_ranging_tech_bm_check(rfu, tech_payload->id)) {
			LOG_WRN("Precision Finding: OOB message: Ranging Configuration: "
				"Ranging Technology bitmask (0x%04X) does not match configuration "
				"payload for technology ID: %d or the payload is duplicated",
				msg_desc->ranging_config_req.ranging_tech_bm,
				tech_payload->id);

			return -EINVAL;
		}
		bt_fast_pair_fhn_pf_ranging_tech_bm_write(&rfu, tech_payload->id, 0);

		msg_desc->ranging_config_req.config_payload_num--;
		config_payload_num++;
	} while (net_buf_simple_max_len(msg_buf) != 0);

	__ASSERT_NO_MSG(config_payload_num <= FP_FMDN_PF_OOB_MSG_RANGING_TECH_PAYLOAD_MAX_NUM);
	msg_desc->ranging_config_req.config_payload_num = config_payload_num;

	return 0;
}

static int pf_oob_msg_stop_ranging_req_decode(struct net_buf_simple *msg_buf,
					      struct fp_fmdn_pf_oob_msg *msg_desc)
{
	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_REQ);

	if (net_buf_simple_max_len(msg_buf) != OOB_MSG_STOP_RANGING_REQ_PAYLOAD_LEN) {
		LOG_WRN("Precision Finding: OOB message: Stop Ranging: "
			"invalid payload length");

		return -EINVAL;
	}

	msg_desc->stop_ranging_req.ranging_tech_bm = net_buf_simple_pull_le16(msg_buf);
	if (msg_desc->stop_ranging_req.ranging_tech_bm == 0) {
		LOG_WRN("Precision Finding: OOB message: Stop Ranging: "
			"ranging technologies bitmask cannot be equal to zero");

		return -EINVAL;
	}

	return 0;
}

int fp_fmdn_pf_oob_msg_decode(struct net_buf_simple *msg_buf,
			      struct fp_fmdn_pf_oob_msg *msg_desc)
{
	__ASSERT_NO_MSG(msg_desc);
	__ASSERT_NO_MSG(msg_buf);

	if (net_buf_simple_max_len(msg_buf) < OOB_MSG_MSG_ID_HDR_LEN) {
		LOG_WRN("Precision Finding: OOB message: packet does not contain header");

		return -EINVAL;
	}

	msg_desc->version = net_buf_simple_pull_u8(msg_buf);
	if (msg_desc->version < MSG_VERSION_MIN) {
		LOG_WRN("Precision Finding: OOB message: invalid version");

		return -EINVAL;
	}

	msg_desc->id = net_buf_simple_pull_u8(msg_buf);
	switch (msg_desc->id) {
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_REQ:
		return pf_oob_msg_ranging_capability_req_decode(msg_buf, msg_desc);
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_REQ:
		return pf_oob_msg_ranging_config_req_decode(msg_buf, msg_desc);
	case FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_REQ:
		return pf_oob_msg_stop_ranging_req_decode(msg_buf, msg_desc);
	default:
		LOG_WRN("Precision Finding: OOB message: invalid message ID from Initiator");

		return -EINVAL;
	}
}

static bool pf_oob_msg_capability_payload_len_validate(
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *payload)
{
	uint8_t payload_len = OOB_MSG_RANGING_TECH_HDR_LEN;

	__ASSERT_NO_MSG(payload);

	payload_len += payload->data_len;

	switch (payload->id) {
	case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_UWB:
		return (payload_len == OOB_MSG_RANGING_CAPABILITY_RSP_UWB_PAYLOAD_LEN);
	case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS:
		return (payload_len == OOB_MSG_RANGING_CAPABILITY_RSP_BLE_CS_PAYLOAD_LEN);
	case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_WIFI_NAN_RTT:
		return (payload_len == OOB_MSG_RANGING_CAPABILITY_RSP_WIFI_NAN_PAYLOAD_LEN);
	case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI:
		return (payload_len == OOB_MSG_RANGING_CAPABILITY_RSP_BLE_RSSI_PAYLOAD_LEN);
	default:
		/* Unknown ranging technology ID. */
		__ASSERT_NO_MSG(0);
	}

	return false;
}

static uint8_t pf_oob_msg_len_get(const struct fp_fmdn_pf_oob_msg *msg_desc)
{
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *payload;
	uint8_t capability_payload_num;
	uint8_t len = OOB_MSG_MSG_ID_HDR_LEN;

	__ASSERT_NO_MSG(msg_desc);

	switch (msg_desc->id) {
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP:
		len += OOB_MSG_RANGING_CAPABILITY_RSP_BM_LEN;
		capability_payload_num = (msg_desc->ranging_capability_rsp.ranging_tech_bm != 0) ?
			msg_desc->ranging_capability_rsp.capability_payload_num : 0;

		for (uint8_t i; i < msg_desc->ranging_capability_rsp.capability_payload_num; i++) {
			payload = &msg_desc->ranging_capability_rsp.capability_payloads[i];

			__ASSERT_NO_MSG(pf_oob_msg_capability_payload_len_validate(payload));

			len += OOB_MSG_RANGING_TECH_HDR_LEN;
			len += payload->data_len;
		}
		break;
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP:
		len += OOB_MSG_RANGING_CONFIG_RSP_PAYLOAD_LEN;
		break;
	case FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP:
		len += OOB_MSG_STOP_RANGING_RSP_PAYLOAD_LEN;
		break;
	default:
		/* Unsupported or unknown message ID. */
		__ASSERT_NO_MSG(0);
	}

	return len;
}

static int pf_oob_msg_ranging_capability_rsp_encode(const struct fp_fmdn_pf_oob_msg *msg_desc,
						    struct net_buf_simple *msg_buf)
{
	uint8_t msg_buf_min_len;
	uint8_t capability_payload_num = msg_desc->ranging_capability_rsp.capability_payload_num;
	uint16_t ranging_tech_bm = msg_desc->ranging_capability_rsp.ranging_tech_bm;

	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP);

	/* Ignore the capability payload arguments if the ranging technology bitmask is zero. */
	if (msg_desc->ranging_capability_rsp.ranging_tech_bm == 0) {
		capability_payload_num = 0;
	}

	/* Perform extensive validation of the message descriptor. */
	if (msg_desc->ranging_capability_rsp.ranging_tech_bm != 0 &&
	    (!msg_desc->ranging_capability_rsp.capability_payloads || !capability_payload_num)) {
		LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
			"missing configuration of the capability payloads ");

		return -EINVAL;
	}

	for (uint8_t i = 0; i < capability_payload_num; i++) {
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload =
			&msg_desc->ranging_capability_rsp.capability_payloads[i];

		if (tech_payload->id > BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI) {
			LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
				"invalid unknown technology ID: %d", tech_payload->id);

			return -EINVAL;
		}

		if (!pf_oob_msg_capability_payload_len_validate(tech_payload)) {
			LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
				"invalid length of capability payload for technology ID: %d",
				tech_payload->id);

			return -EINVAL;
		}

		if (!bt_fast_pair_fhn_pf_ranging_tech_bm_check(
			ranging_tech_bm, tech_payload->id)) {
			LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
				"Ranging Technology bitmask (0x%04X) does not match capability "
				"payload for technology ID: %d or the payload is duplicated",
				msg_desc->ranging_capability_rsp.ranging_tech_bm,
				tech_payload->id);

			return -EINVAL;
		}

		bt_fast_pair_fhn_pf_ranging_tech_bm_write(&ranging_tech_bm, tech_payload->id, 0);
	}

	if (ranging_tech_bm) {
		LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
			"Ranging Technology bitmask uses excessive bits (0x%04X) that do not "
			"match capability payloads", ranging_tech_bm);

		return -EINVAL;
	}

	/* Validate the provided buffer. */
	msg_buf_min_len = pf_oob_msg_len_get(msg_desc);
	if (net_buf_simple_max_len(msg_buf) < msg_buf_min_len) {
		LOG_ERR("Precision Finding: OOB message: Ranging Capability Response: "
			"response buffer too small: %d < %d", net_buf_simple_max_len(msg_buf),
			msg_buf_min_len);

		return -ENOMEM;
	}

	/* Encode the payload. */
	net_buf_simple_add_u8(msg_buf, msg_desc->version);
	net_buf_simple_add_u8(msg_buf, msg_desc->id);

	net_buf_simple_add_le16(msg_buf, msg_desc->ranging_capability_rsp.ranging_tech_bm);
	for (uint8_t i = 0; i < capability_payload_num; i++) {
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload =
			&msg_desc->ranging_capability_rsp.capability_payloads[i];
		uint8_t tech_payload_len = OOB_MSG_RANGING_TECH_HDR_LEN + tech_payload->data_len;

		net_buf_simple_add_u8(msg_buf, tech_payload->id);
		net_buf_simple_add_u8(msg_buf, tech_payload_len);
		net_buf_simple_add_mem(msg_buf, tech_payload->data, tech_payload->data_len);
	}

	return 0;
}

static int pf_oob_msg_ranging_config_rsp_encode(const struct fp_fmdn_pf_oob_msg *msg_desc,
						struct net_buf_simple *msg_buf)
{
	uint8_t msg_buf_min_len;

	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP);

	/* Validate the provided buffer. */
	msg_buf_min_len = pf_oob_msg_len_get(msg_desc);
	if (net_buf_simple_max_len(msg_buf) < msg_buf_min_len) {
		LOG_ERR("Precision Finding: OOB message: Ranging Configuration Response: "
			"response buffer too small: %d < %d", net_buf_simple_max_len(msg_buf),
			msg_buf_min_len);

		return -ENOMEM;
	}

	/* Encode the payload. */
	net_buf_simple_add_u8(msg_buf, msg_desc->version);
	net_buf_simple_add_u8(msg_buf, msg_desc->id);
	net_buf_simple_add_le16(msg_buf, msg_desc->ranging_config_rsp.ranging_tech_bm);

	return 0;
}

static int pf_oob_msg_stop_ranging_rsp_encode(const struct fp_fmdn_pf_oob_msg *msg_desc,
					      struct net_buf_simple *msg_buf)
{
	uint8_t msg_buf_min_len;

	__ASSERT_NO_MSG(msg_desc->id == FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP);

	/* Validate the provided buffer. */
	msg_buf_min_len = pf_oob_msg_len_get(msg_desc);
	if (net_buf_simple_max_len(msg_buf) < msg_buf_min_len) {
		LOG_ERR("Precision Finding: OOB message: Stop Ranging Response: "
			"response buffer too small: %d < %d", net_buf_simple_max_len(msg_buf),
			msg_buf_min_len);

		return -ENOMEM;
	}

	/* Encode the payload. */
	net_buf_simple_add_u8(msg_buf, msg_desc->version);
	net_buf_simple_add_u8(msg_buf, msg_desc->id);
	net_buf_simple_add_le16(msg_buf, msg_desc->stop_ranging_rsp.ranging_tech_bm);

	return 0;
}

int fp_fmdn_pf_oob_msg_encode(const struct fp_fmdn_pf_oob_msg *msg_desc,
			      struct net_buf_simple *msg_buf)
{
	__ASSERT_NO_MSG(msg_desc);
	__ASSERT_NO_MSG(msg_buf);

	switch (msg_desc->id) {
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP:
		return pf_oob_msg_ranging_capability_rsp_encode(msg_desc, msg_buf);
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP:
		return pf_oob_msg_ranging_config_rsp_encode(msg_desc, msg_buf);
	case FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP:
		return pf_oob_msg_stop_ranging_rsp_encode(msg_desc, msg_buf);
	default:
		/* Unsupported or unknown message ID. */
		__ASSERT_NO_MSG(0);
	}

	return 0;
}

uint8_t fp_fmdn_pf_oob_msg_max_len_get(enum fp_fmdn_pf_oob_msg_id msg_id)
{
	uint8_t len = OOB_MSG_MSG_ID_HDR_LEN;

	switch (msg_id) {
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP:
		len += OOB_MSG_RANGING_CAPABILITY_RSP_PAYLOAD_MAX_LEN;
		break;
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP:
		len += OOB_MSG_RANGING_CONFIG_RSP_PAYLOAD_LEN;
		break;
	case FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP:
		len += OOB_MSG_STOP_RANGING_RSP_PAYLOAD_LEN;
		break;
	default:
		/* Unsupported or unknown message ID. */
		__ASSERT_NO_MSG(0);
	}

	return len;
}
