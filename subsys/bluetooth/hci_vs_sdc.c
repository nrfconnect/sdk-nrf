/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/hci.h>
#include <bluetooth/hci_vs_sdc.h>

static int hci_vs_cmd_with_rsp_only(uint16_t opcode, void *rsp, size_t rsp_size)
{
	int err;
	struct net_buf *rsp_buf;

	err = bt_hci_cmd_send_sync(opcode, NULL, &rsp_buf);
	if (err) {
		return err;
	}

	memcpy(rsp, (void *)&rsp_buf->data[1], rsp_size);

	net_buf_unref(rsp_buf);

	return 0;
}

static int hci_vs_cmd_no_rsp(uint16_t opcode, const void *params, size_t params_size)
{
	struct net_buf *buf;

	buf = bt_hci_cmd_create(opcode, params_size);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, params, params_size);

	return bt_hci_cmd_send_sync(opcode, buf, NULL);
}

static int hci_vs_cmd_with_rsp(
	uint16_t opcode, const void *params, size_t params_size, void *rsp, size_t rsp_size)
{
	int err;
	struct net_buf *buf;
	struct net_buf *rsp_buf;

	buf = bt_hci_cmd_create(opcode, params_size);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, params, params_size);

	err = bt_hci_cmd_send_sync(opcode, buf, &rsp_buf);
	if (err) {
		return err;
	}

	memcpy(rsp, (void *)&rsp_buf->data[1], rsp_size);

	net_buf_unref(rsp_buf);

	return 0;
}

int hci_vs_sdc_zephyr_read_version_info(
	sdc_hci_cmd_vs_zephyr_read_version_info_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_VERSION_INFO,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_zephyr_read_supported_commands(
	sdc_hci_cmd_vs_zephyr_read_supported_commands_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_SUPPORTED_COMMANDS,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_zephyr_write_bd_addr(const sdc_hci_cmd_vs_zephyr_write_bd_addr_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_WRITE_BD_ADDR,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_zephyr_read_static_addresses(
	sdc_hci_cmd_vs_zephyr_read_static_addresses_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_STATIC_ADDRESSES,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_zephyr_read_key_hierarchy_roots(
	sdc_hci_cmd_vs_zephyr_read_key_hierarchy_roots_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_KEY_HIERARCHY_ROOTS,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_zephyr_read_chip_temp(sdc_hci_cmd_vs_zephyr_read_chip_temp_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_CHIP_TEMP,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_zephyr_write_tx_power(
	const sdc_hci_cmd_vs_zephyr_write_tx_power_t *params,
	sdc_hci_cmd_vs_zephyr_write_tx_power_return_t *return_params)
{
	return hci_vs_cmd_with_rsp(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_WRITE_TX_POWER,
				   params, sizeof(*params),
				   return_params, sizeof(*return_params));
}

int hci_vs_sdc_zephyr_read_tx_power(
	const sdc_hci_cmd_vs_zephyr_read_tx_power_t *params,
	sdc_hci_cmd_vs_zephyr_read_tx_power_return_t *return_params)
{
	return hci_vs_cmd_with_rsp(SDC_HCI_OPCODE_CMD_VS_ZEPHYR_READ_TX_POWER,
				   params, sizeof(*params),
				   return_params, sizeof(*return_params));
}

int hci_vs_sdc_read_supported_vs_commands(
	sdc_hci_cmd_vs_read_supported_vs_commands_return_t *return_params)
{
	return hci_vs_cmd_with_rsp_only(SDC_HCI_OPCODE_CMD_VS_READ_SUPPORTED_VS_COMMANDS,
					return_params,
					sizeof(*return_params));
}

int hci_vs_sdc_llpm_mode_set(const sdc_hci_cmd_vs_llpm_mode_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_LLPM_MODE_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_conn_update(const sdc_hci_cmd_vs_conn_update_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE,
				 params,
				 sizeof(*params));
}


int hci_vs_sdc_conn_event_extend(const sdc_hci_cmd_vs_conn_event_extend_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CONN_EVENT_EXTEND,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_qos_conn_event_report_enable(
	const sdc_hci_cmd_vs_qos_conn_event_report_enable_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_QOS_CONN_EVENT_REPORT_ENABLE,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_event_length_set(const sdc_hci_cmd_vs_event_length_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_EVENT_LENGTH_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_periodic_adv_event_length_set(
	const sdc_hci_cmd_vs_periodic_adv_event_length_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_PERIODIC_ADV_EVENT_LENGTH_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_peripheral_latency_mode_set(
	const sdc_hci_cmd_vs_peripheral_latency_mode_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_PERIPHERAL_LATENCY_MODE_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_write_remote_tx_power(const sdc_hci_cmd_vs_write_remote_tx_power_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_WRITE_REMOTE_TX_POWER,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_set_adv_randomness(const sdc_hci_cmd_vs_set_adv_randomness_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SET_ADV_RANDOMNESS,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_compat_mode_window_offset_set(
	const sdc_hci_cmd_vs_compat_mode_window_offset_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_COMPAT_MODE_WINDOW_OFFSET_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_qos_channel_survey_enable(const sdc_hci_cmd_vs_qos_channel_survey_enable_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_QOS_CHANNEL_SURVEY_ENABLE,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_set_power_control_request_params(
	const sdc_hci_cmd_vs_set_power_control_request_params_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SET_POWER_CONTROL_REQUEST_PARAMS,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_read_average_rssi(const sdc_hci_cmd_vs_read_average_rssi_t *params,
					 sdc_hci_cmd_vs_read_average_rssi_return_t *return_params)
{
	return hci_vs_cmd_with_rsp(SDC_HCI_OPCODE_CMD_VS_READ_AVERAGE_RSSI,
				   params, sizeof(*params),
				   return_params, sizeof(*return_params));
}

int hci_vs_sdc_central_acl_event_spacing_set(
	const sdc_hci_cmd_vs_central_acl_event_spacing_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CENTRAL_ACL_EVENT_SPACING_SET,
				 params,
				 sizeof(*params));
}


int hci_vs_sdc_set_conn_event_trigger(const sdc_hci_cmd_vs_set_conn_event_trigger_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SET_CONN_EVENT_TRIGGER,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_get_next_conn_event_counter(
	const sdc_hci_cmd_vs_get_next_conn_event_counter_t *params,
	sdc_hci_cmd_vs_get_next_conn_event_counter_return_t *return_params)
{
	return hci_vs_cmd_with_rsp(SDC_HCI_OPCODE_CMD_VS_GET_NEXT_CONN_EVENT_COUNTER,
				   params, sizeof(*params),
				   return_params, sizeof(*return_params));
}

int hci_vs_sdc_allow_parallel_connection_establishments(
	const sdc_hci_cmd_vs_allow_parallel_connection_establishments_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_ALLOW_PARALLEL_CONNECTION_ESTABLISHMENTS,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_min_val_of_max_acl_tx_payload_set(
	const sdc_hci_cmd_vs_min_val_of_max_acl_tx_payload_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_MIN_VAL_OF_MAX_ACL_TX_PAYLOAD_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_iso_read_tx_timestamp(const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *params,
				     sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *return_params)
{
	return hci_vs_cmd_with_rsp(SDC_HCI_OPCODE_CMD_VS_ISO_READ_TX_TIMESTAMP,
				   params, sizeof(*params),
				   return_params, sizeof(*return_params));
}

int hci_vs_sdc_big_reserved_time_set(const sdc_hci_cmd_vs_big_reserved_time_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_BIG_RESERVED_TIME_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_cig_reserved_time_set(const sdc_hci_cmd_vs_cig_reserved_time_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CIG_RESERVED_TIME_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_cis_subevent_length_set(const sdc_hci_cmd_vs_cis_subevent_length_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CIS_SUBEVENT_LENGTH_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_scan_channel_map_set(const sdc_hci_cmd_vs_scan_channel_map_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SCAN_CHANNEL_MAP_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_scan_accept_ext_adv_packets_set(
	const sdc_hci_cmd_vs_scan_accept_ext_adv_packets_set_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SCAN_ACCEPT_EXT_ADV_PACKETS_SET,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_set_role_priority(const sdc_hci_cmd_vs_set_role_priority_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SET_ROLE_PRIORITY,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_set_event_start_task(const sdc_hci_cmd_vs_set_event_start_task_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_SET_EVENT_START_TASK,
				 params,
				 sizeof(*params));
}

int hci_vs_sdc_conn_anchor_point_update_event_report_enable(
	const sdc_hci_cmd_vs_conn_anchor_point_update_event_report_enable_t *params)
{
	return hci_vs_cmd_no_rsp(SDC_HCI_OPCODE_CMD_VS_CONN_ANCHOR_POINT_UPDATE_EVENT_REPORT_ENABLE,
				 params,
				 sizeof(*params));
}
