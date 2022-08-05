/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <errno.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_shell.h>
#include "zigbee_shell_utils.h"


/* Defines default value for minimum interval inside configure reporting request. */
#define ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_MIN_INTERVAL 1

/* Defines default value for maximum interval inside configure reporting request. */
#define ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_MAX_INTERVAL 60

/* Defines default value for minimum value change inside configure reporting request. */
#define ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_VALUE_CHANGE NULL

/* Defines default value for minimum interval configured in order to turn off reporting.
 * See ZCL specification, sec. 2.5.7.1.5.
 * This can be any value, only max_interval parameters is relevant.
 */
#define ZIGBEE_SHELL_CONFIGURE_REPORT_OFF_MIN_INTERVAL 0x000F

/* Defines default value for maximum interval inside configure reporting request.
 * See ZCL specification, sec. 2.5.7.1.6.
 */
#define ZIGBEE_SHELL_CONFIGURE_REPORT_OFF_MAX_INTERVAL 0xFFFF


LOG_MODULE_REGISTER(zigbee_shell_report, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

/**@brief Prints the Configure Reporting Response.
 *
 * @param[in] entry         Pointer to the entry in the context manager.
 * @param[in] bufid         ZBOSS buffer id
 */
static void cmd_zb_subscribe_unsubscribe_cb(struct ctx_entry *entry, zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;
	zb_bool_t failed = ZB_FALSE;
	zb_zcl_configure_reporting_res_t *resp = NULL;

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(zb_shell_zcl_cmd_timeout_cb, ZB_ALARM_ANY_PARAM);
	ZB_ERROR_CHECK(zb_err_code);

	/* Check if response contains only status code. */
	if (sizeof(zb_zcl_configure_reporting_res_t) > zb_buf_len(bufid)) {
		resp = (zb_zcl_configure_reporting_res_t *)zb_buf_begin(bufid);

		if (resp->status == ZB_ZCL_STATUS_SUCCESS) {
			zb_shell_print_done(entry->shell, ZB_FALSE);
		} else {
			shell_error(entry->shell,
				    "Error: Unable to configure reporting. Status: %d",
				    resp->status);
		}
		goto delete_ctx_entry;
	}

	/* Received a full Configure Reporting Response frame. */
	ZB_ZCL_GENERAL_GET_NEXT_CONFIGURE_REPORTING_RES(bufid, resp);
	if (resp == NULL) {
		zb_shell_print_error(
			entry->shell,
			"Unable to parse configure reporting response",
			ZB_TRUE);
		goto delete_ctx_entry;
	}

	while (resp != NULL) {
		if (resp->status == ZB_ZCL_STATUS_SUCCESS) {
			switch (resp->direction) {
			case ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT:
				shell_print(entry->shell,
					    "Local subscription to attribute ID %hx updated",
					    resp->attr_id);
				break;

			case ZB_ZCL_CONFIGURE_REPORTING_RECV_REPORT:
				shell_print(entry->shell,
					    "Remote node subscription to receive attribute ID %hx updated",
					    resp->attr_id);
				break;

			default:
				shell_error(entry->shell,
					    "Error: Unknown reporting configuration direction for attribute %hx",
					    resp->attr_id);
				failed = ZB_TRUE;
				break;
			}
		} else {
			shell_error(
				entry->shell,
				"Error: Unable to configure attribute %hx reporting. Status: %hd",
				resp->attr_id,
				resp->status);
			failed = ZB_TRUE;
		}
		ZB_ZCL_GENERAL_GET_NEXT_CONFIGURE_REPORTING_RES(bufid, resp);
	}

	if (failed == ZB_TRUE) {
		zb_shell_print_error(
			entry->shell,
			"One or more attributes reporting were not configured successfully",
			ZB_TRUE);
	} else {
		zb_shell_print_done(entry->shell, ZB_FALSE);
	}

delete_ctx_entry:
	zb_buf_free(bufid);
	ctx_mgr_delete_entry(entry);
}

/**@brief Print the Report Attribute Command
 *
 * @param[in] zcl_hdr       Pointer to parsed ZCL header
 * @param[in] bufid         ZBOSS buffer id
 */
static void print_attr_update(zb_zcl_parsed_hdr_t *zcl_hdr, zb_bufid_t bufid)
{
	char print_buf[255];
	int bytes_written = 0;
	zb_zcl_report_attr_req_t *attr_resp = NULL;
	zb_zcl_addr_t remote_node_data = zcl_hdr->addr_data.common_data.source;

	if (remote_node_data.addr_type == ZB_ZCL_ADDR_TYPE_SHORT) {
		LOG_INF("Received value updates from the remote node 0x%04x",
			remote_node_data.u.short_addr);
	} else {
		bytes_written = ieee_addr_to_str(print_buf,
						 sizeof(print_buf),
						 remote_node_data.u.ieee_addr);
		if (bytes_written < 0) {
			LOG_INF("Received value updates from the remote node (unknown address)");
		} else {
			LOG_INF("Received value updates from the remote node 0x%s",
				print_buf);
		}
	}

	/* Get the contents of Read Attribute Response frame. */
	ZB_ZCL_GENERAL_GET_NEXT_REPORT_ATTR_REQ(bufid, attr_resp);
	while (attr_resp != NULL) {
		bytes_written = 0;
		bytes_written =
			zb_shell_zcl_attr_to_str(
				print_buf,
				sizeof(print_buf),
				attr_resp->attr_type,
				attr_resp->attr_value);

		if (bytes_written < 0) {
			LOG_ERR("    Unable to print updated attribute value");
		} else {
			LOG_INF("    Profile: 0x%04x Cluster: 0x%04x Attribute: 0x%04x"
				" Type: %hu Value: %s",
				zcl_hdr->profile_id,
				zcl_hdr->cluster_id,
				attr_resp->attr_id,
				attr_resp->attr_type,
				print_buf);
		}

		ZB_ZCL_GENERAL_GET_NEXT_REPORT_ATTR_REQ(bufid, attr_resp);
	}
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint
 *
 * @param[in] bufid   ZBOSS buffer id.
 *
 * @returns ZB_TRUE if ZCL command was processed.
 */
zb_uint8_t zb_shell_ep_handler_report(zb_bufid_t bufid)
{
	struct ctx_entry *entry;
	zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);

	if (cmd_info->cmd_id == ZB_ZCL_CMD_REPORT_ATTRIB) {
		print_attr_update(cmd_info, bufid);
		zb_buf_free(bufid);
		return ZB_TRUE;

	} else if (cmd_info->cmd_id == ZB_ZCL_CMD_CONFIG_REPORT_RESP) {
		/* Find command context by ZCL sequence number. */
		entry = ctx_mgr_find_ctx_entry(cmd_info->seq_number,
					       CTX_MGR_CFG_REPORT_REQ_ENTRY_TYPE);

		if (entry != NULL) {
			cmd_zb_subscribe_unsubscribe_cb(entry, bufid);
			return ZB_TRUE;
		}
	}

	return ZB_FALSE;
}

/**@brief Function to construct Configure Reporting command in the buffer.
 *        Function to be called in ZBOSS thread context to prevent non thread-safe operations.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 */
static void construct_reporting_frame(struct ctx_entry *entry)
{
	struct configure_reporting_req *req_data = &(entry->zcl_data.configure_reporting_req);

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	/* Construct and send request. */
	ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_SRV_REQ(
		entry->zcl_data.pkt_info.buffer,
		entry->zcl_data.pkt_info.ptr,
		ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
	ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
		entry->zcl_data.pkt_info.ptr,
		req_data->attr_id,
		req_data->attr_type,
		req_data->interval_min,
		req_data->interval_max,
		ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_VALUE_CHANGE);
}

/**@brief Actually send the Configure Reporting frame.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_attr_report_frame(zb_uint8_t index)
{
	zb_ret_t zb_err_code;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send Configure Reporting frame - context entry %d not found",
			index);
		return;
	}

	construct_reporting_frame(entry);

	packet_info = &(entry->zcl_data.pkt_info);

	/* Send the actual frame. */
	zb_err_code = zb_zcl_finish_and_send_packet_new(
				packet_info->buffer,
				packet_info->ptr,
				&(packet_info->dst_addr),
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				packet_info->cluster_id,
				packet_info->cb,
				0,
				packet_info->disable_aps_ack,
				0);

	if (zb_err_code != RET_OK) {
		zb_shell_print_error(entry->shell, "Can not send ZCL frame", ZB_FALSE);
		zb_buf_free(packet_info->buffer);

		/* Invalidate an entry with frame data in the context manager. */
		ctx_mgr_delete_entry(entry);
	}
}

/**@brief Function to send Configure Reporting command.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 * @return Error code with function execution status.
 */
static int send_reporting_frame(struct ctx_entry *entry)
{
	zb_ret_t zb_err_code;
	uint8_t entry_index = ctx_mgr_get_index_by_entry(entry);

	zb_err_code = ZB_SCHEDULE_APP_ALARM(
			zb_shell_zcl_cmd_timeout_cb,
			entry_index,
			(CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT * ZB_TIME_ONE_SECOND));

	if (zb_err_code != RET_OK) {
		zb_shell_print_error(entry->shell, "Couldn't schedule timeout cb.", ZB_FALSE);
		goto error;
	}

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zcl_send_attr_report_frame,
					       entry_index);
	if (zb_err_code != RET_OK) {
		zb_shell_print_error(entry->shell, "Can not schedule zcl frame.", ZB_FALSE);

		zb_err_code =
			ZB_SCHEDULE_APP_ALARM_CANCEL(zb_shell_zcl_cmd_timeout_cb, entry_index);
		ZB_ERROR_CHECK(zb_err_code);
		goto error;
	}

	return 0;

error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(entry->zcl_data.pkt_info.buffer);
	zb_osif_enable_all_inter();

	/* Invalidate an entry with frame data in the context manager. */
	ctx_mgr_delete_entry(entry);

	return -ENOEXEC;
}

/**@brief Validate and parse input arguments `subscribe` command is called with.
 *        Takes the same input as command handler and if parsed correctly,
 *        fills the context manager `entry` with request data.
 */
static int cmd_zb_subscribe_parse_input(size_t argc, char **argv, struct ctx_entry *entry)
{
	struct zcl_packet_info *packet_info = &(entry->zcl_data.pkt_info);
	struct configure_reporting_req *req_data = &(entry->zcl_data.configure_reporting_req);

	packet_info->dst_addr_mode = parse_address(argv[1], &packet_info->dst_addr, ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_shell_print_error(entry->shell, "Invalid remote address", ZB_FALSE);
		return -EINVAL;
	}

	if (!zb_shell_sscan_uint8(argv[2], &(packet_info->dst_ep))) {
		zb_shell_print_error(entry->shell, "Incorrect remote endpoint", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u16(argv[3], &(packet_info->cluster_id))) {
		zb_shell_print_error(entry->shell, "Incorrect cluster ID", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u16(argv[4], &(packet_info->prof_id))) {
		zb_shell_print_error(entry->shell, "Incorrect profile ID", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u16(argv[5], &(req_data->attr_id))) {
		zb_shell_print_error(entry->shell, "Incorrect attribute ID", ZB_FALSE);
		return -EINVAL;
	}

	if (!zb_shell_sscan_uint8(argv[6], &(req_data->attr_type))) {
		zb_shell_print_error(entry->shell, "Incorrect attribute type", ZB_FALSE);
		return -EINVAL;
	}

	/* Optional parameters parsing. */
	if (argc > 7) {
		if (!zb_shell_sscan_uint(argv[7], (uint8_t *)&req_data->interval_min, 2, 10)) {
			zb_shell_print_error(entry->shell, "Incorrect minimum interval", ZB_FALSE);
			return -EINVAL;
		}
	}

	if (argc > 8) {
		if (!zb_shell_sscan_uint(argv[8], (uint8_t *)&req_data->interval_max, 2, 10)) {
			zb_shell_print_error(entry->shell, "Incorrect maximum interval", ZB_FALSE);
			return -EINVAL;
		}
	}

	return 0;
}

/**@brief Subscribe to the attribute changes on the remote node.
 *
 * @code
 * zcl subscribe on <h:addr> <d:ep> <h:cluster> <h:profile> <h:attr_id>
 *                  <d:attr_type> [<d:min_interval (s)>] [<d:max_interval (s)>]
 * @endcode
 *
 * Enable reporting on the node identified by `addr`, with the endpoint `ep`
 * that uses the profile `profile` of the attribute `attr_id` with the type
 * `attr_type` in the cluster `cluster`.
 *
 * Reports must be generated in intervals not shorter than `min_interval`
 * (1 second by default) and not longer
 * than `max_interval` (60 seconds by default).
 */
int cmd_zb_subscribe_on(const struct shell *shell, size_t argc, char **argv)
{
	int ret_val;
	zb_bufid_t bufid;
	struct configure_reporting_req *req_data;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_CFG_REPORT_REQ_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.configure_reporting_req);

	/* Set default interval values. */
	req_data->interval_min = ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_MIN_INTERVAL;
	req_data->interval_max = ZIGBEE_SHELL_CONFIGURE_REPORT_DEFAULT_MAX_INTERVAL;

	/* Set pointer to the shell to be used by the command handler. */
	entry->shell = shell;

	ret_val = cmd_zb_subscribe_parse_input(argc, argv, entry);
	if (ret_val) {
		/* Parsing input arguments failed, error message has been already printed
		 * in the `cmd_zb_subscribe_parse_input`, so free the ctx entry.
		 */
		ctx_mgr_delete_entry(entry);
		return ret_val;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(
			shell,
			"Failed to execute command (buf alloc failed)",
			ZB_FALSE);
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	/* Fill the structure for sending ZCL frame. */
	entry->zcl_data.pkt_info.buffer = bufid;
	/* DstAddr, DstAddr Mode and dst endpoint are already set. */
	entry->zcl_data.pkt_info.ep = zb_shell_get_endpoint();
	/* Profile ID and Cluster ID are already set. */
	entry->zcl_data.pkt_info.cb = NULL;
	entry->zcl_data.pkt_info.disable_aps_ack = ZB_FALSE;

	return send_reporting_frame(entry);
}

/**@brief Unsubscribe from the attribute changes on the remote node.
 *
 * @code
 * zcl subscribe off <h:addr> <d:ep> <h:cluster> <h:profile> <h:attr_id>
 *                   <d:attr_type> [<d:min_interval (s)>] [<d:max_interval (s)>]
 * @endcode
 *
 * Disable reporting on the node identified by `addr`, with the endpoint `ep`
 * that uses the profile `profile` of the attribute `attr_id` with the type
 * `attr_type` in the cluster `cluster`.
 */
int cmd_zb_subscribe_off(const struct shell *shell, size_t argc, char **argv)
{
	int ret_val;
	zb_bufid_t bufid;
	struct configure_reporting_req *req_data;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_CFG_REPORT_REQ_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.configure_reporting_req);

	/* Set default interval values. */
	req_data->interval_min = ZIGBEE_SHELL_CONFIGURE_REPORT_OFF_MIN_INTERVAL;
	req_data->interval_max = ZIGBEE_SHELL_CONFIGURE_REPORT_OFF_MAX_INTERVAL;

	/* Set pointer to the shell to be used by the command handler. */
	entry->shell = shell;

	ret_val = cmd_zb_subscribe_parse_input(argc, argv, entry);
	if (ret_val) {
		/* Parsing input arguments failed, error message has been already printed
		 * in the `cmd_zb_subscribe_parse_input`, so free the ctx entry.
		 */
		ctx_mgr_delete_entry(entry);
		return ret_val;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(
			shell,
			"Failed to execute command (buf alloc failed)",
			ZB_FALSE);
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	/* Fill the structure for sending ZCL frame. */
	entry->zcl_data.pkt_info.buffer = bufid;
	/* DstAddr, DstAddr Mode and dst endpoint are already set. */
	entry->zcl_data.pkt_info.ep = zb_shell_get_endpoint();
	/* Profile ID and Cluster ID are already set. */
	entry->zcl_data.pkt_info.cb = NULL;
	entry->zcl_data.pkt_info.disable_aps_ack = ZB_FALSE;

	return send_reporting_frame(entry);
}
