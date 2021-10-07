/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include "zigbee_cli.h"
#include "zigbee_cli_utils.h"
#include "zigbee_cli_cmd_zcl.h"


/* Timeout after which packet data entry in ctx manager is invalidated if response not received. */
#define ATTRIBUTE_ENTRY_TIMEOUT_S  10


LOG_MODULE_DECLARE(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

/**@brief Invalidate an entry with Read/Write Attributes request data after the timeout.
 *        This function is called as the ZBOSS callback.
 *
 * @param index   Index of context entry with Read/Write Attributes data to invalidate.
 */
static void invalidate_read_write_attr_entry_cb(zb_uint8_t index)
{
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't get attr entry %d - entry not found", index);
		return;
	}

	zb_cli_print_error(entry->shell, "Request timed out", ZB_FALSE);
	ctx_mgr_delete_entry(entry);
}

/**@brief Check if the frame we received is the response to our request in the context manager.
 *
 * @param zcl_hdr           Pointer to the parsed header of the frame.
 * @param entry             Pointer to the entry in the context manager to check against.
 *
 * @return Whether it is response or not.
 */
static zb_bool_t is_response(zb_zcl_parsed_hdr_t *zcl_hdr, struct ctx_entry *entry)
{
	zb_uint16_t remote_node_short = 0;
	struct zcl_packet_info *packet_info = &(entry->read_write_attr_req_data.packet_info);

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

	if (zcl_hdr->cmd_id != ZB_ZCL_CMD_DEFAULT_RESP &&
	    zcl_hdr->cmd_id != ZB_ZCL_CMD_READ_ATTRIB_RESP &&
	    zcl_hdr->cmd_id != ZB_ZCL_CMD_WRITE_ATTRIB_RESP) {
		return ZB_FALSE;
	}

	return ZB_TRUE;
}

/**@brief Print the Read Attribute Response.
 *
 * @param bufid             Zigbee buffer ID with Read Attribute Response packet.
 * @param entry             Pointer to the entry in the context manager.
 */
static void print_read_attr_response(zb_bufid_t bufid, struct ctx_entry *entry)
{
	zb_zcl_read_attr_res_t *attr_resp;

	/* Get the contents of Read Attribute Response frame. */
	ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(bufid, attr_resp);
	if (attr_resp->status == ZB_ZCL_STATUS_SUCCESS) {
		char attr_buf[40];
		int bytes_written = zb_cli_zcl_attr_to_str(attr_buf,
							   sizeof(attr_buf),
							   attr_resp->attr_type,
							   attr_resp->attr_value);

		if (bytes_written < 0) {
			zb_cli_print_error(entry->shell,
					   "Unable to print attribute value",
					   ZB_TRUE);
		} else {
			shell_print(entry->shell,
				    "ID: %d Type: %x Value: %s",
				    attr_resp->attr_id,
				    attr_resp->attr_type,
				    attr_buf);
			zb_cli_print_done(entry->shell, ZB_FALSE);
		}
	} else {
		shell_print(entry->shell, "Error: Status %d", attr_resp->status);
	}
}

/**@brief Print the Write Attribute Response.
 *
 * @param bufid             Zigbee buffer ID with Write Attribute Response packet.
 * @param entry             Pointer to the entyr in the context manager.
 */
static void print_write_attr_response(zb_bufid_t bufid, struct ctx_entry *entry)
{
	zb_zcl_write_attr_res_t *attr_resp;

	/* Get the contents of Write Attribute Response frame. */
	ZB_ZCL_GET_NEXT_WRITE_ATTR_RES(bufid, attr_resp);

	if (!attr_resp) {
		zb_cli_print_error(entry->shell, "No attribute could be retrieved", ZB_TRUE);
		return;
	}

	if (attr_resp->status != ZB_ZCL_STATUS_SUCCESS) {
		shell_print(entry->shell, "Error: Status %d", attr_resp->status);
		return;
	}

	zb_cli_print_done(entry->shell, ZB_FALSE);
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint.
 *
 * @param bufid   ZBOSS buffer id.
 */
zb_uint8_t cli_agent_ep_handler_attr(zb_bufid_t bufid)
{
	zb_zcl_parsed_hdr_t *zcl_hdr;
	struct ctx_entry *entry;

	zcl_hdr = ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);

	/* Get the entry in the context manager according by the seq number. */
	entry = ctx_mgr_find_ctx_entry(zcl_hdr->seq_number, CTX_MGR_ATTR_REQ_ENTRY_TYPE);

	if (entry == NULL) {
		return ZB_FALSE;
	}

	if (!is_response(zcl_hdr, entry)) {
		return ZB_FALSE;
	}

	if (zcl_hdr->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP) {
		zb_zcl_default_resp_payload_t *def_resp;

		def_resp = ZB_ZCL_READ_DEFAULT_RESP(bufid);
		shell_error(entry->shell, "Error: Default Response received; ");
		shell_error(entry->shell,
			    "Command: %d, Status: %d ",
			    def_resp->command_id,
			    def_resp->status);
	} else {
		if (entry->read_write_attr_req_data.req_type == ATTR_READ_REQUEST) {
			print_read_attr_response(bufid, entry);
		} else {
			print_write_attr_response(bufid, entry);
		}
	}
	/* Cancel the ongoing alarm which was to delete entry with frame data ... */
	(void)(ZB_SCHEDULE_APP_ALARM_CANCEL(invalidate_read_write_attr_entry_cb,
					    ctx_mgr_get_index_by_entry(entry)));
	/* ... and delete it manually. */
	ctx_mgr_delete_entry(entry);

	zb_buf_free(bufid);

	return ZB_TRUE;
}

/**@brief Function to construct Read/Write attribute command in the buffer.
 *        Function to be called in ZBOSS thread context to prevent non thread-safe operations.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 */
static void construct_read_write_attr_frame(struct ctx_entry *entry)
{
	struct read_write_attr_req_data *req_data = &(entry->read_write_attr_req_data);

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	/* Fill the buffer with data depending on the request type. */
	if (req_data->req_type == ATTR_READ_REQUEST) {
		ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ_A(req_data->packet_info.buffer,
						    req_data->packet_info.ptr,
						    req_data->direction,
						    ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
		ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(req_data->packet_info.ptr,
						    req_data->attr_id);
	} else {
		ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ_A(req_data->packet_info.buffer,
						     req_data->packet_info.ptr,
						     req_data->direction,
						     ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
		ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(req_data->packet_info.ptr,
							req_data->attr_id,
							req_data->attr_type,
							req_data->attr_value);
	}
}

/**@brief Actually send the Read or Write Attribute frame.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_read_write_attr_frame(zb_uint8_t index)
{
	zb_ret_t zb_err_code;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send ping frame - context entry %d not found", index);
		return;
	}

	construct_read_write_attr_frame(entry);

	packet_info = &(entry->read_write_attr_req_data.packet_info);

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

/**@brief Actually construct the Read or Write Attribute frame.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 * @return Error code with function execution status.
 */
static int read_write_attr_send(struct ctx_entry *entry)
{
	zb_ret_t zb_err_code;
	uint8_t entry_index = ctx_mgr_get_index_by_entry(entry);

	zb_err_code = ZB_SCHEDULE_APP_ALARM(invalidate_read_write_attr_entry_cb,
					    entry_index,
					    (ATTRIBUTE_ENTRY_TIMEOUT_S * ZB_TIME_ONE_SECOND));
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(entry->shell,
				   "Couldn't schedule timeout cb.",
				   ZB_FALSE);
		goto error;
	}

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zcl_send_read_write_attr_frame, entry_index);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(entry->shell, "Can not schedule zcl frame.", ZB_FALSE);

		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(invalidate_read_write_attr_entry_cb,
							   entry_index);
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(entry->shell, "Unable to cancel timeout timer", ZB_TRUE);
		}
		goto error;
	}

	return 0;

error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(entry->read_write_attr_req_data.packet_info.buffer);
	zb_osif_enable_all_inter();

	/* Invalidate an entry with frame data in the context manager. */
	ctx_mgr_delete_entry(entry);

	return -ENOEXEC;
}

/**@brief Retrieve the attribute value of the remote node.
 *
 * @code
 * zcl attr read <h:dst_addr> <d:ep> <h:cluster> [-c] <h:profile> <h:attr_id>
 * @endcode
 *
 * Read the value of the attribute `attr_id` in the cluster `cluster`.
 * The cluster belongs to the profile `profile`, which resides on the endpoint
 * `ep` of the remote node `dst_addr`. If the attribute is on the client role
 * side of the cluster, use the `-c` switch.
 */
int cmd_zb_readattr(const struct shell *shell, size_t argc, char **argv)
{
	bool is_direction_present;
	zb_bufid_t bufid;
	struct zcl_packet_info *packet_info;
	struct read_write_attr_req_data *req_data;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_ATTR_REQ_ENTRY_TYPE);

	is_direction_present = ((argc == 7) && !strcmp(argv[4], "-c"));

	if (argc != 6 && !is_direction_present) {
		zb_cli_print_error(shell, "Wrong number of arguments", ZB_FALSE);
		return -EINVAL;
	}

	if (entry == NULL) {
		zb_cli_print_error(shell, "Request pool empty - wait a bit", ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->read_write_attr_req_data);
	packet_info = &(req_data->packet_info);

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_cli_print_error(shell, "Invalid address", ZB_FALSE);
		return -EINVAL;
	}

	(void)(zb_cli_sscan_uint8(*(++argv), &(packet_info->dst_ep)));

	if (!parse_hex_u16(*(++argv), &(packet_info->cluster_id))) {
		zb_cli_print_error(shell, "Invalid cluster id", ZB_FALSE);
		return -EINVAL;
	}

	if (is_direction_present) {
		req_data->direction = ZB_ZCL_FRAME_DIRECTION_TO_CLI;
		++argv;
	} else {
		req_data->direction = ZB_ZCL_FRAME_DIRECTION_TO_SRV;
	}

	if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
		zb_cli_print_error(shell, "Invalid profile id", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u16(*(++argv), &(req_data->attr_id))) {
		zb_cli_print_error(shell, "Invalid attribute id", ZB_FALSE);
		return -EINVAL;
	}

	req_data->req_type = ATTR_READ_REQUEST;

	/* Put the shell instance to be used later. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)", ZB_FALSE);
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	/* Fill the structure for sending ZCL frame. */
	packet_info->buffer = bufid;
	/* DstAddr, DstAddr Mode, Destination endpoint are already set. */
	packet_info->ep = zb_cli_get_endpoint();
	packet_info->disable_aps_ack = ZB_FALSE;

	return read_write_attr_send(entry);
}

/**@brief Write the attribute value to the remote node.
 *
 * @code
 * zcl attr write <h:dst_addr> <d:ep> <h:cluster> <h:profile> <h:attr_id>
 *                <h:attr_type> <h:attr_value>
 * @endcode
 *
 * Write the `attr_value` value of the attribute `attr_id` of the type
 * `attr_type` in the cluster `cluster`. The cluster belongs to the profile
 * `profile`, which resides on the endpoint `ep` of the remote node `dst_addr`.
 *
 * @note The `attr_value` value must be in hexadecimal format, unless it is a
 * string (`attr_type == 42`), then it must be a string.
 *
 */
int cmd_zb_writeattr(const struct shell *shell, size_t argc, char **argv)
{
	zb_bufid_t bufid;
	struct zcl_packet_info *packet_info;
	struct read_write_attr_req_data *req_data;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_ATTR_REQ_ENTRY_TYPE);

	bool is_direction_present = ((argc == 9) && !strcmp(argv[4], "-c"));

	if (argc != 8 && !is_direction_present) {
		zb_cli_print_error(shell, "Wrong number of arguments", ZB_FALSE);
		return -EINVAL;
	}

	if (entry == NULL) {
		zb_cli_print_error(shell, "Request pool empty - wait a bit", ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->read_write_attr_req_data);
	packet_info = &(req_data->packet_info);

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_cli_print_error(shell, "Invalid address", ZB_FALSE);
		return -EINVAL;
	}

	(void)(zb_cli_sscan_uint8(*(++argv), &(packet_info->dst_ep)));

	if (!parse_hex_u16(*(++argv), &(packet_info->cluster_id))) {
		zb_cli_print_error(shell, "Invalid cluster id", ZB_FALSE);
		return -EINVAL;
	}

	if (is_direction_present) {
		req_data->direction = ZB_ZCL_FRAME_DIRECTION_TO_CLI;
		++argv;
	} else {
		req_data->direction = ZB_ZCL_FRAME_DIRECTION_TO_SRV;
	}

	if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
		zb_cli_print_error(shell, "Invalid profile id", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u16(*(++argv), &(req_data->attr_id))) {
		zb_cli_print_error(shell, "Invalid attribute id", ZB_FALSE);
		return -EINVAL;
	}

	if (!parse_hex_u8(*(++argv), &(req_data->attr_type))) {
		zb_cli_print_error(shell, "Invalid attribute type", ZB_FALSE);
		return -EINVAL;
	}

	uint8_t len = strlen(*(++argv));

	if (req_data->attr_type == ZB_ZCL_ATTR_TYPE_CHAR_STRING) {
		req_data->attr_value[0] = len;
		strncpy((zb_char_t *)(req_data->attr_value + 1),
			*argv,
			sizeof(req_data->attr_value) - 1);
	} else if (!parse_hex_str(*argv, len, req_data->attr_value,
				  sizeof(req_data->attr_value), true)) {

		zb_cli_print_error(shell, "Invalid attribute value", ZB_FALSE);
		return -EINVAL;
	}

	req_data->req_type = ATTR_WRITE_REQUEST;
	/* Put the shell instance to be used later. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)", ZB_FALSE);
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	/* Fill the structure for sending ZCL frame. */
	packet_info->buffer = bufid;
	/* DstAddr, DstAddr Mode, Destination endpoint are already set. */
	packet_info->ep = zb_cli_get_endpoint();
	packet_info->disable_aps_ack = ZB_FALSE;

	return read_write_attr_send(entry);
}
