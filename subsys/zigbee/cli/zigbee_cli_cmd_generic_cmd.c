/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include "zigbee_cli.h"
#include "zigbee_cli_utils.h"
#include "zigbee_cli_cmd_zcl.h"


/* Timeout after which packet data entry in ctx manager is invalidated if response not received. */
#define CMD_ENTRY_TIMEOUT_S   10U

/* CMD ID used to mark that frame is to be constructed as `zcl raw`. */
#define ZCL_CMD_ID_RAW        0xFFFF

LOG_MODULE_DECLARE(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

/**@brief Handles timeout error and invalidates ZCL command entry in context manager.
 *
 * @param[in] index   Index of context entry with ZCL command data to invalidate.
 */
static void cmd_zb_zcl_cmd_timeout(zb_uint8_t index)
{
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't get attr entry %d - entry not found", index);
		return;
	}

	zb_cli_print_error(entry->shell, "Request timed out", ZB_FALSE);

	ctx_mgr_delete_entry(entry);
}

/**@brief Function called when command is APS ACKed - prints done and invalidates ZCL command entry
 *        in context manager.
 *
 * @param[in] bufid   Reference to a ZBOSS buffer containing zb_zcl_command_send_status_t data.
 */
static void zb_zcl_cmd_acked(zb_uint8_t bufid)
{
	uint8_t index = 0;
	zb_ret_t zb_err_code = RET_OK;
	zb_zcl_command_send_status_t *cmd_status = NULL;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (bufid == ZB_BUF_INVALID) {
		return;
	}

	cmd_status = ZB_BUF_GET_PARAM(bufid, zb_zcl_command_send_status_t);

	/* Handle only success status. */
	if (cmd_status->status != RET_OK) {
		goto exit;
	}

	/* Find entry of CTX_MGR_GENERIC_CMD_ENTRY_TYPE type with matching buffer id.
	 * When ZCL command is APS ACKed, callback is called with the same buffer id.
	 * This is used to find matching ctx entry.
	 */
	while (entry != NULL) {
		if ((entry->type != CTX_MGR_GENERIC_CMD_ENTRY_TYPE) || (entry->taken != true)) {
			continue;
		}

		if (entry->generic_cmd_data.packet_info.buffer == bufid) {
			break;
		}

		index++;
		entry = ctx_mgr_get_entry_by_index(index);
	}

	if (entry == NULL) {
		LOG_ERR("Couldn't find matching entry for ZCL generic cmd");
		goto exit;
	}

	zb_cli_print_done(entry->shell, ZB_FALSE);

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_zcl_cmd_timeout, index);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(entry->shell, "Unable to cancel timeout timer", ZB_TRUE);
	}

	ctx_mgr_delete_entry(entry);
exit:
	zb_buf_free(bufid);
}

/**@brief Check if the frame we received is the response to our request in the table.
 *
 * @param zcl_hdr           Pointer to the parsed header of the frame.
 * @param entry             Pointer to the entry in the context manager to check against.
 *
 * @return Whether it is response or not.
 */
static zb_bool_t is_response(zb_zcl_parsed_hdr_t *zcl_hdr, struct ctx_entry *entry)
{
	zb_uint16_t remote_node_short = 0;
	struct zcl_packet_info *packet_info = &entry->generic_cmd_data.packet_info;

	if (packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT) {
		remote_node_short = zb_address_short_by_ieee(packet_info->dst_addr.addr_long);
	} else {
		remote_node_short = packet_info->dst_addr.addr_short;
	}

	if (zcl_hdr->cluster_id != packet_info->cluster_id) {
		return ZB_FALSE;
	}

	if (zcl_hdr->profile_id != packet_info->prof_id) {
		return ZB_FALSE;
	}

	if (zcl_hdr->addr_data.common_data.src_endpoint != packet_info->dst_ep) {
		return ZB_FALSE;
	}

	if (zcl_hdr->addr_data.common_data.source.addr_type == ZB_ZCL_ADDR_TYPE_SHORT) {
		if (zcl_hdr->addr_data.common_data.source.u.short_addr != remote_node_short) {
			return ZB_FALSE;
		}
	} else {
		return ZB_FALSE;
	}

	if (zcl_hdr->cmd_id != ZB_ZCL_CMD_DEFAULT_RESP) {
		return ZB_FALSE;
	}

	return ZB_TRUE;
}


/**@brief Function to construct ZCL command in the buffer.
 *        Function to be called in ZBOSS thread context to prevent non thread-safe operations.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 */
static void construct_zcl_cmd_frame(struct ctx_entry *entry)
{
	struct generic_cmd_data *req_data = &(entry->generic_cmd_data);

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	if (req_data->cmd_id != ZCL_CMD_ID_RAW) {
		/* Start filling buffer with packet data. */
		req_data->packet_info.ptr = ZB_ZCL_START_PACKET_REQ(req_data->packet_info.buffer)

		ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(req_data->packet_info.ptr,
								    (req_data->def_resp))
		ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(req_data->packet_info.ptr,
						    ZB_ZCL_GET_SEQ_NUM(),
						    req_data->cmd_id);
	} else {
		/* Start filling buffer with packet data. */
		req_data->packet_info.ptr = ZB_ZCL_START_PACKET(req_data->packet_info.buffer);
	}

	/* Put payload in buffer with command to send. */
	for (zb_uint8_t i = 0; i < req_data->payload_length; i++) {
		ZB_ZCL_PACKET_PUT_DATA8(req_data->packet_info.ptr, (req_data->payload[i]));
	}
}

/**@brief Actually send the ZCL command frame.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_cmd_frame(zb_uint8_t index)
{
	zb_ret_t zb_err_code;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	construct_zcl_cmd_frame(entry);

	packet_info = &(entry->generic_cmd_data.packet_info);

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
		zb_cli_print_error(entry->shell, "Can not send ZCL frame", ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(packet_info->buffer);
		zb_osif_enable_all_inter();

		/* Invalidate an entry with frame data in the context manager. */
		ctx_mgr_delete_entry(entry);
	}
}

/**@brief Function to send the ZCL command frame.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 * @return Error code with function execution status.
 */
static int zcl_cmd_send(struct ctx_entry *entry)
{
	zb_ret_t zb_err_code;
	uint8_t entry_index = ctx_mgr_get_index_by_entry(entry);

	if (entry->generic_cmd_data.def_resp == ZB_ZCL_ENABLE_DEFAULT_RESPONSE) {
		zb_err_code = ZB_SCHEDULE_APP_ALARM(cmd_zb_zcl_cmd_timeout,
						    entry_index,
						    (CMD_ENTRY_TIMEOUT_S * ZB_TIME_ONE_SECOND));
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(entry->shell, "Couldn't schedule timeout cb.", ZB_FALSE);
			goto error;
		}
	}

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zcl_send_cmd_frame, entry_index);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(entry->shell, "Can not schedule zcl frame.", ZB_FALSE);

		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_zcl_cmd_timeout, entry_index);
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(entry->shell, "Unable to cancel timeout timer", ZB_TRUE);
		}
		goto error;
	}

	return 0;

error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(entry->generic_cmd_data.packet_info.buffer);
	zb_osif_enable_all_inter();

	/* Invalidate an entry with frame data in the context manager. */
	ctx_mgr_delete_entry(entry);

	return -ENOEXEC;
}

/**@brief Send generic command to the remote node.
 *
 * @code
 * zcl cmd [-d] <h:dst_addr> <d:ep> <h:cluster> [-p h:profile] <h:cmd_ID>
 *         [-l h:payload]
 * @endcode
 *
 * Send generic command with ID `cmd_ID` with payload `payload` to the cluster
 * `cluster`. The cluster belongs to the profile `profile`, which resides
 * on the endpoint `ep` of the remote node `dst_addr`. Optional default
 * response can be set with `-d`.
 *
 * @note By default profile is set to Home Automation Profile
 * @note By default payload is empty
 * @note To send via binding table, set `dst_addr` and `ep` to 0.
 *
 */
int cmd_zb_generic_cmd(const struct shell *shell, size_t argc, char **argv)
{
	size_t len;
	int ret_val;
	zb_bufid_t bufid;
	struct generic_cmd_data *req_data;
	struct zcl_packet_info *packet_info;
	char **arg = &argv[1];
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_GENERIC_CMD_ENTRY_TYPE);

	ARG_UNUSED(argc);

	if (entry == NULL) {
		zb_cli_print_error(shell, "Request pool empty - wait a bit", ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->generic_cmd_data);
	packet_info = &(req_data->packet_info);

	/*  Set default command values. */
	req_data->def_resp = ZB_ZCL_DISABLE_DEFAULT_RESPONSE;
	req_data->payload_length = 0;
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;

	/* Check if default response should be requested. */
	if (strcmp(*arg, "-d") == 0) {
		req_data->def_resp = ZB_ZCL_ENABLE_DEFAULT_RESPONSE;
		arg++;
	}

	/* Parse and check remote node address. */
	packet_info->dst_addr_mode = parse_address(*arg, &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		/* Handle the Binding table case (address == '0'). */
		if (!strcmp(*arg, "0")) {
			packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
			packet_info->dst_addr.addr_short = 0;
		} else {
			zb_cli_print_error(shell, "Wrong address format", ZB_FALSE);
			goto error;
		}
	}
	arg++;

	/* Read endpoint and cluster id. */
	ret_val = zb_cli_sscan_uint8(*(arg++), &(packet_info->dst_ep));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Remote ep value", ZB_FALSE);
		goto error;
	}

	ret_val = parse_hex_u16(*(arg++), &(packet_info->cluster_id));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Cluster id value", ZB_FALSE);
		goto error;
	}

	/* Check if different from HA profile should be used. */
	if (strcmp(*arg, "-p") == 0) {
		ret_val = parse_hex_u16(*(++arg), &(packet_info->prof_id));
		if (ret_val == 0) {
			zb_cli_print_error(shell, "Profile id value", ZB_FALSE);
			goto error;
		}
		arg++;
	}

	/* Read command id. */
	ret_val = parse_hex_u16(*(arg++), &(req_data->cmd_id));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Cmd id value", ZB_FALSE);
		goto error;
	}

	/* Check if payload should be sent. */
	if (strcmp(*(arg++), "-l") == 0) {
		len = strlen(*arg);
		if (len > (2 * CMD_PAYLOAD_SIZE)) {
			shell_warn(shell,
				   "Payload length is too big, trimming it to first %d bytes",
				   CMD_PAYLOAD_SIZE);
			len = (2 * CMD_PAYLOAD_SIZE);
		}
		if ((len != 0) && (len % 2) != 0) {
			zb_cli_print_error(shell, "Payload length is not even", ZB_FALSE);
			goto error;
		}
		ret_val = parse_hex_str(*arg, len, req_data->payload, CMD_PAYLOAD_SIZE, false);
		if (ret_val == 0) {
			zb_cli_print_error(shell, "Payload value", ZB_FALSE);
			goto error;
		}
		req_data->payload_length = len / 2;
	}

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_cli_get_endpoint();
	/* Profile ID and Cluster ID are already set. */
	/* If no default response requested, set cb function to call when cmd is APS ACKed. */
	if (req_data->def_resp == ZB_ZCL_DISABLE_DEFAULT_RESPONSE) {
		packet_info->cb = zb_zcl_cmd_acked;
	}
	packet_info->disable_aps_ack = ZB_FALSE;

	return zcl_cmd_send(entry);
error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief Send raw ZCL frame.
 *
 * @code
 * zcl raw <h:dst_addr> <d:ep> <h:cluster> <h:profile> <h:payload>
 * @endcode
 *
 * Send raw ZCL frame with payload `payload` to the cluster `cluster`.
 * The cluster belongs to the profile `profile`, which resides
 * on the endpoint `ep` of the remote node `dst_addr`. The payload represents
 * the ZCL Header plus ZCL Payload.
 *
 * @note To send via binding table, set `dst_addr` and `ep` to 0.
 *
 */
int cmd_zb_zcl_raw(const struct shell *shell, size_t argc, char **argv)
{
	size_t len;
	int ret_val;
	zb_bufid_t bufid;
	struct generic_cmd_data *req_data;
	struct zcl_packet_info *packet_info;
	char **arg = &argv[1];
	struct ctx_entry *entry;

	ARG_UNUSED(argc);

	/* Debug mode quick return. */
	if (!zb_cli_debug_get()) {
		zb_cli_print_error(shell,
				   "This command is available only in debug mode. Run 'debug on' to enable it",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	entry = ctx_mgr_new_entry(CTX_MGR_GENERIC_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		zb_cli_print_error(shell, "Request pool empty - wait a bit", ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->generic_cmd_data);
	packet_info = &(req_data->packet_info);

	/*  Reset the counter. */
	req_data->payload_length = 0;

	/* Parse and check remote node address. */
	packet_info->dst_addr_mode = parse_address(*arg, &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		/* Handle the Binding table case (address == '0'). */
		if (!strcmp(*arg, "0")) {
			packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
			packet_info->dst_addr.addr_short = 0;
		} else {
			zb_cli_print_error(shell, "Wrong address format", ZB_FALSE);
			goto error;
		}
	}
	arg++;

	/* Read endpoint and cluster id. */
	ret_val = zb_cli_sscan_uint8(*(arg++), &(packet_info->dst_ep));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Remote ep value", ZB_FALSE);
		goto error;
	}

	ret_val = parse_hex_u16(*(arg++), &(packet_info->cluster_id));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Cluster id value", ZB_FALSE);
		goto error;
	}

	ret_val = parse_hex_u16(*(arg++), &(packet_info->prof_id));
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Profile id value", ZB_FALSE);
		goto error;
	}

	/* Reuse the payload field from the context. */
	len = strlen(*arg);
	if (len > (2 * CMD_PAYLOAD_SIZE)) {
		shell_warn(shell,
			   "Raw data length is too big, trimming it to first %d bytes",
			   CMD_PAYLOAD_SIZE);
		len = (2 * CMD_PAYLOAD_SIZE);
	}

	if ((len != 0) && (len % 2) != 0) {
		zb_cli_print_error(shell, "Payload length is not even", ZB_FALSE);
		goto error;
	}

	ret_val = parse_hex_str(*arg, len, req_data->payload, CMD_PAYLOAD_SIZE, false);
	if (ret_val == 0) {
		zb_cli_print_error(shell, "Payload value", ZB_FALSE);
		goto error;
	}
	req_data->payload_length = len / 2;

	/* Disable default response for zcl raw command. */
	req_data->def_resp = ZB_ZCL_DISABLE_DEFAULT_RESPONSE;

	/* Set CMD ID to ZCL_CMD_ID_RAW to mark this frame as `zcl raw`. */
	req_data->cmd_id = ZCL_CMD_ID_RAW;

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free - delete entry. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_cli_get_endpoint();
	/* Profile ID and Cluster ID are already set. */
	packet_info->cb = zb_zcl_cmd_acked;
	packet_info->disable_aps_ack = ZB_FALSE;

	return zcl_cmd_send(entry);
error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint.
 *
 * @param bufid   Reference to a ZBOSS buffer.
 */
zb_uint8_t cli_agent_ep_handler_generic_cmd(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;
	struct ctx_entry *entry;
	zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);

	/* Find command context by ZCL sequence number. */
	entry = ctx_mgr_find_ctx_entry(cmd_info->seq_number, CTX_MGR_GENERIC_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		return ZB_FALSE;
	}

	if (!is_response(cmd_info, entry)) {
		return ZB_FALSE;
	}

	if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP) {
		zb_zcl_default_resp_payload_t *def_resp = ZB_ZCL_READ_DEFAULT_RESP(bufid);

		/* Print info received from default response. */
		shell_fprintf(entry->shell,
			      (def_resp->status == ZB_ZCL_STATUS_SUCCESS) ?
			       SHELL_INFO : SHELL_ERROR,
			      "Default Response received: ");
		shell_fprintf(entry->shell,
			      (def_resp->status == ZB_ZCL_STATUS_SUCCESS) ?
			       SHELL_INFO : SHELL_ERROR,
			      "Command: %d, Status: %d\n",
			      def_resp->command_id,
			      def_resp->status);

		if (def_resp->status != ZB_ZCL_STATUS_SUCCESS) {
			zb_cli_print_error(entry->shell, "Command not successful", ZB_FALSE);
		} else {
			zb_cli_print_done(entry->shell, ZB_FALSE);
		}
	} else {
		/* In case of unknown response. */
		zb_cli_print_error(entry->shell, "Unknown response", ZB_FALSE);
	}
	/* Cancel the ongoing alarm which was to delete entry in the context manager ... */
	if (entry->generic_cmd_data.def_resp == ZB_ZCL_ENABLE_DEFAULT_RESPONSE) {
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_zcl_cmd_timeout,
							   ctx_mgr_get_index_by_entry(entry));
		ZB_ERROR_CHECK(zb_err_code);

		/* ...and erase it manually. */
		ctx_mgr_delete_entry(entry);
	}
	zb_buf_free(bufid);

	return ZB_TRUE;
}
