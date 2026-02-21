/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>

#include <bluetooth/services/fast_pair/fhn/pf/pf.h>
#include <bluetooth/services/fast_pair/fhn/pf/ble_cs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/* Timeout in seconds for the ephemeral state of the Bluetooth CS ranging session. All states
 * except the MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED state are considered ephemeral.
 */
#define RANGING_TECH_BLE_CS_SESSION_STATE_EPHEMERAL_TIMEOUT (K_SECONDS(10))

/* Settings strings used by this ranging module. */
#define SETTINGS_RANGING_SUBTREE_NAME "ranging"
#define SETTINGS_RANGING_PAIRING_ADDR_ADDR_NAME "pairing_addr"
#define SETTINGS_RANGING_PAIRING_ADDR_ADDR_FULL_NAME \
	(SETTINGS_RANGING_SUBTREE_NAME "/" SETTINGS_RANGING_PAIRING_ADDR_ADDR_NAME)

/* States of the management connection. */
enum mgmt_conn_state {
	/* Initial state after the connection is established. */
	MGMT_CONN_STATE_INIT,

	/* Ranging Capability Request message has been received. */
	MGMT_CONN_STATE_RANGING_CAPABILITY_REQUEST_RECEIVED,

	/* Ranging Capability Response message has been sent. */
	MGMT_CONN_STATE_RANGING_CAPABILITY_RESPONSE_SENT,

	/* Ranging Configuration message has been received. */
	MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED,

	/* Ranging Configuration Response message has been sent. */
	MGMT_CONN_STATE_RANGING_CONFIG_RESPONSE_SENT,

	/* Stop Ranging message has been received. */
	MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED,

	/* Stop Ranging Response message has been sent. */
	MGMT_CONN_STATE_STOP_RANGING_RESPONSE_SENT,
};

/* States of the Bluetooth CS ranging technology in the management connection. */
enum mgmt_conn_ble_cs_session_state {
	/* The Bluetooth CS ranging session is disabled. */
	MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED,

	/* The Bluetooth CS ranging session is enabled. */
	MGMT_CONN_BLE_CS_SESSION_STATE_ENABLED,

	/* The Bluetooth CS ranging session has been terminated with an error. */
	MGMT_CONN_BLE_CS_SESSION_STATE_ERROR,
};

/* Representation of the connection object that is used to manage ranging process. */
struct mgmt_conn {
	/* Bluetooth connection object used to manage ranging. */
	struct bt_conn *conn;

	/* State of the management connection. */
	enum mgmt_conn_state state;

	/* Bitmask indicating the ranging technologies used in the current ranging session. */
	uint16_t ranging_tech_bm;

	/* Bitmask indicating the ranging technologies that stopped successfully in the current
	 * ranging session. Used only in the MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED state.
	 */
	uint16_t stop_ranging_tech_bm;

	/* Context relevant to the Bluetooth Channel Sounding (CS) ranging technology. */
	struct {
		/* State of the Bluetooth CS ranging session. */
		enum mgmt_conn_ble_cs_session_state state;

		/* Timeout work for the Bluetooth CS ranging session. */
		struct k_work_delayable timeout_work;

		/* Indicates if the ranging response message is ready to be sent. */
		bool is_rsp_ready;

		union {
			/* Payload of the BLE CS ranging capability response message. */
			uint8_t capability_payload[
				sizeof(struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability)];

			/* Descriptor of the BLE CS technology configuration payload. */
			struct bt_fast_pair_fhn_pf_ble_cs_ranging_config config_desc;
		};
	} ble_cs;
};

static struct mgmt_conn mgmt_conns[CONFIG_BT_MAX_CONN];
static bt_addr_t pairing_addr;

static struct mgmt_conn *mgmt_conn_get(struct bt_conn *conn)
{
	uint8_t index;

	index = bt_conn_index(conn);
	__ASSERT_NO_MSG(index < ARRAY_SIZE(mgmt_conns));

	return &mgmt_conns[index];
}

static const char *ranging_tech_str_get(enum bt_fast_pair_fhn_pf_ranging_tech_id tech_id)
{
	static const char * const ranging_tech_names[] = {
		[BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_UWB] = "UWB",
		[BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS] = "BLE CS",
		[BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_WIFI_NAN_RTT] = "WiFi NAN RTT",
		[BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI] = "BLE RSSI",
	};

	__ASSERT_NO_MSG((tech_id <= BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI) &&
			ranging_tech_names[tech_id]);

	return ranging_tech_names[tech_id];
}

static void ranging_tech_bm_print(uint16_t bm)
{
	LOG_INF("\t%s=(%s), %s=(%s), %s=(%s), %s=(%s)",
		ranging_tech_str_get(BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_UWB),
		bt_fast_pair_fhn_pf_ranging_tech_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_UWB) ? "X" : " ",
		ranging_tech_str_get(BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS),
		bt_fast_pair_fhn_pf_ranging_tech_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS) ? "X" : " ",
		ranging_tech_str_get(BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_WIFI_NAN_RTT),
		bt_fast_pair_fhn_pf_ranging_tech_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_WIFI_NAN_RTT) ? "X" : " ",
		ranging_tech_str_get(BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI),
		bt_fast_pair_fhn_pf_ranging_tech_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI) ? "X" : " ");
}

static const char *ranging_tech_ble_cs_security_level_str_get(
	enum bt_fast_pair_fhn_pf_ble_cs_security_level level)
{
	static const char * const ranging_tech_ble_cs_security_level_names[] = {
		[BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_UNKNOWN] = "Level unknown",
		[BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_ONE]     = "Level 1",
		[BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_TWO]     = "Level 2",
		[BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_THREE]   = "Level 3",
		[BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR]    = "Level 4",
	};

	__ASSERT_NO_MSG((level <= BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR) &&
			ranging_tech_ble_cs_security_level_names[level]);

	return ranging_tech_ble_cs_security_level_names[level];
}

static bool ranging_rsp_ready_check(struct mgmt_conn *mgmt_conn)
{
	__ASSERT_NO_MSG(mgmt_conn);
	__ASSERT_NO_MSG(mgmt_conn->state == MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED ||
			mgmt_conn->state == MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED);

	return mgmt_conn->ble_cs.is_rsp_ready;
}

static void ranging_rsp_ready_clear(struct mgmt_conn *mgmt_conn)
{
	__ASSERT_NO_MSG(mgmt_conn);
	__ASSERT_NO_MSG(mgmt_conn->state == MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED ||
		mgmt_conn->state == MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED);

	mgmt_conn->ble_cs.is_rsp_ready = false;
}

static void ranging_config_rsp_send(struct mgmt_conn *mgmt_conn)
{
	int err;

	__ASSERT_NO_MSG(mgmt_conn);

	err = bt_fast_pair_fhn_pf_ranging_config_response_send(mgmt_conn->conn,
							       mgmt_conn->ranging_tech_bm);
	if (err) {
		LOG_ERR("Ranging: Ranging Configuration Response failed to be sent: %d", err);
		return;
	}

	LOG_INF("Ranging: Ranging Configuration Response message to %p", mgmt_conn->conn);
	LOG_INF("\tRanging technologies configuration set successfully bitfield 0x%04X:",
		mgmt_conn->ranging_tech_bm);
	ranging_tech_bm_print(mgmt_conn->ranging_tech_bm);

	/* Perform the state transition. */
	mgmt_conn->state = MGMT_CONN_STATE_RANGING_CONFIG_RESPONSE_SENT;
}

static void stop_ranging_rsp_send(struct mgmt_conn *mgmt_conn)
{
	int err;

	__ASSERT_NO_MSG(mgmt_conn);

	err = bt_fast_pair_fhn_pf_stop_ranging_response_send(mgmt_conn->conn,
							     mgmt_conn->stop_ranging_tech_bm);
	if (err) {
		LOG_ERR("Ranging: Stop Ranging Response failed to be sent: %d", err);
		return;
	}

	LOG_INF("Ranging: Stop Ranging Response message to %p", mgmt_conn->conn);
	LOG_INF("\tRanging technologies stopped successfully bitfield 0x%04X:",
		mgmt_conn->ranging_tech_bm);
	ranging_tech_bm_print(mgmt_conn->ranging_tech_bm);

	/* Perform the state transition. */
	mgmt_conn->state = MGMT_CONN_STATE_STOP_RANGING_RESPONSE_SENT;
	mgmt_conn->ranging_tech_bm = mgmt_conn->ranging_tech_bm & ~mgmt_conn->stop_ranging_tech_bm;
	mgmt_conn->stop_ranging_tech_bm = 0;
}

static void ranging_rsp_ready_check_and_send(struct mgmt_conn *mgmt_conn)
{
	__ASSERT_NO_MSG(mgmt_conn);

	/* Check if the local device is ready to send the response. */
	if (!ranging_rsp_ready_check(mgmt_conn)) {
		return;
	}
	ranging_rsp_ready_clear(mgmt_conn);

	switch (mgmt_conn->state) {
	case MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED:
		ranging_config_rsp_send(mgmt_conn);
		break;
	case MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED:
		stop_ranging_rsp_send(mgmt_conn);
		break;
	default:
		__ASSERT_NO_MSG(0);
	}
}

static void ranging_tech_ble_cs_rsp_ready_set(struct mgmt_conn *mgmt_conn, bool cs_op_successful)
{
	__ASSERT_NO_MSG(mgmt_conn);

	if (!bt_fast_pair_fhn_pf_ranging_tech_bm_check(
		mgmt_conn->ranging_tech_bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS)) {
		LOG_WRN("Ranging: BLE CS: attempting to change the CS bit in ranging tech bitmask "
			"in the wrong state: 0x%04X", mgmt_conn->ranging_tech_bm);
		return;
	}

	mgmt_conn->ble_cs.is_rsp_ready = true;

	switch (mgmt_conn->state) {
	case MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED:
		bt_fast_pair_fhn_pf_ranging_tech_bm_write(
			&mgmt_conn->ranging_tech_bm,
			BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS,
			cs_op_successful);
		break;
	case MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED:
		bt_fast_pair_fhn_pf_ranging_tech_bm_write(
			&mgmt_conn->stop_ranging_tech_bm,
			BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS,
			cs_op_successful);
		break;
	default:
		__ASSERT_NO_MSG(0);
	}

	ranging_rsp_ready_check_and_send(mgmt_conn);
}

static void ranging_tech_ble_cs_session_state_set(struct mgmt_conn *mgmt_conn,
						  enum mgmt_conn_ble_cs_session_state state)
{
	__ASSERT_NO_MSG(mgmt_conn);

	mgmt_conn->ble_cs.state = state;
	if (mgmt_conn->ble_cs.state != MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED) {
		k_work_reschedule(&mgmt_conn->ble_cs.timeout_work,
				  RANGING_TECH_BLE_CS_SESSION_STATE_EPHEMERAL_TIMEOUT);
	} else {
		k_work_cancel_delayable(&mgmt_conn->ble_cs.timeout_work);
	}

	/* The CS session establishment procedure has been completed after
	 * the Ranging Configuration message was received.
	 */
	if (mgmt_conn->state == MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED) {
		ranging_tech_ble_cs_rsp_ready_set(
			mgmt_conn, (state == MGMT_CONN_BLE_CS_SESSION_STATE_ENABLED));
	}

	/* The CS session teardown procedure has been completed after
	 * the Stop Ranging message was received.
	 */
	if (mgmt_conn->state == MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED) {
		ranging_tech_ble_cs_rsp_ready_set(
			mgmt_conn, (state == MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED));
	}
}

static void ranging_tech_ble_cs_timeout_work_handle(struct k_work *work)
{
	struct k_work_delayable *work_d = k_work_delayable_from_work(work);
	struct mgmt_conn *mgmt_conn = CONTAINER_OF(work_d, struct mgmt_conn, ble_cs.timeout_work);

	ranging_tech_ble_cs_session_state_set(mgmt_conn, MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED);
}

static void ranging_tech_ble_cs_bond_find(const struct bt_bond_info *info,
					  void *user_data)
{
	struct bt_conn_info *conn_info = user_data;

	if (conn_info->role) {
		return;
	}

	if (memcmp(&info->addr.a, &conn_info->le.dst->a, sizeof(info->addr.a)) == 0) {
		/* The Bluetooth bond is present in the system. */
		conn_info->role = true;
	}
}

static bool ranging_tech_ble_cs_conn_bond_validate(struct mgmt_conn *mgmt_conn)
{
	int err;
	struct bt_conn_info conn_info;

	__ASSERT_NO_MSG(mgmt_conn);

	err = bt_conn_get_info(mgmt_conn->conn, &conn_info);
	__ASSERT_NO_MSG(!err);

	/* Use the role field in the bt_conn_info structure to store search result.
	 * false -> The Bluetooth bond has not been found.
	 * true -> The Bluetooth bond has been found.
	 */
	conn_info.role = false;
	bt_foreach_bond(conn_info.id, ranging_tech_ble_cs_bond_find, &conn_info);

	return (conn_info.role == true);
}

static int ranging_tech_ble_cs_security_level_max_supported_discover(uint8_t *security_level)
{
	int err;
	struct bt_conn_le_cs_capabilities capabilities = {0};

	__ASSERT_NO_MSG(security_level);

	err = bt_le_cs_read_local_supported_capabilities(&capabilities);
	if (err) {
		LOG_ERR("Ranging: BLE CS: failed to read local supported capabilities: %d", err);
		return err;
	}

	/* Implementation based on the Bluetooth Core Specification v6.2, Volume 3,
	 * Part C (Generic Access Profile), section 10.11.1 (Channel Sounding security).
	 */
	 *security_level = 0;

	/* Level 1: CS tones are mandatory if CONFIG_BT_CHANNEL_SOUNDING Kconfig is enabled. */
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_BT_CHANNEL_SOUNDING));
	(*security_level)++;


	/* Level 2: 150 ns RTT accuracy is mandatory. */
	if ((capabilities.rtt_aa_only_precision == BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP) &&
	    (capabilities.rtt_sounding_precision == BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP) &&
	    (capabilities.rtt_random_payload_precision ==
		BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP)) {
		return 0;
	}
	(*security_level)++;

	/* Level 3: 10 ns RTT accuracy is supported. */
	if ((capabilities.rtt_aa_only_precision != BT_CONN_LE_CS_RTT_AA_ONLY_10NS) &&
	    (capabilities.rtt_sounding_precision != BT_CONN_LE_CS_RTT_SOUNDING_10NS) &&
	    (capabilities.rtt_random_payload_precision != BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS)) {
		return 0;
	}
	(*security_level)++;

	/* Level 4: 10 ns RTT accuracy with NADM is supported for either:
	 *          - sounding sequence, or
	 *          - random sequence payloads
	 */
	if (((capabilities.rtt_sounding_precision != BT_CONN_LE_CS_RTT_SOUNDING_10NS) ||
	     !capabilities.phase_based_nadm_sounding_supported) &&
	    ((capabilities.rtt_random_payload_precision != BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS) &&
	     !capabilities.phase_based_nadm_random_supported)) {
		return 0;
	}
	(*security_level)++;

	return 0;
}

static uint8_t ranging_tech_ble_cs_security_level_bm_get(void)
{
	int err;
	const uint8_t security_level_bm_unpopulated = 0xFF;
	static uint8_t security_level_bm = security_level_bm_unpopulated;

	/* Dynamically calculate the security level bitmask if the bitmask is not yet cached. */
	if (security_level_bm == security_level_bm_unpopulated) {
		const uint8_t security_level_max = 4;
		const uint8_t security_level_min = 1;
		uint8_t security_level_max_supported = 0;

		/* Discover the maximum supported security level. */
		err = ranging_tech_ble_cs_security_level_max_supported_discover(
			&security_level_max_supported);
		if (err) {
			uint8_t unknown_security_level_bm;

			unknown_security_level_bm = 0;
			bt_fast_pair_fhn_pf_ble_cs_security_level_bm_write(
				&unknown_security_level_bm,
				BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_UNKNOWN,
				true);

			return unknown_security_level_bm;
		}

		__ASSERT_NO_MSG(security_level_max_supported >= security_level_min);
		__ASSERT_NO_MSG(security_level_max_supported <= security_level_max);

		LOG_INF("Ranging: BLE CS: Maximum locally supported security level: %s",
			ranging_tech_ble_cs_security_level_str_get(security_level_max_supported));

		/* Populate the security level bitmask with the supported security levels. */
		security_level_bm = 0;
		security_level_max_supported =
			MIN(security_level_max_supported, security_level_max);
		for (uint8_t i = security_level_min; i <= security_level_max_supported; i++) {
			bt_fast_pair_fhn_pf_ble_cs_security_level_bm_write(
				&security_level_bm, i, true);
		}
	}

	return security_level_bm;
}

static void ranging_tech_ble_cs_capability_desc_populate(
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability *caps)
{
	__ASSERT_NO_MSG(caps);

	caps->supported_security_level_bm = ranging_tech_ble_cs_security_level_bm_get();
	memcpy(&caps->addr, &pairing_addr, sizeof(caps->addr));
}

static void ranging_tech_ble_cs_capability_print(void)
{
	uint8_t bm;
	char addr[BT_ADDR_STR_LEN];
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability caps;

	ranging_tech_ble_cs_capability_desc_populate(&caps);

	LOG_INF("\tBLE CS (0x%02X) technology payload:",
		BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS);

	bm = caps.supported_security_level_bm;
	LOG_INF("\t\tSupported security level bitfield 0x%02X:", bm);
	LOG_INF("\t\t%s=(%s), %s=(%s), %s=(%s), %s=(%s), %s=(%s)",
		ranging_tech_ble_cs_security_level_str_get(
			BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_UNKNOWN),
		bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_UNKNOWN) ? "X" : " ",
		ranging_tech_ble_cs_security_level_str_get(
			BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_ONE),
		bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_ONE) ? "X" : " ",
		ranging_tech_ble_cs_security_level_str_get(
			BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_TWO),
		bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_TWO) ? "X" : " ",
		ranging_tech_ble_cs_security_level_str_get(
			BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_THREE),
		bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_THREE) ? "X" : " ",
		ranging_tech_ble_cs_security_level_str_get(
			BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR),
		bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
			bm, BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR) ? "X" : " ");

	bt_addr_to_str(&caps.addr, addr, sizeof(addr));
	LOG_INF("\t\tDevice address: %s", addr);
}

static bool ranging_tech_ble_cs_capability_prepare(
	struct mgmt_conn *mgmt_conn,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payload)
{
	int err;
	const bt_addr_t zero_addr = {0};
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability caps = {0};

	__ASSERT_NO_MSG(mgmt_conn);
	__ASSERT_NO_MSG(capability_payload);

	if (!ranging_tech_ble_cs_conn_bond_validate(mgmt_conn)) {
		LOG_WRN("Ranging: BLE CS: connection (%p) is not bonded: "
			"rejecting ranging with CS technology", mgmt_conn->conn);
		return false;
	}

	/* In the case of Bluetooth CS, the management connection and ranging connection
	 * are the same entity. It is mandatory to encode the local RPA that was used during
	 * the Bluetooth LE pairing procedure with the Android device. Android devices identify
	 * peers by the first encountered address (RPA in this case) instead of relying on the
	 * identity address that is provided to them during pairing.
	 */
	if (memcmp(&pairing_addr, &zero_addr, sizeof(zero_addr)) == 0) {
		LOG_WRN("Ranging: BLE CS: pairing address is not set: "
			"rejecting ranging with CS technology");
		return false;
	}

	ranging_tech_ble_cs_capability_desc_populate(&caps);

	/* Encode the payload. */
	capability_payload->id = BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS;
	capability_payload->data = mgmt_conn->ble_cs.capability_payload;
	capability_payload->data_len = sizeof(mgmt_conn->ble_cs.capability_payload);

	err = bt_fast_pair_fhn_pf_ble_cs_ranging_capability_encode(&caps, capability_payload);
	__ASSERT_NO_MSG(!err);

	return true;
}

static void ranging_tech_ble_cs_config_handle(
	struct mgmt_conn *mgmt_conn,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payload)
{
	int err;
	char addr[BT_ADDR_STR_LEN];
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_config *config_desc;
	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = false,
		.enable_reflector_role = true,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	__ASSERT_NO_MSG(mgmt_conn);
	__ASSERT_NO_MSG(config_payload);

	if (!ranging_tech_ble_cs_conn_bond_validate(mgmt_conn)) {
		LOG_WRN("Ranging: BLE CS: connection (%p) is not bonded: "
			"rejecting ranging with CS technology", mgmt_conn->conn);

		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, false);
		return;
	}

	/* Parse the BLE CS configuration data and print the results. */
	config_desc = &mgmt_conn->ble_cs.config_desc;
	err = bt_fast_pair_fhn_pf_ble_cs_ranging_config_decode(config_payload, config_desc);
	if (err) {
		LOG_ERR("Ranging: BLE CS: failed to decode configuration data (err %d)", err);

		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, false);
		return;
	}

	LOG_INF("\tBLE CS (0x%02X) technology payload:",
		BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS);
	LOG_INF("\t\tSelected security level: %s",
		ranging_tech_ble_cs_security_level_str_get(config_desc->selected_security_level));
	bt_addr_to_str(&config_desc->addr, addr, sizeof(addr));
	LOG_INF("\t\tDevice address: %s", addr);

	/* Start setting up ranging using BLE CS. */
	err = bt_le_cs_set_default_settings(mgmt_conn->conn, &default_settings);
	if (err) {
		LOG_ERR("Ranging: BLE CS: failed to set default settings (err %d)", err);

		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, false);
		return;
	}

	switch (mgmt_conn->ble_cs.state) {
	case MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED:
		/* The CS session is not yet enabled, refresh the timeout. */
		k_work_reschedule(&mgmt_conn->ble_cs.timeout_work,
				  RANGING_TECH_BLE_CS_SESSION_STATE_EPHEMERAL_TIMEOUT);
		break;
	case MGMT_CONN_BLE_CS_SESSION_STATE_ENABLED:
		/* The CS is enabled, we can proceed with the response. */
		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, true);
		break;
	case MGMT_CONN_BLE_CS_SESSION_STATE_ERROR:
		/* The CS operation has failed, we can proceed with the response. */
		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, false);
		break;
	}
}

static void ranging_tech_ble_cs_stop_handle(struct mgmt_conn *mgmt_conn)
{
	__ASSERT_NO_MSG(mgmt_conn);

	switch (mgmt_conn->ble_cs.state) {
	case MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED:
		/* The CS is disabled, we can proceed with the response. */
		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, true);
		break;
	case MGMT_CONN_BLE_CS_SESSION_STATE_ENABLED:
		/* The CS session is not yet disabled, refresh the timeout. */
		k_work_reschedule(&mgmt_conn->ble_cs.timeout_work,
			RANGING_TECH_BLE_CS_SESSION_STATE_EPHEMERAL_TIMEOUT);
		break;
	case MGMT_CONN_BLE_CS_SESSION_STATE_ERROR:
		/* The CS operation has failed, we can proceed with the response. */
		ranging_tech_ble_cs_rsp_ready_set(mgmt_conn, false);
		break;
	}
}

static void pf_ranging_capability_request(struct bt_conn *conn, uint16_t ranging_tech_bm)
{
	int err;
	struct mgmt_conn *mgmt_conn;
	uint16_t rsp_bm;
	uint8_t filtered_capability_payload_num;
	enum bt_fast_pair_fhn_pf_ranging_tech_id supported_tech_ids[] = {
		BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS,
	};
	struct bt_fast_pair_fhn_pf_ranging_tech_payload
		filtered_capability_payloads[ARRAY_SIZE(supported_tech_ids)];

	LOG_INF("Ranging: Ranging Capability Request message from %p", conn);
	LOG_INF("\tRequested ranging technologies bitfield 0x%04X:", ranging_tech_bm);
	ranging_tech_bm_print(ranging_tech_bm);

	/* Validate the state of the ranging tech bitmask. */
	mgmt_conn = mgmt_conn_get(conn);
	if ((mgmt_conn->state == MGMT_CONN_STATE_STOP_RANGING_RESPONSE_SENT) &&
	    (mgmt_conn->ranging_tech_bm != 0)) {
		LOG_WRN("Ranging: Ranging Capability Request: some technologies are still ranging "
			"due to the previous message exchange: 0x%04X",
			mgmt_conn->ranging_tech_bm);
	}

	/* Perform the state transition. */
	mgmt_conn->state = MGMT_CONN_STATE_RANGING_CAPABILITY_REQUEST_RECEIVED;

	/* Prepare the response parameters. */
	rsp_bm = 0;
	filtered_capability_payload_num = 0;
	for (uint8_t i = 0; i < ARRAY_SIZE(supported_tech_ids); i++) {
		bool is_tech_accepted;
		enum bt_fast_pair_fhn_pf_ranging_tech_id tech_id;
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload;

		tech_id = supported_tech_ids[i];
		if (!bt_fast_pair_fhn_pf_ranging_tech_bm_check(ranging_tech_bm, tech_id)) {
			continue;
		}

		tech_payload = &filtered_capability_payloads[filtered_capability_payload_num];
		switch (tech_id) {
		case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS:
			is_tech_accepted =
				ranging_tech_ble_cs_capability_prepare(mgmt_conn, tech_payload);
			break;
		default:
			is_tech_accepted = false;
			__ASSERT_NO_MSG(0);
		}

		if (!is_tech_accepted) {
			continue;
		}

		bt_fast_pair_fhn_pf_ranging_tech_bm_write(&rsp_bm, tech_payload->id, true);
		filtered_capability_payload_num++;
	}
	mgmt_conn->ranging_tech_bm = rsp_bm;

	/* Send the Ranging Capability Response message synchronously. */
	err = bt_fast_pair_fhn_pf_ranging_capability_response_send(
		conn, rsp_bm, filtered_capability_payloads, filtered_capability_payload_num);
	if (err) {
		LOG_ERR("Ranging: Ranging Capability Response failed to be sent: %d", err);
		return;
	}

	LOG_INF("Ranging: Ranging Capability Response message to %p", conn);
	LOG_INF("\tSupported ranging technologies bitfield 0x%04X:", rsp_bm);
	ranging_tech_bm_print(rsp_bm);
	for (uint8_t i = 0; i < filtered_capability_payload_num; i++) {
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload =
			&filtered_capability_payloads[i];

		switch (tech_payload->id) {
		case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS:
			ranging_tech_ble_cs_capability_print();
			break;
		default:
			__ASSERT_NO_MSG(0);
		}
	}

	/* Perform the state transition. */
	mgmt_conn->state = MGMT_CONN_STATE_RANGING_CAPABILITY_RESPONSE_SENT;
}

static void pf_ranging_config_request(
	struct bt_conn *conn,
	uint16_t ranging_tech_bm,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payloads,
	uint8_t config_payload_num)
{
	struct mgmt_conn *mgmt_conn;

	LOG_INF("Ranging: Ranging Configuration message from %p", conn);
	LOG_INF("\tRanging technologies configuration set bitfield 0x%04X:", ranging_tech_bm);
	ranging_tech_bm_print(ranging_tech_bm);

	/* Validate the state of the ranging tech bitmask. */
	mgmt_conn = mgmt_conn_get(conn);
	if ((mgmt_conn->state == MGMT_CONN_STATE_STOP_RANGING_RESPONSE_SENT) &&
	    (mgmt_conn->ranging_tech_bm != 0)) {
		LOG_WRN("Ranging: Ranging Configuration: some technologies are still ranging "
			"due to the previous message exchange: 0x%04X",
			mgmt_conn->ranging_tech_bm);
	}

	/* Perform the state transition. */
	mgmt_conn = mgmt_conn_get(conn);
	mgmt_conn->state = MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED;
	mgmt_conn->ranging_tech_bm = ranging_tech_bm;

	/* Handle the technology-specific configuration for each configured ranging technology. */
	for (uint8_t i = 0; i < config_payload_num; i++) {
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *tech_payload =
			&config_payloads[i];

		switch (tech_payload->id) {
		case BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS:
			ranging_tech_ble_cs_config_handle(mgmt_conn, tech_payload);
			break;
		default:
			LOG_WRN("Ranging: Ranging Configuration: unsupported technology from "
				"capability exchange: 0x%04X", tech_payload->id);

			bt_fast_pair_fhn_pf_ranging_tech_bm_write(
				&mgmt_conn->ranging_tech_bm, tech_payload->id, false);
			break;
		}
	}

	/* If all affected technologies can provide a response synchronously,
	 * the response can be sent in this context.
	 */
	if (mgmt_conn->state == MGMT_CONN_STATE_RANGING_CONFIG_RECEIVED) {
		ranging_rsp_ready_check_and_send(mgmt_conn);
	}
}

static void pf_stop_ranging_request(struct bt_conn *conn, uint16_t ranging_tech_bm)
{
	struct mgmt_conn *mgmt_conn;

	LOG_INF("Ranging: Stop Ranging message from %p", conn);
	LOG_INF("\tRanging technologies to stop bitfield 0x%04X:", ranging_tech_bm);
	ranging_tech_bm_print(ranging_tech_bm);

	/* Perform the state transition. */
	mgmt_conn = mgmt_conn_get(conn);
	mgmt_conn->state = MGMT_CONN_STATE_STOP_RANGING_REQUEST_RECEIVED;

	/* Handle the technology-specific stop for each configured ranging technology.
	 * The only supported ranging technology is Bluetooth CS and is handled asynchronously.
	 */
	if (bt_fast_pair_fhn_pf_ranging_tech_bm_check(
		mgmt_conn->ranging_tech_bm, BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS)) {
		ranging_tech_ble_cs_stop_handle(mgmt_conn);
	}
}

static void pf_comm_channel_terminated(struct bt_conn *conn)
{
	LOG_INF("Ranging: communication channel terminated for %p", conn);

	/* No need to do anything as we support only Bluetooth connection-based ranging.
	 * The connection object context will be cleared in the disconnected callback.
	 */
}

static struct bt_fast_pair_fhn_pf_ranging_mgmt_cb ranging_mgmt_cb = {
	.ranging_capability_request = pf_ranging_capability_request,
	.ranging_config_request = pf_ranging_config_request,
	.stop_ranging_request = pf_stop_ranging_request,
	.comm_channel_terminated = pf_comm_channel_terminated,
};

static void ranging_connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct mgmt_conn *mgmt_conn = mgmt_conn_get(conn);

	if (conn_err) {
		return;
	}

	mgmt_conn->conn = conn;
}

static void ranging_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct mgmt_conn *mgmt_conn = mgmt_conn_get(conn);

	/* Clear the management connection context. */
	mgmt_conn->conn = NULL;
	mgmt_conn->ranging_tech_bm = 0;
	mgmt_conn->stop_ranging_tech_bm = 0;
	mgmt_conn->state = MGMT_CONN_STATE_INIT;

	k_work_cancel_delayable(&mgmt_conn->ble_cs.timeout_work);
	mgmt_conn->ble_cs.state = MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED;
	mgmt_conn->ble_cs.is_rsp_ready = false;
}

static void ranging_ble_cs_remote_capabilities_completed(
	struct bt_conn *conn, uint8_t status, struct bt_conn_le_cs_capabilities *params)
{
	struct mgmt_conn *mgmt_conn;

	ARG_UNUSED(params);

	if (status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("Ranging: BLE CS: capability exchange completed with %p", conn);
	} else {
		LOG_WRN("Ranging: BLE CS: capability exchange failed for %p (HCI status 0x%02x)",
			conn, status);

		mgmt_conn = mgmt_conn_get(conn);
		ranging_tech_ble_cs_session_state_set(
			mgmt_conn, MGMT_CONN_BLE_CS_SESSION_STATE_ERROR);
	}
}

static void ranging_ble_cs_config_completed(
	struct bt_conn *conn, uint8_t status, struct bt_conn_le_cs_config *config)
{
	struct mgmt_conn *mgmt_conn;
	bool cs_op_error = false;

	if (status == BT_HCI_ERR_SUCCESS) {
		int err;
		const struct bt_le_cs_set_procedure_parameters_param procedure_params = {
			.config_id = 0,
			.max_procedure_len = 1000,
			.min_procedure_interval = 1,
			.max_procedure_interval = 100,
			.max_procedure_count = 0,
			.min_subevent_len = 10000,
			.max_subevent_len = 75000,
			.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A1_B1,
			.phy = BT_LE_CS_PROCEDURE_PHY_2M,
			.tx_power_delta = 0x80,
			.preferred_peer_antenna = BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_1,
			.snr_control_initiator = BT_LE_CS_SNR_CONTROL_NOT_USED,
			.snr_control_reflector = BT_LE_CS_SNR_CONTROL_NOT_USED,
		};

		const char *mode_str[5] = {"Unused", "1 (RTT)", "2 (PBR)", "3 (RTT + PBR)",
					   "Invalid"};
		const char *role_str[3] = {"Initiator", "Reflector", "Invalid"};
		const char *rtt_type_str[8] = {
			"AA only",	 "32-bit sounding", "96-bit sounding", "32-bit random",
			"64-bit random", "96-bit random",   "128-bit random",  "Invalid"};
		const char *phy_str[4] = {"Invalid", "LE 1M PHY", "LE 2M PHY", "LE 2M 2BT PHY"};
		const char *chsel_type_str[3] = {"Algorithm #3b", "Algorithm #3c", "Invalid"};
		const char *ch3c_shape_str[3] = {"Hat shape", "X shape", "Invalid"};

		uint8_t mode_idx = config->mode > 0 && config->mode < 4 ? config->mode : 4;
		uint8_t role_idx = MIN(config->role, 2);
		uint8_t rtt_type_idx = MIN(config->rtt_type, 7);
		uint8_t phy_idx = config->cs_sync_phy > 0 && config->cs_sync_phy < 4
					  ? config->cs_sync_phy
					  : 0;
		uint8_t chsel_type_idx = MIN(config->channel_selection_type, 2);
		uint8_t ch3c_shape_idx = MIN(config->ch3c_shape, 2);

		LOG_INF("Ranging: BLE CS: config creation complete for %p:\n"
			" - id: %u\n"
			" - mode: %s\n"
			" - min_main_mode_steps: %u\n"
			" - max_main_mode_steps: %u\n"
			" - main_mode_repetition: %u\n"
			" - mode_0_steps: %u\n"
			" - role: %s\n"
			" - rtt_type: %s\n"
			" - cs_sync_phy: %s\n"
			" - channel_map_repetition: %u\n"
			" - channel_selection_type: %s\n"
			" - ch3c_shape: %s\n"
			" - ch3c_jump: %u\n"
			" - t_ip1_time_us: %u\n"
			" - t_ip2_time_us: %u\n"
			" - t_fcs_time_us: %u\n"
			" - t_pm_time_us: %u\n"
			" - channel_map: 0x%08X%08X%04X\n",
			conn,
			config->id, mode_str[mode_idx],
			config->min_main_mode_steps, config->max_main_mode_steps,
			config->main_mode_repetition, config->mode_0_steps, role_str[role_idx],
			rtt_type_str[rtt_type_idx], phy_str[phy_idx],
			config->channel_map_repetition, chsel_type_str[chsel_type_idx],
			ch3c_shape_str[ch3c_shape_idx], config->ch3c_jump, config->t_ip1_time_us,
			config->t_ip2_time_us, config->t_fcs_time_us, config->t_pm_time_us,
			sys_get_le32(&config->channel_map[6]),
			sys_get_le32(&config->channel_map[2]),
			sys_get_le16(&config->channel_map[0]));

		err = bt_le_cs_set_procedure_parameters(conn, &procedure_params);
		if (err) {
			LOG_ERR("Failed to set procedure parameters (err %d)", err);
			cs_op_error = true;
		}
	} else {
		LOG_WRN("Ranging: BLE CS: config creation failed for %p (HCI status 0x%02x)",
			conn, status);

		cs_op_error = true;
	}

	if (cs_op_error) {
		mgmt_conn = mgmt_conn_get(conn);
		ranging_tech_ble_cs_session_state_set(
			mgmt_conn, MGMT_CONN_BLE_CS_SESSION_STATE_ERROR);
	}
}

static void ranging_ble_cs_security_enable_completed(struct bt_conn *conn, uint8_t status)
{
	struct mgmt_conn *mgmt_conn;

	if (status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("Ranging: BLE CS: security enabled with %p", conn);
	} else {
		LOG_WRN("Ranging: BLE CS: security enable failed for %p (HCI status 0x%02x)",
			conn, status);

		mgmt_conn = mgmt_conn_get(conn);
		ranging_tech_ble_cs_session_state_set(
			mgmt_conn, MGMT_CONN_BLE_CS_SESSION_STATE_ERROR);
	}
}

static void ranging_ble_cs_procedure_enable_completed(
	struct bt_conn *conn,
	uint8_t status,
	struct bt_conn_le_cs_procedure_enable_complete *params)
{
	struct mgmt_conn *mgmt_conn;
	enum mgmt_conn_ble_cs_session_state state;

	if (status == BT_HCI_ERR_SUCCESS) {
		if (params->state == 1) {
			LOG_INF("Ranging: BLE CS: procedures enabled with %p:\n"
				" - config ID: %u\n"
				" - antenna configuration index: %u\n"
				" - TX power: %d dbm\n"
				" - subevent length: %u us\n"
				" - subevents per event: %u\n"
				" - subevent interval: %u\n"
				" - event interval: %u\n"
				" - procedure interval: %u\n"
				" - procedure count: %u\n"
				" - maximum procedure length: %u",
				conn,
				params->config_id, params->tone_antenna_config_selection,
				params->selected_tx_power, params->subevent_len,
				params->subevents_per_event, params->subevent_interval,
				params->event_interval, params->procedure_interval,
				params->procedure_count, params->max_procedure_len);

			state = MGMT_CONN_BLE_CS_SESSION_STATE_ENABLED;
		} else {
			LOG_INF("Ranging: BLE CS: procedures disabled with %p", conn);

			state = MGMT_CONN_BLE_CS_SESSION_STATE_DISABLED;
		}
	} else {
		LOG_WRN("Ranging: BLE CS: procedures enable failed for %p (HCI status 0x%02x)",
			conn, status);

		state = MGMT_CONN_BLE_CS_SESSION_STATE_ERROR;
	}

	mgmt_conn = mgmt_conn_get(conn);
	ranging_tech_ble_cs_session_state_set(mgmt_conn, state);
}

BT_CONN_CB_DEFINE(app_ranging_conn_callbacks) = {
	.connected = ranging_connected,
	.disconnected = ranging_disconnected,
	.le_cs_read_remote_capabilities_complete = ranging_ble_cs_remote_capabilities_completed,
	.le_cs_config_complete = ranging_ble_cs_config_completed,
	.le_cs_security_enable_complete = ranging_ble_cs_security_enable_completed,
	.le_cs_procedure_enable_complete = ranging_ble_cs_procedure_enable_completed,
};

static void ranging_pairing_complete(struct bt_conn *conn, bool bonded)
{
	int err;
	const bt_addr_t *local_addr;
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_STR_LEN];

	/* In the case of Bluetooth CS, the management connection and ranging
	 * connection are the same entity.
	 */
	err = bt_conn_get_info(conn, &conn_info);
	__ASSERT_NO_MSG(!err);
	local_addr = &conn_info.le.local->a;

	bt_addr_to_str(local_addr, addr, sizeof(addr));
	LOG_INF("Ranging: pairing completed (%s) with %p: local address: %s",
		bonded ? "bonded" : "unbonded", conn, addr);
	if (!bonded) {
		LOG_WRN("Ranging: pairing without bonding: "
			"bonding is required for Bluetooth CS ranging");
		return;
	}

	err = settings_save_one(SETTINGS_RANGING_PAIRING_ADDR_ADDR_FULL_NAME,
				local_addr,
				sizeof(*local_addr));
	if (err) {
		LOG_ERR("Ranging: settings_save_one failed (err %d) for local pairing address",
			err);

		/* Storage failure should not happen. */
		__ASSERT_NO_MSG(0);
	}

	memcpy(&pairing_addr, local_addr, sizeof(pairing_addr));
}

static struct bt_conn_auth_info_cb ranging_conn_auth_info_cb = {
	.pairing_complete = ranging_pairing_complete,
};

static int ranging_settings_pairing_addr_load(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int ret;
	bt_addr_t addr;

	BUILD_ASSERT(sizeof(addr) == sizeof(pairing_addr));

	if (len != sizeof(addr)) {
		return -EINVAL;
	}

	ret = read_cb(cb_arg, &addr, sizeof(addr));
	if (ret < 0) {
		return ret;
	}

	if (ret != sizeof(addr)) {
		return -EINVAL;
	}

	memcpy(&pairing_addr, &addr, sizeof(pairing_addr));

	return 0;
}

static int ranging_settings_set(const char *name,
				size_t len,
				settings_read_cb read_cb,
				void *cb_arg)
{
	int err;

	if (!strncmp(name,
		     SETTINGS_RANGING_PAIRING_ADDR_ADDR_NAME,
		     sizeof(SETTINGS_RANGING_PAIRING_ADDR_ADDR_NAME))) {
		err = ranging_settings_pairing_addr_load(len, read_cb, cb_arg);
	} else {
		/* The module does not handle any other settings entries. */
		err = -EINVAL;
	}

	__ASSERT_NO_MSG(!err);

	return err;
}

SETTINGS_STATIC_HANDLER_DEFINE(app_ranging_settings,
	SETTINGS_RANGING_SUBTREE_NAME,
	NULL,
	ranging_settings_set,
	NULL,
	NULL);

static void ranging_mgmt_conn_context_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(mgmt_conns); i++) {
		k_work_init_delayable(&mgmt_conns[i].ble_cs.timeout_work,
				      ranging_tech_ble_cs_timeout_work_handle);
	}
}

static int ranging_init(void)
{
	int err;

	ranging_mgmt_conn_context_init();

	err = bt_conn_auth_info_cb_register(&ranging_conn_auth_info_cb);
	if (err) {
		LOG_ERR("Ranging: bt_conn_auth_info_cb_register failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_fhn_pf_ranging_mgmt_cb_register(&ranging_mgmt_cb);
	if (err) {
		LOG_ERR("Ranging: bt_fast_pair_fhn_pf_ranging_mgmt_cb_register failed (err %d)",
			err);
		return err;
	}

	return 0;
}

SYS_INIT(ranging_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
