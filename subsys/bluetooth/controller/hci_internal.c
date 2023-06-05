/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_err.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <sdc_hci.h>
#include <sdc_hci_cmd_controller_baseband.h>
#include <sdc_hci_cmd_info_params.h>
#include <sdc_hci_cmd_le.h>
#include <sdc_hci_cmd_link_control.h>
#include <sdc_hci_cmd_status_params.h>
#include <sdc_hci_vs.h>

#include "hci_internal.h"
#include "ecdh.h"

#define CMD_COMPLETE_MIN_SIZE (BT_HCI_EVT_HDR_SIZE \
				+ sizeof(struct bt_hci_evt_cmd_complete) \
				+ sizeof(struct bt_hci_evt_cc_status))

static struct
{
	bool occurred; /**< Set in only one execution context */
	uint8_t raw_event[BT_BUF_EVT_RX_SIZE];
} cmd_complete_or_status;

static hci_internal_user_cmd_handler_t user_cmd_handler;

static bool command_generates_command_complete_event(uint16_t hci_opcode)
{
	switch (hci_opcode) {
	case SDC_HCI_OPCODE_CMD_LC_DISCONNECT:
	case SDC_HCI_OPCODE_CMD_LE_SET_PHY:
	case SDC_HCI_OPCODE_CMD_LC_READ_REMOTE_VERSION_INFORMATION:
	case SDC_HCI_OPCODE_CMD_LE_CREATE_CONN:
	case SDC_HCI_OPCODE_CMD_LE_CONN_UPDATE:
	case SDC_HCI_OPCODE_CMD_LE_READ_REMOTE_FEATURES:
	case SDC_HCI_OPCODE_CMD_LE_ENABLE_ENCRYPTION:
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN:
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN_V2:
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_CREATE_SYNC:
	case SDC_HCI_OPCODE_CMD_LE_REQUEST_PEER_SCA:
	case SDC_HCI_OPCODE_CMD_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL:
	case SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE:
	case SDC_HCI_OPCODE_CMD_VS_WRITE_REMOTE_TX_POWER:
	case BT_HCI_OP_LE_P256_PUBLIC_KEY:
	case BT_HCI_OP_LE_GENERATE_DHKEY:
		return false;
	default:
		return true;
	}
}

/* Return true if the host has been using both legacy and extended HCI commands
 * since last HCI reset.
 */
static bool is_host_using_legacy_and_extended_commands(uint16_t hci_opcode)
{
#if defined(CONFIG_BT_HCI_HOST)
	/* For a combined host and controller build, we know that the zephyr
	 * host is used. For this case we know that the host is not
	 * combining legacy and extended commands. Therefore we can
	 * simplify this validation. */
	return false;
#else
	/* A host is not allowed to use both legacy and extended HCI commands.
	 * See Core v5.1, Vol2, Part E, 3.1.1 Legacy and extended advertising
	 */
	static enum {
		ADV_COMMAND_TYPE_NONE,
		ADV_COMMAND_TYPE_LEGACY,
		ADV_COMMAND_TYPE_EXTENDED,
	} type_of_adv_cmd_used_since_reset;

	switch (hci_opcode) {
#if defined(CONFIG_BT_BROADCASTER)
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_DATA:
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_RESPONSE_DATA:
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_ENABLE:
	case SDC_HCI_OPCODE_CMD_LE_READ_MAX_ADV_DATA_LENGTH:
	case SDC_HCI_OPCODE_CMD_LE_READ_NUMBER_OF_SUPPORTED_ADV_SETS:
	case SDC_HCI_OPCODE_CMD_LE_REMOVE_ADV_SET:
	case SDC_HCI_OPCODE_CMD_LE_CLEAR_ADV_SETS:
#if defined(CONFIG_BT_PER_ADV)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_DATA:
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_ENABLE:
#endif  /* CONFIG_BT_PER_ADV */
#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_PARAMS_V2:
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_SUBEVENT_DATA:
#endif /* CONFIG_BT_CTLR_SDC_PAWR_ADV */
#endif  /* CONFIG_BT_BROADCASTER */
#if defined(CONFIG_BT_OBSERVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_ENABLE:
#endif  /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN:
#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN_V2:
#endif /* CONFIG_BT_CTLR_SDC_PAWR_ADV */
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PER_ADV_SYNC)
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_CREATE_SYNC:
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL:
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_TERMINATE_SYNC:
	case SDC_HCI_OPCODE_CMD_LE_ADD_DEVICE_TO_PERIODIC_ADV_LIST:
	case SDC_HCI_OPCODE_CMD_LE_REMOVE_DEVICE_FROM_PERIODIC_ADV_LIST:
	case SDC_HCI_OPCODE_CMD_LE_CLEAR_PERIODIC_ADV_LIST:
	case SDC_HCI_OPCODE_CMD_LE_READ_PERIODIC_ADV_LIST_SIZE:
#endif
#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_SYNC_SUBEVENT:
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_RESPONSE_DATA:
#endif
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_SET_DEFAULT_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */
		if (type_of_adv_cmd_used_since_reset == ADV_COMMAND_TYPE_NONE) {
			type_of_adv_cmd_used_since_reset = ADV_COMMAND_TYPE_EXTENDED;
			return false;
		}
		return type_of_adv_cmd_used_since_reset == ADV_COMMAND_TYPE_LEGACY;

#if defined(CONFIG_BT_OBSERVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_READ_ADV_PHYSICAL_CHANNEL_TX_POWER:
	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_DATA:
	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_RESPONSE_DATA:
	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_ENABLE:
#endif  /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_OBSERVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_PARAMS:
	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_ENABLE:
#endif  /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_CREATE_CONN:
#endif  /* CONFIG_BT_CENTRAL */
		if (type_of_adv_cmd_used_since_reset == ADV_COMMAND_TYPE_NONE) {
			type_of_adv_cmd_used_since_reset = ADV_COMMAND_TYPE_LEGACY;
			return false;
		}
		return type_of_adv_cmd_used_since_reset == ADV_COMMAND_TYPE_EXTENDED;
	case SDC_HCI_OPCODE_CMD_CB_RESET:
		type_of_adv_cmd_used_since_reset = ADV_COMMAND_TYPE_NONE;
		break;
	default:
		/* Ignore command */
		break;
	}

	return false;
#endif /* CONFIG_BT_HCI */
}

static void encode_command_status(uint8_t * const event,
				  uint16_t hci_opcode,
				  uint8_t status_code)
{

	struct bt_hci_evt_hdr *evt_hdr = (struct bt_hci_evt_hdr *)event;
	struct bt_hci_evt_cmd_status *evt_data =
			(struct bt_hci_evt_cmd_status *)&event[BT_HCI_EVT_HDR_SIZE];

	evt_hdr->evt = BT_HCI_EVT_CMD_STATUS;
	evt_hdr->len = sizeof(struct bt_hci_evt_cmd_status);

	evt_data->status = status_code;
	evt_data->ncmd = 1;
	evt_data->opcode = hci_opcode;
}

static void encode_command_complete_header(uint8_t * const event,
					   uint16_t hci_opcode,
					   uint8_t param_length,
					   uint8_t status)
{
	struct bt_hci_evt_hdr *evt_hdr = (struct bt_hci_evt_hdr *)event;
	struct bt_hci_evt_cmd_complete *evt_data =
			(struct bt_hci_evt_cmd_complete *)&event[BT_HCI_EVT_HDR_SIZE];

	evt_hdr->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt_hdr->len = param_length;
	evt_data->ncmd = 1;
	evt_data->opcode = hci_opcode;
	event[BT_HCI_EVT_HDR_SIZE + sizeof(struct bt_hci_evt_cmd_complete)] = status;
}

void hci_internal_supported_commands(sdc_hci_ip_supported_commands_t *cmds)
{
	memset(cmds, 0, sizeof(*cmds));

#if defined(CONFIG_BT_CONN)
	cmds->hci_disconnect = 1;
	cmds->hci_read_remote_version_information = 1;
#endif

	cmds->hci_set_event_mask = 1;
	cmds->hci_reset = 1;

#if defined(CONFIG_BT_CONN)
	cmds->hci_read_transmit_power_level = 1;
#endif

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	cmds->hci_set_controller_to_host_flow_control = 1;
	cmds->hci_host_buffer_size = 1;
	cmds->hci_host_number_of_completed_packets = 1;
#endif

	cmds->hci_read_local_version_information = 1;
	cmds->hci_read_local_supported_features = 1;
	cmds->hci_read_bd_addr = 1;

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	cmds->hci_read_rssi = 1;
#endif

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CTLR_LE_PING)
	cmds->hci_set_event_mask_page_2 = 1;
#endif

	cmds->hci_le_set_event_mask = 1;
	cmds->hci_le_read_buffer_size_v1 = 1;
	cmds->hci_le_read_local_supported_features = 1;
	cmds->hci_le_set_random_address = 1;

#if defined(CONFIG_BT_BROADCASTER)
	cmds->hci_le_set_advertising_parameters = 1;
	cmds->hci_le_read_advertising_physical_channel_tx_power = 1;
	cmds->hci_le_set_advertising_data = 1;
	cmds->hci_le_set_scan_response_data = 1;
	cmds->hci_le_set_advertising_enable = 1;
	cmds->hci_le_set_data_related_address_changes = 1;
#endif

#if defined(CONFIG_BT_OBSERVER)
	cmds->hci_le_set_scan_parameters = 1;
	cmds->hci_le_set_scan_enable = 1;
#endif

#if defined(CONFIG_BT_CENTRAL)
	cmds->hci_le_create_connection = 1;
	cmds->hci_le_create_connection_cancel = 1;
#endif

	cmds->hci_le_read_filter_accept_list_size = 1;
	cmds->hci_le_clear_filter_accept_list = 1;
	cmds->hci_le_add_device_to_filter_accept_list = 1;
	cmds->hci_le_remove_device_from_filter_accept_list = 1;

#if defined(CONFIG_BT_CENTRAL)
	cmds->hci_le_connection_update = 1;
#endif

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_ADV_EXT)
	cmds->hci_le_set_host_channel_classification = 1;
#endif

#if defined(CONFIG_BT_CONN)
	cmds->hci_le_read_channel_map = 1;
	cmds->hci_le_read_remote_features = 1;
#endif

	cmds->hci_le_encrypt = 1;
	cmds->hci_le_rand = 1;

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	cmds->hci_le_enable_encryption = 1;
#endif

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
	cmds->hci_le_long_term_key_request_reply = 1;
	cmds->hci_le_long_term_key_request_negative_reply = 1;
#endif

	cmds->hci_le_read_supported_states = 1;

	/* NOTE: The DTM commands are *not* supported by the SoftDevice
	 * controller. See doc/nrf/known_issues.rst.
	 */
	cmds->hci_le_receiver_test_v1 = 1;
	cmds->hci_le_transmitter_test_v1 = 1;
	cmds->hci_le_test_end = 1;

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CTLR_LE_PING)
	cmds->hci_read_authenticated_payload_timeout = 1;
	cmds->hci_write_authenticated_payload_timeout = 1;
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	cmds->hci_le_set_data_length = 1;
	cmds->hci_le_read_suggested_default_data_length = 1;
	cmds->hci_le_write_suggested_default_data_length = 1;
#endif

#if defined(CONFIG_BT_CTLR_PRIVACY)
	cmds->hci_le_add_device_to_resolving_list = 1;
	cmds->hci_le_remove_device_from_resolving_list = 1;
	cmds->hci_le_clear_resolving_list = 1;
	cmds->hci_le_read_resolving_list_size = 1;
	cmds->hci_le_set_address_resolution_enable = 1;
	cmds->hci_le_set_resolvable_private_address_timeout = 1;
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	cmds->hci_le_read_maximum_data_length = 1;
#endif

#if defined(CONFIG_BT_CTLR_PHY)
	cmds->hci_le_read_phy = 1;
	cmds->hci_le_set_default_phy = 1;
	cmds->hci_le_set_phy = 1;
#endif

	/* NOTE: The DTM commands are *not* supported by the SoftDevice
	 * controller. See doc/nrf/known_issues.rst.
	 */
	cmds->hci_le_receiver_test_v2 = 1;
	cmds->hci_le_transmitter_test_v2 = 1;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	cmds->hci_le_set_advertising_set_random_address = 1;
	cmds->hci_le_set_extended_advertising_parameters = 1;
	cmds->hci_le_set_extended_advertising_data = 1;
	cmds->hci_le_set_extended_scan_response_data = 1;
	cmds->hci_le_set_extended_advertising_enable = 1;
	cmds->hci_le_read_maximum_advertising_data_length = 1;
	cmds->hci_le_read_number_of_supported_advertising_sets = 1;
	cmds->hci_le_remove_advertising_set = 1;
	cmds->hci_le_clear_advertising_sets = 1;
#if defined(CONFIG_BT_PER_ADV)
	cmds->hci_le_set_periodic_advertising_parameters = 1;
	cmds->hci_le_set_periodic_advertising_data = 1;
	cmds->hci_le_set_periodic_advertising_enable = 1;
#endif /* CONFIG_BT_PER_ADV*/
#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	cmds->hci_le_set_periodic_advertising_subevent_data = 1;
	cmds->hci_le_extended_create_connection_v2 = 1;
	cmds->hci_le_set_periodic_advertising_parameters_v2 = 1;
#endif /* CONFIG_BT_CTLR_SDC_PAWR_ADV */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	cmds->hci_le_set_extended_scan_parameters = 1;
	cmds->hci_le_set_extended_scan_enable = 1;
#endif
#if defined(CONFIG_BT_CENTRAL)
	cmds->hci_le_extended_create_connection = 1;
#endif
#if defined(CONFIG_BT_PER_ADV_SYNC)
	cmds->hci_le_periodic_advertising_create_sync = 1;
	cmds->hci_le_periodic_advertising_create_sync_cancel = 1;
	cmds->hci_le_periodic_advertising_terminate_sync = 1;
	cmds->hci_le_add_device_to_periodic_advertiser_list = 1;
	cmds->hci_le_remove_device_from_periodic_advertiser_list = 1;
	cmds->hci_le_clear_periodic_advertiser_list = 1;
	cmds->hci_le_read_periodic_advertiser_list_size = 1;
	cmds->hci_le_set_periodic_advertising_receive_enable = 1;
#endif
#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	cmds->hci_le_set_periodic_advertising_response_data = 1;
	cmds->hci_le_set_periodic_sync_subevent = 1;
#endif
#endif

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
	cmds->hci_le_periodic_advertising_sync_transfer = 1;
	cmds->hci_le_periodic_advertising_set_info_transfer = 1;
#endif

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
	cmds->hci_le_set_periodic_advertising_sync_transfer_parameters = 1;
	cmds->hci_le_set_default_periodic_advertising_sync_transfer_parameters = 1;
#endif

	cmds->hci_le_read_transmit_power = 1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	cmds->hci_le_set_privacy_mode = 1;
#endif

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	cmds->hci_le_set_connectionless_cte_transmit_parameters = 1;
	cmds->hci_le_set_connectionless_cte_transmit_enable = 1;
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	cmds->hci_le_connection_cte_response_enable = 1;
	cmds->hci_le_set_connection_cte_transmit_parameters = 1;
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP) || defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* NOTE: The DTM commands are *not* supported by the SoftDevice
	 * controller. See doc/nrf/known_issues.rst.
	 */
	cmds->hci_le_transmitter_test_v3 = 1;
#endif

#if defined(CONFIG_BT_CTLR_DF)
	cmds->hci_le_read_antenna_information = 1;
#endif

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
	cmds->hci_le_enhanced_read_transmit_power_level = 1;
	cmds->hci_le_read_remote_transmit_power_level = 1;
	cmds->hci_le_set_transmit_power_reporting_enable = 1;
#endif

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL) || defined(CONFIG_BT_CTLR_ADV_EXT)
	cmds->hci_le_read_rf_path_compensation = 1;
	cmds->hci_le_write_rf_path_compensation = 1;
#endif

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	cmds->hci_le_request_peer_sca = 1;
#endif

#if (defined(CONFIG_BT_HCI_RAW) && defined(CONFIG_BT_TINYCRYPT_ECC)) || defined(CONFIG_BT_CTLR_ECDH)
	cmds->hci_le_read_local_p256_public_key = 1;
	cmds->hci_le_generate_dhkey_v1 = 1;
	cmds->hci_le_generate_dhkey_v2 = 1;
#endif
}

#if defined(CONFIG_BT_HCI_VS)
static void vs_zephyr_supported_commands(sdc_hci_vs_zephyr_supported_commands_t *cmds)
{
	memset(cmds, 0, sizeof(*cmds));

	cmds->read_version_info = 1;
	cmds->read_supported_commands = 1;

#if defined(CONFIG_BT_HCI_VS_EXT)
	cmds->write_bd_addr = 1;
	cmds->read_static_addresses = 1;
	cmds->read_key_hierarchy_roots = 1;
	cmds->read_chip_temperature = 1;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	cmds->write_tx_power_level = 1;
	cmds->read_tx_power_level = 1;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
#endif /* CONFIG_BT_HCI_VS_EXT */
}

static void vs_supported_commands(sdc_hci_vs_supported_vs_commands_t *cmds)
{
	memset(cmds, 0, sizeof(*cmds));

	cmds->read_supported_vs_commands = 1;
	cmds->llpm_mode_set = 1;
	cmds->conn_update = 1;
	cmds->conn_event_extend = 1;
	cmds->qos_conn_event_report_enable = 1;
	cmds->event_length_set = 1;
#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
	cmds->write_remote_tx_power = 1;
	cmds->set_auto_power_control_request_param = 1;
#endif
}
#endif	/* CONFIG_BT_HCI_VS */

static void supported_features(sdc_hci_ip_lmp_features_t *features)
{
	memset(features, 0, sizeof(*features));

	features->bdedr_not_supported = 1;
	features->le_supported = 1;
}

void hci_internal_le_supported_features(
	sdc_hci_cmd_le_read_local_supported_features_return_t *features)
{
	memset(features, 0, sizeof(*features));

	features->params.le_encryption = 1;
	features->params.extended_reject_indication = 1;
	features->params.slave_initiated_features_exchange = 1;
	features->params.le_ping = 1;

#ifdef CONFIG_BT_CTLR_DATA_LENGTH
	features->params.le_data_packet_length_extension = 1;
#endif

#ifdef CONFIG_BT_CTLR_PRIVACY
	features->params.ll_privacy = 1;
#endif

#ifdef CONFIG_BT_CTLR_EXT_SCAN_FP
	features->params.extended_scanner_filter_policies = 1;
#endif

#ifdef CONFIG_BT_CTLR_PHY_2M
	features->params.le_2m_phy = 1;
#endif

#ifdef CONFIG_BT_CTLR_PHY_CODED
	features->params.le_coded_phy = 1;
#endif

#ifdef CONFIG_BT_CTLR_ADV_EXT
	features->params.le_extended_advertising = 1;
#endif

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC) || defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	features->params.le_periodic_advertising = 1;
#ifdef CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER
	features->params.periodic_advertising_sync_transfer_sender = 1;
#endif
#ifdef CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER
	features->params.periodic_advertising_sync_transfer_recipient = 1;
#endif
#endif

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	features->params.connectionless_cte_transmitter = 1;
#endif

	features->params.channel_selection_algorithm_2 = 1;

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
	features->params.le_power_control_request = 1;
	features->params.le_power_change_indication = 1;
#endif

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	features->params.periodic_advertising_adi_support = 1;
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	features->params.connection_cte_response = 1;
#endif

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	features->params.sleep_clock_accuracy_updates = 1;
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	features->params.periodic_advertising_with_responses_advertiser = 1;
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	features->params.periodic_advertising_with_responses_scanner = 1;
#endif
}

static void le_read_supported_states(uint8_t *buf)
{
	/* Use 2*uint32_t instead of uint64_t to reduce code size. */
	uint32_t states1 = 0U;
	uint32_t states2 = 0U;

#define ST_ADV (BIT(0)  | BIT(1)  | BIT(8)  | BIT(9)  | BIT(12) | \
		BIT(13) | BIT(16) | BIT(17) | BIT(18) | BIT(19) | \
		BIT(20) | BIT(21))

#define ST_SCA (BIT(4)  | BIT(5)  | BIT(8)  | BIT(9)  | BIT(10) | \
		BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15) | \
		BIT(22) | BIT(23) | BIT(24) | BIT(25) | BIT(26) | \
		BIT(27) | BIT(30) | BIT(31))

#define ST_SLA (BIT(2)  | BIT(3)  | BIT(7)  | BIT(10) | BIT(11) | \
		BIT(14) | BIT(15) | BIT(20) | BIT(21) | BIT(26) | \
		BIT(27) | BIT(29) | BIT(30) | BIT(31))

#define ST_SLA2 (BIT(32 - 32) | \
		BIT(33 - 32) | BIT(34 - 32) | BIT(35 - 32) | BIT(36 - 32) | BIT(37 - 32) | \
		BIT(38 - 32) | BIT(39 - 32) | BIT(40 - 32) | BIT(41 - 32))


#define ST_MAS (BIT(6)  | BIT(16) | BIT(17) | BIT(18) | BIT(19) | \
		BIT(22) | BIT(23) | BIT(24) | BIT(25) | BIT(28))

#define ST_MAS2 (BIT(32 - 32) | BIT(33 - 32) | BIT(34 - 32) | BIT(35 - 32) | BIT(36 - 32) | \
		BIT(37 - 32) | BIT(41 - 32))

#if defined(CONFIG_BT_BROADCASTER)
	states1 |= ST_ADV;
#else
	states1 &= ~ST_ADV;
#endif
#if defined(CONFIG_BT_OBSERVER)
	states1 |= ST_SCA;
#else
	states1 &= ~ST_SCA;
#endif
#if defined(CONFIG_BT_PERIPHERAL)
	states1 |= ST_SLA;
	states2 |= ST_SLA2;
#else
	states1 &= ~ST_SLA;
	states2 &= ~ST_SLA2;
#endif
#if defined(CONFIG_BT_CENTRAL)
	states1 |= ST_MAS;
	states2 |= ST_MAS;
#else
	states1 &= ~ST_MAS;
	states2 &= ~ST_MAS2;
#endif
	/* All states and combinations supported except:
	 * Initiating State + Passive Scanning
	 * Initiating State + Active Scanning
	 */
	states1 &= ~(BIT(22) | BIT(23));
	*buf = states1;
	*(buf + 4) = states2;
}

int hci_internal_user_cmd_handler_register(const hci_internal_user_cmd_handler_t handler)
{
	if (user_cmd_handler) {
		return -EAGAIN;
	}

	user_cmd_handler = handler;
	return 0;
}

#if defined(CONFIG_BT_CONN)
static uint8_t link_control_cmd_put(uint8_t const * const cmd)
{
	uint16_t opcode = sys_get_le16(cmd);
	uint8_t const *cmd_params = &cmd[BT_HCI_CMD_HDR_SIZE];

	switch (opcode)	{
	case SDC_HCI_OPCODE_CMD_LC_DISCONNECT:
		return sdc_hci_cmd_lc_disconnect((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_LC_READ_REMOTE_VERSION_INFORMATION:
		return sdc_hci_cmd_lc_read_remote_version_information((void *)cmd_params);
	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}
#endif

static uint8_t controller_and_baseband_cmd_put(uint8_t const * const cmd,
					       uint8_t * const raw_event_out,
					       uint8_t *param_length_out)
{
	uint8_t const *cmd_params = &cmd[BT_HCI_CMD_HDR_SIZE];
	uint16_t opcode = sys_get_le16(cmd);
#if defined(CONFIG_BT_CONN)
	uint8_t * const event_out_params = &raw_event_out[CMD_COMPLETE_MIN_SIZE];
#endif

	switch (opcode)	{
	case SDC_HCI_OPCODE_CMD_CB_SET_EVENT_MASK:
		return sdc_hci_cmd_cb_set_event_mask((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_CB_RESET:
		return sdc_hci_cmd_cb_reset();

#if defined(CONFIG_BT_CONN)
	case SDC_HCI_OPCODE_CMD_CB_READ_TRANSMIT_POWER_LEVEL:
		*param_length_out += sizeof(sdc_hci_cmd_cb_read_transmit_power_level_return_t);
		return sdc_hci_cmd_cb_read_transmit_power_level((void *)cmd_params,
								(void *)event_out_params);
#endif
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	case SDC_HCI_OPCODE_CMD_CB_SET_CONTROLLER_TO_HOST_FLOW_CONTROL:
		return sdc_hci_cmd_cb_set_controller_to_host_flow_control((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_CB_HOST_BUFFER_SIZE:
		return sdc_hci_cmd_cb_host_buffer_size((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_CB_HOST_NUMBER_OF_COMPLETED_PACKETS:
		return sdc_hci_cmd_cb_host_number_of_completed_packets((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CTLR_LE_PING)
	case SDC_HCI_OPCODE_CMD_CB_SET_EVENT_MASK_PAGE_2:
		return sdc_hci_cmd_cb_set_event_mask_page_2((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_CB_READ_AUTHENTICATED_PAYLOAD_TIMEOUT:
		*param_length_out +=
				sizeof(sdc_hci_cmd_cb_read_authenticated_payload_timeout_return_t);
		return sdc_hci_cmd_cb_read_authenticated_payload_timeout((void *)cmd_params,
									 (void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_CB_WRITE_AUTHENTICATED_PAYLOAD_TIMEOUT:
		*param_length_out +=
				sizeof(sdc_hci_cmd_cb_write_authenticated_payload_timeout_return_t);
		return sdc_hci_cmd_cb_write_authenticated_payload_timeout((void *)cmd_params,
									  (void *)event_out_params);
#endif
	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}

static uint8_t info_param_cmd_put(uint8_t const * const cmd,
				 uint8_t * const raw_event_out,
				 uint8_t *param_length_out)
{
	uint8_t * const event_out_params = &raw_event_out[CMD_COMPLETE_MIN_SIZE];
	uint16_t opcode = sys_get_le16(cmd);

	switch (opcode)	{
	case SDC_HCI_OPCODE_CMD_IP_READ_LOCAL_VERSION_INFORMATION:
		*param_length_out += sizeof(sdc_hci_cmd_ip_read_local_version_information_return_t);
		return sdc_hci_cmd_ip_read_local_version_information((void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_IP_READ_LOCAL_SUPPORTED_COMMANDS:
		*param_length_out += sizeof(sdc_hci_cmd_ip_read_local_supported_commands_return_t);
		hci_internal_supported_commands((void *)event_out_params);
		return 0;
	case SDC_HCI_OPCODE_CMD_IP_READ_LOCAL_SUPPORTED_FEATURES:
		*param_length_out += sizeof(sdc_hci_cmd_ip_read_local_supported_features_return_t);
		supported_features((void *)event_out_params);
		return 0;
	case SDC_HCI_OPCODE_CMD_IP_READ_BD_ADDR:
		*param_length_out += sizeof(sdc_hci_cmd_ip_read_bd_addr_return_t);
		return sdc_hci_cmd_ip_read_bd_addr((void *)event_out_params);
	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}

static uint8_t status_param_cmd_put(uint8_t const * const cmd,
				    uint8_t * const raw_event_out,
				    uint8_t *param_length_out)
{
#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	uint8_t const *cmd_params = &cmd[BT_HCI_CMD_HDR_SIZE];
	uint8_t * const event_out_params = &raw_event_out[CMD_COMPLETE_MIN_SIZE];
#endif
	uint16_t opcode = sys_get_le16(cmd);

	switch (opcode)	{
#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case SDC_HCI_OPCODE_CMD_SP_READ_RSSI:
		*param_length_out += sizeof(sdc_hci_cmd_sp_read_rssi_return_t);
		return sdc_hci_cmd_sp_read_rssi((void *)cmd_params,
						(void *)event_out_params);
#endif
	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}

static uint8_t le_controller_cmd_put(uint8_t const * const cmd,
				     uint8_t * const raw_event_out,
				     uint8_t *param_length_out)
{
	uint8_t const *cmd_params = &cmd[BT_HCI_CMD_HDR_SIZE];
	uint8_t * const event_out_params = &raw_event_out[CMD_COMPLETE_MIN_SIZE];
	uint16_t opcode = sys_get_le16(cmd);

	switch (opcode)	{
	case SDC_HCI_OPCODE_CMD_LE_SET_EVENT_MASK:
		return sdc_hci_cmd_le_set_event_mask((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_BUFFER_SIZE:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_buffer_size_return_t);
		return sdc_hci_cmd_le_read_buffer_size((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_LOCAL_SUPPORTED_FEATURES:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_local_supported_features_return_t);
		hci_internal_le_supported_features((void *)event_out_params);
		return 0;

	case SDC_HCI_OPCODE_CMD_LE_SET_RANDOM_ADDRESS:
		return sdc_hci_cmd_le_set_random_address((void *)cmd_params);

#if defined(CONFIG_BT_BROADCASTER)
	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_PARAMS:
		return sdc_hci_cmd_le_set_adv_params((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_ADV_PHYSICAL_CHANNEL_TX_POWER:
		*param_length_out +=
				sizeof(sdc_hci_cmd_le_read_adv_physical_channel_tx_power_return_t);
		return sdc_hci_cmd_le_read_adv_physical_channel_tx_power((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_DATA:
		return sdc_hci_cmd_le_set_adv_data((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_RESPONSE_DATA:
		return sdc_hci_cmd_le_set_scan_response_data((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_ENABLE:
		return sdc_hci_cmd_le_set_adv_enable((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_DATA_RELATED_ADDRESS_CHANGES:
		return sdc_hci_cmd_le_set_data_related_address_changes((void *)cmd_params);
#endif

#if defined(CONFIG_BT_OBSERVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_PARAMS:
		return sdc_hci_cmd_le_set_scan_params((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_SCAN_ENABLE:
		return sdc_hci_cmd_le_set_scan_enable((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_CREATE_CONN:
		return sdc_hci_cmd_le_create_conn((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_CREATE_CONN_CANCEL:
		return sdc_hci_cmd_le_create_conn_cancel();
#endif

	case SDC_HCI_OPCODE_CMD_LE_READ_FILTER_ACCEPT_LIST_SIZE:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_filter_accept_list_size_return_t);
		return sdc_hci_cmd_le_read_filter_accept_list_size((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_CLEAR_FILTER_ACCEPT_LIST:
		return sdc_hci_cmd_le_clear_filter_accept_list();

	case SDC_HCI_OPCODE_CMD_LE_ADD_DEVICE_TO_FILTER_ACCEPT_LIST:
		return sdc_hci_cmd_le_add_device_to_filter_accept_list((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_REMOVE_DEVICE_FROM_FILTER_ACCEPT_LIST:
		return sdc_hci_cmd_le_remove_device_from_filter_accept_list((void *)cmd_params);

#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_CONN_UPDATE:
		return sdc_hci_cmd_le_conn_update((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_ADV_EXT)
	case SDC_HCI_OPCODE_CMD_LE_SET_HOST_CHANNEL_CLASSIFICATION:
		return sdc_hci_cmd_le_set_host_channel_classification((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CONN)
	case SDC_HCI_OPCODE_CMD_LE_READ_CHANNEL_MAP:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_channel_map_return_t);
		return sdc_hci_cmd_le_read_channel_map((void *)cmd_params,
						       (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_REMOTE_FEATURES:
		return sdc_hci_cmd_le_read_remote_features((void *)cmd_params);
#endif

	case SDC_HCI_OPCODE_CMD_LE_ENCRYPT:
		*param_length_out += sizeof(sdc_hci_cmd_le_encrypt_return_t);
		return sdc_hci_cmd_le_encrypt((void *)cmd_params, (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_RAND:
		*param_length_out += sizeof(sdc_hci_cmd_le_rand_return_t);
		return sdc_hci_cmd_le_rand((void *)event_out_params);

#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_ENABLE_ENCRYPTION:
		return sdc_hci_cmd_le_enable_encryption((void *)cmd_params);
#endif

#if defined(CONFIG_BT_PERIPHERAL)
	case SDC_HCI_OPCODE_CMD_LE_LONG_TERM_KEY_REQUEST_REPLY:
		*param_length_out += sizeof(sdc_hci_cmd_le_long_term_key_request_reply_return_t);
		return sdc_hci_cmd_le_long_term_key_request_reply((void *)cmd_params,
								  (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_LONG_TERM_KEY_REQUEST_NEGATIVE_REPLY:
		*param_length_out +=
			sizeof(sdc_hci_cmd_le_long_term_key_request_negative_reply_return_t);
		return sdc_hci_cmd_le_long_term_key_request_negative_reply(
				(void *)cmd_params,
				(void *)event_out_params);
#endif

	case SDC_HCI_OPCODE_CMD_LE_READ_SUPPORTED_STATES:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_supported_states_return_t);
		le_read_supported_states((void *)event_out_params);
		return 0;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case SDC_HCI_OPCODE_CMD_LE_SET_DATA_LENGTH:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_data_length_return_t);
		return sdc_hci_cmd_le_set_data_length((void *)cmd_params, (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH:
		*param_length_out +=
				sizeof(sdc_hci_cmd_le_read_suggested_default_data_length_return_t);
		return sdc_hci_cmd_le_read_suggested_default_data_length((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH:
		return sdc_hci_cmd_le_write_suggested_default_data_length((void *)cmd_params);

#endif
#if defined(CONFIG_BT_CTLR_ECDH)
	case BT_HCI_OP_LE_P256_PUBLIC_KEY:
		return hci_cmd_le_read_local_p256_public_key();
	case BT_HCI_OP_LE_GENERATE_DHKEY:
		return hci_cmd_le_generate_dhkey((void *)cmd_params);
	case BT_HCI_OP_LE_GENERATE_DHKEY_V2:
		return hci_cmd_le_generate_dhkey_v2((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_PRIVACY)
	case SDC_HCI_OPCODE_CMD_LE_ADD_DEVICE_TO_RESOLVING_LIST:
		return sdc_hci_cmd_le_add_device_to_resolving_list((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_REMOVE_DEVICE_FROM_RESOLVING_LIST:
		return sdc_hci_cmd_le_remove_device_from_resolving_list((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_CLEAR_RESOLVING_LIST:
		return sdc_hci_cmd_le_clear_resolving_list();

	case SDC_HCI_OPCODE_CMD_LE_READ_RESOLVING_LIST_SIZE:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_resolving_list_size_return_t);
		return sdc_hci_cmd_le_read_resolving_list_size((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_ADDRESS_RESOLUTION_ENABLE:
		return sdc_hci_cmd_le_set_address_resolution_enable((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT:
		return sdc_hci_cmd_le_set_resolvable_private_address_timeout((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case SDC_HCI_OPCODE_CMD_LE_READ_MAX_DATA_LENGTH:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_max_data_length_return_t);
		return sdc_hci_cmd_le_read_max_data_length((void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_PHY)
	case SDC_HCI_OPCODE_CMD_LE_READ_PHY:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_phy_return_t);
		return sdc_hci_cmd_le_read_phy((void *)cmd_params, (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_DEFAULT_PHY:
		return sdc_hci_cmd_le_set_default_phy((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_PHY:
		return sdc_hci_cmd_le_set_phy((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	case SDC_HCI_OPCODE_CMD_LE_SET_ADV_SET_RANDOM_ADDRESS:
		return sdc_hci_cmd_le_set_adv_set_random_address((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_PARAMS:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_ext_adv_params_return_t);
		return sdc_hci_cmd_le_set_ext_adv_params((void *)cmd_params,
							 (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_DATA:
		return sdc_hci_cmd_le_set_ext_adv_data((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_RESPONSE_DATA:
		return sdc_hci_cmd_le_set_ext_scan_response_data((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_ADV_ENABLE:
		return sdc_hci_cmd_le_set_ext_adv_enable((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_MAX_ADV_DATA_LENGTH:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_max_adv_data_length_return_t);
		return sdc_hci_cmd_le_read_max_adv_data_length((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_NUMBER_OF_SUPPORTED_ADV_SETS:
		*param_length_out +=
				sizeof(sdc_hci_cmd_le_read_number_of_supported_adv_sets_return_t);
		return sdc_hci_cmd_le_read_number_of_supported_adv_sets((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_REMOVE_ADV_SET:
		return sdc_hci_cmd_le_remove_adv_set((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_CLEAR_ADV_SETS:
		return sdc_hci_cmd_le_clear_adv_sets();

#if defined(CONFIG_BT_PER_ADV)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_PARAMS:
		return sdc_hci_cmd_le_set_periodic_adv_params((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_DATA:
		return sdc_hci_cmd_le_set_periodic_adv_data((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_ENABLE:
		return sdc_hci_cmd_le_set_periodic_adv_enable((void *)cmd_params);
#endif /* CONFIG_BT_PER_ADV */

#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_PARAMS:
		return sdc_hci_cmd_le_set_ext_scan_params((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_EXT_SCAN_ENABLE:
		return sdc_hci_cmd_le_set_ext_scan_enable((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CENTRAL)
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN:
		return sdc_hci_cmd_le_ext_create_conn((void *)cmd_params);
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PER_ADV_SYNC)
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_CREATE_SYNC:
		return sdc_hci_cmd_le_periodic_adv_create_sync((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL:
		return sdc_hci_cmd_le_periodic_adv_create_sync_cancel();

	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_TERMINATE_SYNC:
		return sdc_hci_cmd_le_periodic_adv_terminate_sync((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_ADD_DEVICE_TO_PERIODIC_ADV_LIST:
		return sdc_hci_cmd_le_add_device_to_periodic_adv_list((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_REMOVE_DEVICE_FROM_PERIODIC_ADV_LIST:
		return sdc_hci_cmd_le_remove_device_from_periodic_adv_list((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_CLEAR_PERIODIC_ADV_LIST:
		return sdc_hci_cmd_le_clear_periodic_adv_list();

	case SDC_HCI_OPCODE_CMD_LE_READ_PERIODIC_ADV_LIST_SIZE:
		*param_length_out +=
				sizeof(sdc_hci_cmd_le_read_periodic_adv_list_size_return_t);
		return sdc_hci_cmd_le_read_periodic_adv_list_size((void *)event_out_params);
#endif /* CONFIG_BT_PER_ADV_SYNC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	case SDC_HCI_OPCODE_CMD_LE_READ_TRANSMIT_POWER:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_transmit_power_return_t);
		return sdc_hci_cmd_le_read_transmit_power((void *)event_out_params);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	case SDC_HCI_OPCODE_CMD_LE_SET_PRIVACY_MODE:
		return sdc_hci_cmd_le_set_privacy_mode((void *)cmd_params);
#endif

#if defined(CONFIG_BT_PER_ADV_SYNC)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_RECEIVE_ENABLE:
		return sdc_hci_cmd_le_set_periodic_adv_receive_enable((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	case SDC_HCI_OPCODE_CMD_LE_SET_CONNLESS_CTE_TRANSMIT_PARAMS:
		return sdc_hci_cmd_le_set_connless_cte_transmit_params((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_CONNLESS_CTE_TRANSMIT_ENABLE:
		return sdc_hci_cmd_le_set_connless_cte_transmit_enable((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	case SDC_HCI_OPCODE_CMD_LE_SET_CONN_CTE_TRANSMIT_PARAMS:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_conn_cte_transmit_params_return_t);
		return sdc_hci_cmd_le_set_conn_cte_transmit_params((void *)cmd_params,
								   (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_CONN_CTE_RESPONSE_ENABLE:
		*param_length_out += sizeof(sdc_hci_cmd_le_conn_cte_response_enable_return_t);
		return sdc_hci_cmd_le_conn_cte_response_enable((void *)cmd_params,
							       (void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_DF)
	case SDC_HCI_OPCODE_CMD_LE_READ_ANTENNA_INFORMATION:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_antenna_information_return_t);
		return sdc_hci_cmd_le_read_antenna_information((void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
	case SDC_HCI_OPCODE_CMD_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL:
		*param_length_out +=
			sizeof(sdc_hci_cmd_le_enhanced_read_transmit_power_level_return_t);
		return sdc_hci_cmd_le_enhanced_read_transmit_power_level((void *)cmd_params,
									 (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL:
		return sdc_hci_cmd_le_read_remote_transmit_power_level((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE:
		*param_length_out +=
			sizeof(sdc_hci_cmd_le_set_transmit_power_reporting_enable_return_t);
		return sdc_hci_cmd_le_set_transmit_power_reporting_enable((void *)cmd_params,
									  (void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL) || defined(CONFIG_BT_CTLR_ADV_EXT)
	case SDC_HCI_OPCODE_CMD_LE_READ_RF_PATH_COMPENSATION:
		*param_length_out += sizeof(sdc_hci_cmd_le_read_rf_path_compensation_return_t);
		return sdc_hci_cmd_le_read_rf_path_compensation((void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_WRITE_RF_PATH_COMPENSATION:
		return sdc_hci_cmd_le_write_rf_path_compensation((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_SYNC_TRANSFER:
		*param_length_out += sizeof(sdc_hci_cmd_le_periodic_adv_sync_transfer_return_t);
		return sdc_hci_cmd_le_periodic_adv_sync_transfer((void *)cmd_params,
								 (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_PERIODIC_ADV_SET_INFO_TRANSFER:
		*param_length_out +=
			sizeof(sdc_hci_cmd_le_periodic_adv_set_info_transfer_return_t);
		return sdc_hci_cmd_le_periodic_adv_set_info_transfer((void *)cmd_params,
								     (void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
		*param_length_out +=
			sizeof(sdc_hci_cmd_le_set_periodic_adv_sync_transfer_params_return_t);
		return sdc_hci_cmd_le_set_periodic_adv_sync_transfer_params(
			(void *)cmd_params, (void *)event_out_params);

	case SDC_HCI_OPCODE_CMD_LE_SET_DEFAULT_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
		return sdc_hci_cmd_le_set_default_periodic_adv_sync_transfer_params(
			(void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case SDC_HCI_OPCODE_CMD_LE_REQUEST_PEER_SCA:
		return sdc_hci_cmd_le_request_peer_sca((void *)cmd_params);
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	case SDC_HCI_OPCODE_CMD_LE_EXT_CREATE_CONN_V2:
		return sdc_hci_cmd_le_ext_create_conn_v2((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_PARAMS_V2:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_periodic_adv_params_v2_return_t);
		return sdc_hci_cmd_le_set_periodic_adv_params_v2((void *)cmd_params,
								 (void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_SUBEVENT_DATA:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_periodic_adv_subevent_data_return_t);
		return sdc_hci_cmd_le_set_periodic_adv_subevent_data((void *)cmd_params,
								     (void *)event_out_params);
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_RESPONSE_DATA:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_periodic_adv_response_data_return_t);
		return sdc_hci_cmd_le_set_periodic_adv_response_data((void *)cmd_params,
									(void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_SYNC_SUBEVENT:
		*param_length_out += sizeof(sdc_hci_cmd_le_set_periodic_sync_subevent_return_t);
		return sdc_hci_cmd_le_set_periodic_sync_subevent((void *)cmd_params,
									(void *)event_out_params);
#endif

	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}

#if defined(CONFIG_BT_HCI_VS)
static uint8_t vs_cmd_put(uint8_t const * const cmd,
			  uint8_t * const raw_event_out,
			  uint8_t *param_length_out)
{
	uint8_t const *cmd_params = &cmd[BT_HCI_CMD_HDR_SIZE];
	uint8_t * const event_out_params = &raw_event_out[CMD_COMPLETE_MIN_SIZE];
	uint16_t opcode = sys_get_le16(cmd);

	switch (opcode)	{
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_VERSION_INFO:
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_read_version_info_return_t);
		return sdc_hci_cmd_vs_zephyr_read_version_info((void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_SUPPORTED_COMMANDS:
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_read_supported_commands_return_t);
		vs_zephyr_supported_commands((void *)event_out_params);
		return 0;

#if defined(CONFIG_BT_HCI_VS_EXT)
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_STATIC_ADDRESSES:
		/* We always return one entry */
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_read_static_addresses_return_t);
		*param_length_out += sizeof(sdc_hci_vs_zephyr_static_address_t);
		return sdc_hci_cmd_vs_zephyr_read_static_addresses((void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_KEY_HIERARCHY_ROOTS:
		*param_length_out +=
				sizeof(sdc_hci_cmd_vs_zephyr_read_key_hierarchy_roots_return_t);
		return sdc_hci_cmd_vs_zephyr_read_key_hierarchy_roots((void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_WRITE_BD_ADDR:
		return sdc_hci_cmd_vs_zephyr_write_bd_addr((void *)cmd_params);

	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_CHIP_TEMP:
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_read_chip_temp_return_t);
		return sdc_hci_cmd_vs_zephyr_read_chip_temp((void *)event_out_params);

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_WRITE_TX_POWER:
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_write_tx_power_return_t);
		return sdc_hci_cmd_vs_zephyr_write_tx_power((void *)cmd_params,
							    (void *)event_out_params);
	case SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_TX_POWER:
		*param_length_out += sizeof(sdc_hci_cmd_vs_zephyr_read_tx_power_return_t);
		return sdc_hci_cmd_vs_zephyr_read_tx_power((void *)cmd_params,
							    (void *)event_out_params);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
#endif /* CONFIG_BT_HCI_VS_EXT */
	case SDC_HCI_OPCODE_CMD_VS_READ_SUPPORTED_VS_COMMANDS:
		*param_length_out += sizeof(sdc_hci_cmd_vs_read_supported_vs_commands_return_t);
		vs_supported_commands((void *)event_out_params);
		return 0;
	case SDC_HCI_OPCODE_CMD_VS_LLPM_MODE_SET:
		return sdc_hci_cmd_vs_llpm_mode_set((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE:
		return sdc_hci_cmd_vs_conn_update((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_VS_CONN_EVENT_EXTEND:
		return sdc_hci_cmd_vs_conn_event_extend((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_VS_QOS_CONN_EVENT_REPORT_ENABLE:
		return sdc_hci_cmd_vs_qos_conn_event_report_enable((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_VS_EVENT_LENGTH_SET:
		return sdc_hci_cmd_vs_event_length_set((void *)cmd_params);
#ifdef CONFIG_BT_PERIPHERAL
	case SDC_HCI_OPCODE_CMD_VS_PERIPHERAL_LATENCY_MODE_SET:
		return sdc_hci_cmd_vs_peripheral_latency_mode_set((void *)cmd_params);
#endif
#if defined(CONFIG_BT_BROADCASTER)
	case SDC_HCI_OPCODE_CMD_VS_SET_ADV_RANDOMNESS:
		return sdc_hci_cmd_vs_set_adv_randomness((void *)cmd_params);
#endif
#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
	case SDC_HCI_OPCODE_CMD_VS_WRITE_REMOTE_TX_POWER:
		return sdc_hci_cmd_vs_write_remote_tx_power((void *)cmd_params);
	case SDC_HCI_OPCODE_CMD_VS_SET_AUTO_POWER_CONTROL_REQUEST_PARAM:
		return sdc_hci_cmd_vs_set_auto_power_control_request_param((void *)cmd_params);
#endif
	default:
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}
#endif /* CONFIG_BT_HCI_VS */

static void cmd_put(uint8_t *cmd_in, uint8_t * const raw_event_out)
{
	uint8_t status = BT_HCI_ERR_UNKNOWN_CMD;
	uint16_t opcode = sys_get_le16(cmd_in);
	bool generate_command_status_event;

	/* Assume command complete */
	uint8_t return_param_length = sizeof(struct bt_hci_evt_cmd_complete)
				      + sizeof(struct bt_hci_evt_cc_status);

	if (user_cmd_handler) {
		status = user_cmd_handler(cmd_in,
					  raw_event_out,
					  &return_param_length,
					  &generate_command_status_event);
	}

	if (status == BT_HCI_ERR_UNKNOWN_CMD) {

		switch (BT_OGF(opcode)) {
#if defined(CONFIG_BT_CONN)
		case BT_OGF_LINK_CTRL:
			status = link_control_cmd_put(cmd_in);
			break;
#endif
		case BT_OGF_BASEBAND:
			status = controller_and_baseband_cmd_put(cmd_in,
								 raw_event_out,
								 &return_param_length);
			break;
		case BT_OGF_INFO:
			status = info_param_cmd_put(cmd_in,
						    raw_event_out,
						    &return_param_length);
			break;
		case BT_OGF_STATUS:
			status = status_param_cmd_put(cmd_in,
						      raw_event_out,
						      &return_param_length);
			break;
		case BT_OGF_LE:
			status = le_controller_cmd_put(cmd_in,
						       raw_event_out,
						       &return_param_length);
			break;
#if defined(CONFIG_BT_HCI_VS)
		case BT_OGF_VS:
			status = vs_cmd_put(cmd_in,
					    raw_event_out,
					    &return_param_length);
			break;
#endif
		default:
			status = BT_HCI_ERR_UNKNOWN_CMD;
			break;
		}

		generate_command_status_event = !command_generates_command_complete_event(opcode);
	}

	if (generate_command_status_event ||
	    (status == BT_HCI_ERR_UNKNOWN_CMD))	{
		encode_command_status(raw_event_out, opcode, status);
	} else {
		encode_command_complete_header(raw_event_out, opcode, return_param_length, status);
	}
}

int hci_internal_cmd_put(uint8_t *cmd_in)
{
	uint16_t opcode = sys_get_le16(cmd_in);

	if (cmd_complete_or_status.occurred) {
		return -NRF_EPERM;
	}

	if ((((struct bt_hci_cmd_hdr *)cmd_in)->param_len + BT_HCI_CMD_HDR_SIZE)
		> HCI_CMD_PACKET_MAX_SIZE) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
		cmd_put(cmd_in, &cmd_complete_or_status.raw_event[0]);
	} else if (!is_host_using_legacy_and_extended_commands(opcode)) {
		cmd_put(cmd_in, &cmd_complete_or_status.raw_event[0]);
	} else {
		/* The host is violating the specification
		 * by mixing legacy and extended commands.
		 */
		if (command_generates_command_complete_event(opcode)) {
			uint8_t param_length = sizeof(struct bt_hci_evt_cmd_complete)
					     + sizeof(struct bt_hci_evt_cc_status);
			(void)encode_command_complete_header(cmd_complete_or_status.raw_event,
							     opcode,
							     param_length,
							     BT_HCI_ERR_CMD_DISALLOWED);
		} else {
			(void)encode_command_status(cmd_complete_or_status.raw_event,
						    opcode,
						    BT_HCI_ERR_CMD_DISALLOWED);
		}
	}

	cmd_complete_or_status.occurred = true;

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (opcode == SDC_HCI_OPCODE_CMD_CB_HOST_NUMBER_OF_COMPLETED_PACKETS
	    &&
	    cmd_complete_or_status.raw_event[CMD_COMPLETE_MIN_SIZE - 1] == 0) {
		/* SDC_HCI_OPCODE_CMD_CB_HOST_NUMBER_OF_COMPLETED_PACKETS will only generate
		 *  command complete if it fails.
		 */

		cmd_complete_or_status.occurred = false;
	}
#endif

	if (opcode == SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_RESPONSE_DATA
		&&
		cmd_complete_or_status.raw_event[0] == BT_HCI_EVT_CMD_COMPLETE) {
		/* SDC_HCI_OPCODE_CMD_LE_SET_PERIODIC_ADV_RESPONSE_DATA
		 * will generate command complete at a later time (unless unsupported)
		 */

		cmd_complete_or_status.occurred = false;
	}

	return 0;
}

int hci_internal_msg_get(uint8_t *msg_out, sdc_hci_msg_type_t *msg_type_out)
{
	if (cmd_complete_or_status.occurred) {
		struct bt_hci_evt_hdr *evt_hdr = (void *)&cmd_complete_or_status.raw_event[0];

		memcpy(msg_out,
					 &cmd_complete_or_status.raw_event[0],
					 evt_hdr->len + BT_HCI_EVT_HDR_SIZE);
		cmd_complete_or_status.occurred = false;

		*msg_type_out = SDC_HCI_MSG_TYPE_EVT;

		return 0;
	}

	return sdc_hci_get(msg_out, msg_type_out);
}
