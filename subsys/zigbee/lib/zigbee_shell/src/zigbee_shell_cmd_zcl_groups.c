/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_shell.h>
#include "zigbee_shell_utils.h"

LOG_MODULE_DECLARE(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);


/**@brief Function to construct and send Add Group command.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_add_group_cmd(zb_uint8_t index)
{
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	packet_info = &(entry->zcl_data.pkt_info);

	ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(
				packet_info->buffer,
				packet_info->dst_addr,
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
				packet_info->cb,
				entry->zcl_data.groups_cmd.group_id);
}

/**@brief Function to construct and send Remove Group command.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_remove_group_cmd(zb_uint8_t index)
{
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	packet_info = &(entry->zcl_data.pkt_info);

	ZB_ZCL_GROUPS_SEND_REMOVE_GROUP_REQ(
				packet_info->buffer,
				packet_info->dst_addr,
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
				packet_info->cb,
				entry->zcl_data.groups_cmd.group_id);
}

/**@brief Function to construct and send Get Group Membership command.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_get_group_mem_cmd(zb_uint8_t index)
{
	zb_uint8_t *data_ptr;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	packet_info = &(entry->zcl_data.pkt_info);

	ZB_ZCL_GROUPS_INIT_GET_GROUP_MEMBERSHIP_REQ(
				packet_info->buffer,
				data_ptr,
				ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
				entry->zcl_data.groups_cmd.group_list_cnt);

	for (uint8_t i = 0; i < entry->zcl_data.groups_cmd.group_list_cnt; i++) {
		ZB_ZCL_GROUPS_ADD_ID_GET_GROUP_MEMBERSHIP_REQ(
				data_ptr,
				entry->zcl_data.groups_cmd.group_list[i]);
	}

	ZB_ZCL_GROUPS_SEND_GET_GROUP_MEMBERSHIP_REQ(
				packet_info->buffer,
				data_ptr,
				packet_info->dst_addr,
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				packet_info->cb);
}

/**@brief Function to construct and send Add Group If Identifying command.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_add_group_if_identifying_cmd(zb_uint8_t index)
{
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	packet_info = &(entry->zcl_data.pkt_info);

	ZB_ZCL_GROUPS_SEND_ADD_GROUP_IF_IDENT_REQ(
				packet_info->buffer,
				packet_info->dst_addr,
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
				packet_info->cb,
				entry->zcl_data.groups_cmd.group_id);
}

/**@brief Function to construct and send Remove All Groups command.
 *
 * @param index   Index of the entry with frame to send in the context manager.
 */
static void zb_zcl_send_remove_all_groups_cmd(zb_uint8_t index)
{
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't send the ZCL command frame - context entry %d not found", index);
		return;
	}

	/* Get the ZCL packet sequence number. */
	entry->id = ZCL_CTX().seq_number;

	packet_info = &(entry->zcl_data.pkt_info);

	ZB_ZCL_GROUPS_SEND_REMOVE_ALL_GROUPS_REQ(
				packet_info->buffer,
				packet_info->dst_addr,
				packet_info->dst_addr_mode,
				packet_info->dst_ep,
				packet_info->ep,
				packet_info->prof_id,
				ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
				packet_info->cb);
}

/**@brief Function to send the ZCL groups cmds.
 *
 * @param entry   Pointer to the entry with frame to send in the context manager.
 *
 * @return Error code with function execution status.
 */
static int zcl_groups_cmd_send(struct ctx_entry *entry)
{
	zb_ret_t zb_err_code;
	uint8_t entry_index = ctx_mgr_get_index_by_entry(entry);

	zb_err_code =
		ZB_SCHEDULE_APP_ALARM(
			zb_shell_zcl_cmd_timeout_cb,
			entry_index,
			(CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT * ZB_TIME_ONE_SECOND));

	if (zb_err_code != RET_OK) {
		zb_shell_print_error(entry->shell, "Couldn't schedule timeout cb.", ZB_FALSE);
		goto cmd_send_error;
	}

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(entry->zcl_data.groups_cmd.send_fn, entry_index);

	if (zb_err_code != RET_OK) {
		zb_shell_print_error(entry->shell, "Can not schedule zcl frame.", ZB_FALSE);

		zb_err_code =
			ZB_SCHEDULE_APP_ALARM_CANCEL(zb_shell_zcl_cmd_timeout_cb, entry_index);
		ZB_ERROR_CHECK(zb_err_code);
		goto cmd_send_error;
	}

	return 0;

cmd_send_error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(entry->zcl_data.pkt_info.buffer);
	zb_osif_enable_all_inter();

	/* Invalidate an entry with frame data in the context manager. */
	ctx_mgr_delete_entry(entry);

	return -ENOEXEC;
}

/**@brief Function called when command is APS ACKed - prints done and invalidates groups command
 *        entry in context manager.
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

	/* Find entry of CTX_MGR_GROUPS_CMD_ENTRY_TYPE type with matching buffer id. */
	entry = ctx_mgr_find_zcl_entry_by_bufid(bufid, CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		LOG_ERR("Couldn't find matching entry for ZCL groups cmd");
		goto exit;
	}

	zb_shell_print_done(entry->shell, ZB_FALSE);

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(zb_shell_zcl_cmd_timeout_cb, index);
	ZB_ERROR_CHECK(zb_err_code);

	ctx_mgr_delete_entry(entry);
exit:
	zb_buf_free(bufid);
}

/**@brief Function to check if command is to be sent as unicast.
 *
 * @param[in] packet_info   Pointer to structure with packet data.
 *
 * @return ZB_TRUE if command is sent as unicast, ZB_FALSE otherwise.
 */
static zb_bool_t is_sent_unicast(struct zcl_packet_info *packet_info)
{
	switch (packet_info->dst_addr_mode) {
	case ADDR_LONG:
		return ZB_TRUE;
	case ADDR_SHORT:
		if (ZB_NWK_IS_ADDRESS_BROADCAST(packet_info->dst_addr.addr_short)) {
			return ZB_FALSE;
		} else if ((packet_info->dst_ep < ZB_MIN_ENDPOINT_NUMBER) ||
		    (packet_info->dst_ep == ZB_APS_BROADCAST_ENDPOINT_NUMBER)) {
			return ZB_FALSE;
		} else {
			return ZB_TRUE;
		}
	default:
		return ZB_FALSE;
	}
}

/**@brief Add remote node to the group or remove from the group.
 *
 * @code
 * zcl group add <h:dst_addr> <d:ep> [-p h:profile] <h:group_id>
 * @endcode
 *
 * Send Add Group command to the endpoint `ep` of the remote node `dst_addr`
 * and add it to the group `group_id`.
 * Providing profile ID is optional, Home Automation Profile is used by default.
 *
 * @code
 * zcl group remove <h:dst_addr> <d:ep> [-p h:profile] <h:group_id>
 * @endcode
 *
 * Send Remove Group command to the endpoint `ep` of the remote node `dst_addr`
 * and remove it from the group `group_id`.
 * Providing profile ID is optional, Home Automation Profile is used by default.
 *
 * @note These commands can be sent as groupcast. To send command as groupcast,
 *       set `dst_addr` to group address and `ep` to 0.
 *
 * @note Group name is not supported.
 */
int cmd_zb_add_remove_group(const struct shell *shell, size_t argc, char **argv)
{
	zb_bufid_t bufid;
	struct groups_cmd *req_data;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.groups_cmd);
	packet_info = &(entry->zcl_data.pkt_info);

	/*  Set default profile ID to Home Automation. */
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	/* Set function to be used to send Add Group or Remove Group command. */
	if (strcmp(*(argv), "add") == 0) {
		req_data->send_fn = zb_zcl_send_add_group_cmd;
	} else if (strcmp(*(argv), "remove") == 0) {
		req_data->send_fn = zb_zcl_send_remove_group_cmd;
	} else {
		zb_shell_print_error(entry->shell, "Failed to parse command name", ZB_FALSE);
		goto add_remove_group_error;
	}

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_shell_print_error(shell, "Invalid address", ZB_FALSE);
		goto add_remove_group_error;
	}

	if (!zb_shell_sscan_uint8(*(++argv), &(packet_info->dst_ep))) {
		zb_shell_print_error(shell, "Incorrect remote endpoint", ZB_FALSE);
		goto add_remove_group_error;
	}

	/* Handle sending to group address. */
	if ((packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
	    && (packet_info->dst_ep == 0)) {
		zb_uint16_t group_addr = packet_info->dst_addr.addr_short;

		/* Verify group address. */
		if ((group_addr < ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MIN_VALUE) ||
		    (group_addr > ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MAX_VALUE)) {
			zb_shell_print_error(shell, "Incorrect group address", ZB_FALSE);
			goto add_remove_group_error;
		}

		packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
	}

	/* Check if different from HA profile should be used. */
	if (strcmp(*(++argv), "-p") == 0) {
		/* Check if argument with group id is present. */
		if (argc < 6) {
			zb_shell_print_error(shell, "Invalid number of arguments", ZB_FALSE);
			goto add_remove_group_error;
		}

		if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
			zb_shell_print_error(shell, "Invalid profile id", ZB_FALSE);
			goto add_remove_group_error;
		}
		argv++;
	}

	if (!parse_hex_u16(*argv, &(req_data->group_id))) {
		zb_shell_print_error(shell, "Invalid group id", ZB_FALSE);
		goto add_remove_group_error;
	}

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_shell_get_endpoint();
	/* Profile ID is already set, Cluster ID does not have to be. */
	if (is_sent_unicast(packet_info) == ZB_TRUE) {
		packet_info->cb = NULL;
		packet_info->disable_aps_ack = ZB_FALSE;
	} else {
		packet_info->cb = zb_zcl_cmd_acked;
		packet_info->disable_aps_ack = ZB_TRUE;
	}

	return zcl_groups_cmd_send(entry);

add_remove_group_error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief Get group membership from the remote node.
 *
 * @code
 * zcl group get_member <h:dst_addr> <d:ep> [-p h:profile] [h:group IDs ...]
 * @endcode
 *
 * Send Get Group Membership command to the endpoint `ep` of the remote node `dst_addr`
 * to get which groups from provided group list the remote node is part of.
 * If no group ID is provided, the remote node is requested to response with information about
 * all groups it is part of.
 * Providing profile ID is optional, Home Automation Profile is used by default.
 *
 * @note Response from the remote node is parsed and printed only if the command was sent
 *       as unicast.
 *
 * @note This command can be sent as groupcast. To send command as groupcast,
 *       set `dst_addr` to group address and `ep` to 0.
 */
int cmd_zb_get_group_mem(const struct shell *shell, size_t argc, char **argv)
{
	zb_bufid_t bufid;
	struct groups_cmd *req_data;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.groups_cmd);
	packet_info = &(entry->zcl_data.pkt_info);

	/*  Set default profile ID to Home Automation. */
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	/* Set function to be used to send Get Group Membership command. */
	req_data->send_fn = zb_zcl_send_get_group_mem_cmd;

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);
	argc--;

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_shell_print_error(shell, "Invalid address", ZB_FALSE);
		goto get_group_mem_error;
	}

	if (!zb_shell_sscan_uint8(*(++argv), &(packet_info->dst_ep))) {
		zb_shell_print_error(shell, "Incorrect remote endpoint", ZB_FALSE);
		goto get_group_mem_error;
	}
	argc--;

	/* Handle sending to group address. */
	if ((packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
	    && (packet_info->dst_ep == 0)) {
		zb_uint16_t group_addr = packet_info->dst_addr.addr_short;

		/* Verify group address. */
		if ((group_addr < ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MIN_VALUE) ||
		    (group_addr > ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MAX_VALUE)) {
			zb_shell_print_error(shell, "Incorrect group address", ZB_FALSE);
			goto get_group_mem_error;
		}

		packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
	}

	/* Check if different from HA profile should be used. */
	if ((argc > 1) && (strcmp(*(++argv), "-p") == 0)) {
		/* Check if argument with profile ID is present. */
		if (argc < 3) {
			zb_shell_print_error(shell, "Invalid number of arguments", ZB_FALSE);
			goto get_group_mem_error;
		}

		if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
			zb_shell_print_error(shell, "Invalid profile id", ZB_FALSE);
			goto get_group_mem_error;
		}
		argv++;
		argc -= 2;
	}

	/* Check if any group ID was provided and if so, parse list group ID. */
	if (argc == 1) {
		req_data->group_list_cnt = 0;
	} else {
		req_data->group_list_cnt = (argc - 1);
	}

	for (uint8_t i = 0; i < req_data->group_list_cnt; i++) {
		if (!parse_hex_u16(*argv, &(req_data->group_list[i]))) {
			zb_shell_print_error(shell, "Invalid group id", ZB_FALSE);
			goto get_group_mem_error;
		}
		argv++;
	}

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_shell_get_endpoint();
	/* Profile ID is already set, Cluster ID does not have to be. */
	if (is_sent_unicast(packet_info) == ZB_TRUE) {
		packet_info->cb = NULL;
		packet_info->disable_aps_ack = ZB_FALSE;
	} else {
		packet_info->cb = zb_zcl_cmd_acked;
		packet_info->disable_aps_ack = ZB_TRUE;
	}

	return zcl_groups_cmd_send(entry);

get_group_mem_error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief Add remote nodes to the group if the remote nodes are in the Identify mode.
 *
 * @code
 * zcl group add_if_identify <h:dst_addr> <d:ep> [-p h:profile] <h:group_id>
 * @endcode
 *
 * Send Add Group If Identifying command to the endpoint `ep` of the remote node `dst_addr`
 * and add it to the group `group_id` if the remote node is in the Identify mode.
 * Providing profile ID is optional, Home Automation Profile is used by default.
 *
 * @note This command can be sent as groupcast. To send command as groupcast,
 *       set `dst_addr` to group address and `ep` to 0.
 */
int cmd_zb_add_group_if_identifying(const struct shell *shell, size_t argc, char **argv)
{
	zb_bufid_t bufid;
	struct groups_cmd *req_data;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.groups_cmd);
	packet_info = &(entry->zcl_data.pkt_info);

	/*  Set default profile ID to Home Automation. */
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	/* Set function to be used to send Add Group If Identifying command. */
	req_data->send_fn = zb_zcl_send_add_group_if_identifying_cmd;

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_shell_print_error(shell, "Invalid address", ZB_FALSE);
		goto add_group_if_identifying_error;
	}

	if (!zb_shell_sscan_uint8(*(++argv), &(packet_info->dst_ep))) {
		zb_shell_print_error(shell, "Incorrect remote endpoint", ZB_FALSE);
		goto add_group_if_identifying_error;
	}

	/* Handle sending to group address. */
	if ((packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
	    && (packet_info->dst_ep == 0)) {
		zb_uint16_t group_addr = packet_info->dst_addr.addr_short;

		/* Verify group address. */
		if ((group_addr < ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MIN_VALUE) ||
		    (group_addr > ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MAX_VALUE)) {
			zb_shell_print_error(shell, "Incorrect group address", ZB_FALSE);
			goto add_group_if_identifying_error;
		}

		packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
	}

	/* Check if different from HA profile should be used. */
	if (strcmp(*(++argv), "-p") == 0) {
		/* Check if argument with group id is present. */
		if (argc < 6) {
			zb_shell_print_error(shell, "Invalid number of arguments", ZB_FALSE);
			goto add_group_if_identifying_error;
		}

		if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
			zb_shell_print_error(shell, "Invalid profile id", ZB_FALSE);
			goto add_group_if_identifying_error;
		}
		argv++;
	}

	if (!parse_hex_u16(*argv, &(req_data->group_id))) {
		zb_shell_print_error(shell, "Invalid group id", ZB_FALSE);
		goto add_group_if_identifying_error;
	}

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_shell_get_endpoint();
	/* Profile ID is already set, Cluster ID does not have to be. */
	packet_info->cb = zb_zcl_cmd_acked;
	if (is_sent_unicast(packet_info) == ZB_TRUE) {
		packet_info->disable_aps_ack = ZB_FALSE;
	} else {
		packet_info->disable_aps_ack = ZB_TRUE;
	}

	return zcl_groups_cmd_send(entry);

add_group_if_identifying_error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief Remove the remote node from all groups.
 *
 * @code
 * zcl group remove_all <h:dst_addr> <d:ep> [-p h:profile]
 * @endcode
 *
 * Send Remove All Groups command to the endpoint `ep` of the remote node `dst_addr`.
 * Providing profile ID is optional, Home Automation Profile is used by default.
 *
 * @note This command can be sent as groupcast. To send command as groupcast,
 *       set `dst_addr` to group address and `ep` to 0.
 */
int cmd_zb_remove_all_groups(const struct shell *shell, size_t argc, char **argv)
{
	zb_bufid_t bufid;
	struct groups_cmd *req_data;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *entry = ctx_mgr_new_entry(CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		zb_shell_print_error(
			shell,
			"Request pool empty - wait for ongoing command to finish",
			ZB_FALSE);
		return -ENOEXEC;
	}

	req_data = &(entry->zcl_data.groups_cmd);
	packet_info = &(entry->zcl_data.pkt_info);

	/*  Set default profile ID to Home Automation. */
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	/* Set function to be used to send Remove All groups command. */
	req_data->send_fn = zb_zcl_send_remove_all_groups_cmd;

	packet_info->dst_addr_mode = parse_address(*(++argv), &(packet_info->dst_addr), ADDR_ANY);

	if (packet_info->dst_addr_mode == ADDR_INVALID) {
		zb_shell_print_error(shell, "Invalid address", ZB_FALSE);
		goto remove_all_groups_error;
	}

	if (!zb_shell_sscan_uint8(*(++argv), &(packet_info->dst_ep))) {
		zb_shell_print_error(shell, "Incorrect remote endpoint", ZB_FALSE);
		goto remove_all_groups_error;
	}

	/* Handle sending to group address. */
	if ((packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
	    && (packet_info->dst_ep == 0)) {
		zb_uint16_t group_addr = packet_info->dst_addr.addr_short;

		/* Verify group address. */
		if ((group_addr < ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MIN_VALUE) ||
		    (group_addr > ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_MAX_VALUE)) {
			zb_shell_print_error(shell, "Incorrect group address", ZB_FALSE);
			goto remove_all_groups_error;
		}

		packet_info->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
	}

	/* Check if different from HA profile should be used. */
	if (strcmp(*(++argv), "-p") == 0) {
		/* Check if argument with group id is present. */
		if (argc < 5) {
			zb_shell_print_error(shell, "Invalid number of arguments", ZB_FALSE);
			goto remove_all_groups_error;
		}

		if (!parse_hex_u16(*(++argv), &(packet_info->prof_id))) {
			zb_shell_print_error(shell, "Invalid profile id", ZB_FALSE);
			goto remove_all_groups_error;
		}
		argv++;
	}

	/* Set shell to print logs to. */
	entry->shell = shell;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_shell_print_error(entry->shell, "Can not get buffer.", ZB_FALSE);
		/* Mark data structure as free. */
		ctx_mgr_delete_entry(entry);
		return -ENOEXEC;
	}

	packet_info->buffer = bufid;
	/* DstAddr, Dst Addr Mode and Dst endpoint are already set. */
	packet_info->ep = zb_shell_get_endpoint();
	/* Profile ID is already set, Cluster ID does not have to be. */
	packet_info->cb = zb_zcl_cmd_acked;
	if (is_sent_unicast(packet_info) == ZB_TRUE) {
		packet_info->disable_aps_ack = ZB_FALSE;
	} else {
		packet_info->disable_aps_ack = ZB_TRUE;
	}

	return zcl_groups_cmd_send(entry);

remove_all_groups_error:
	/* Mark data structure as free. */
	ctx_mgr_delete_entry(entry);
	return -EINVAL;
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint.
 *
 * @param bufid   Reference to a ZBOSS buffer.
 *
 * @return ZB_TRUE if command was handled by the function, ZB_FALSE otherwise.
 */
zb_uint8_t zb_shell_ep_handler_groups_cmd(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;
	struct ctx_entry *entry;
	zb_uint16_t remote_node_short_add;
	struct zcl_packet_info *packet_info;
	zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);

	/* Find command context by ZCL sequence number. */
	entry = ctx_mgr_find_ctx_entry(cmd_info->seq_number, CTX_MGR_GROUPS_CMD_ENTRY_TYPE);

	if (entry == NULL) {
		return ZB_FALSE;
	}

	if (!zb_shell_is_zcl_cmd_response(cmd_info, entry)) {
		return ZB_FALSE;
	}

	packet_info = &entry->zcl_data.pkt_info;

	/* Get sender device short address for printing the response command. */
	if (packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT) {
		remote_node_short_add = zb_address_short_by_ieee(packet_info->dst_addr.addr_long);
	} else {
		remote_node_short_add = packet_info->dst_addr.addr_short;
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
			zb_shell_print_error(entry->shell, "Command not successful", ZB_FALSE);
		}
	} else if (cmd_info->cmd_id == ZB_ZCL_CMD_GROUPS_ADD_GROUP_RES) {
		zb_zcl_groups_add_group_res_t *add_group_resp;

		ZB_ZCL_GROUPS_GET_ADD_GROUP_RES(bufid, add_group_resp);

		if (add_group_resp == NULL) {
			zb_shell_print_error(
				entry->shell,
				"Failed to parse response command",
				ZB_FALSE);
		} else {
			shell_fprintf(entry->shell,
				      (add_group_resp->status == ZB_ZCL_STATUS_SUCCESS) ?
				       SHELL_INFO : SHELL_ERROR,
				      "Response command received, group: %#.4hx, status: %d\n",
				      add_group_resp->group_id,
				      add_group_resp->status);

			if (add_group_resp->status != ZB_ZCL_STATUS_SUCCESS) {
				zb_shell_print_error(
					entry->shell,
					"Command not successful",
					ZB_FALSE);
			} else {
				zb_shell_print_done(entry->shell, ZB_FALSE);
			}
		}
	} else if (cmd_info->cmd_id == ZB_ZCL_CMD_GROUPS_REMOVE_GROUP_RES) {
		zb_zcl_groups_remove_group_res_t *remove_group_resp;

		ZB_ZCL_GROUPS_GET_REMOVE_GROUP_RES(bufid, remove_group_resp);

		if (remove_group_resp == NULL) {
			zb_shell_print_error(
				entry->shell,
				"Failed to parse response command",
				ZB_FALSE);
		} else {
			shell_fprintf(entry->shell,
				      (remove_group_resp->status == ZB_ZCL_STATUS_SUCCESS) ?
				       SHELL_INFO : SHELL_ERROR,
				      "Response command received, group: %#.4hx, status: %d\n",
				      remove_group_resp->group_id,
				      remove_group_resp->status);

			if (remove_group_resp->status != ZB_ZCL_STATUS_SUCCESS) {
				zb_shell_print_error(
					entry->shell,
					"Command not successful",
					ZB_FALSE);
			} else {
				zb_shell_print_done(entry->shell, ZB_FALSE);
			}
		}
	} else if (cmd_info->cmd_id == ZB_ZCL_CMD_GROUPS_GET_GROUP_MEMBERSHIP_RES) {
		zb_zcl_groups_get_group_membership_res_t *get_group_mem_resp;
		/* Handle up to 20 group IDs from a single device. */
		char text_buffer[141] = {0};

		ZB_ZCL_GROUPS_GET_GROUP_MEMBERSHIP_RES(bufid, get_group_mem_resp);

		if (get_group_mem_resp == NULL) {
			zb_shell_print_error(
				entry->shell,
				"Failed to parse response command",
				ZB_FALSE);
		} else {
			for (uint8_t i = 0; i < get_group_mem_resp->group_count; i++) {
				sprintf((text_buffer + 7 * i),
					"%#.4hx,",
					get_group_mem_resp->group_id[i]);
			}
			shell_fprintf(entry->shell,
				      SHELL_NORMAL,
				      "short_addr=%#.4hx ep=%hu capacity=%hu group_cnt=%hu group_list=%s\n",
				      remote_node_short_add,
				      packet_info->dst_ep,
				      get_group_mem_resp->capacity,
				      get_group_mem_resp->group_count,
				      text_buffer);

			zb_shell_print_done(entry->shell, ZB_FALSE);
		}
	} else {
		/* In case of unknown response. */
		return ZB_FALSE;
	}

	/* Cancel the ongoing alarm which was to delete entry in the context manager ... */
	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(zb_shell_zcl_cmd_timeout_cb,
						   ctx_mgr_get_index_by_entry(entry));
	ZB_ERROR_CHECK(zb_err_code);

	/* ...and erase it manually. */
	ctx_mgr_delete_entry(entry);
	zb_buf_free(bufid);

	return ZB_TRUE;
}
