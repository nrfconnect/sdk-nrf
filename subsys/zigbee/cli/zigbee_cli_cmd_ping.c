/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <shell/shell.h>

#include <zb_nrf_platform.h>
#include <zigbee/zigbee_error_handler.h>
#include "zigbee_cli.h"
#include "zigbee_cli_ping_types.h"
#include "zigbee_cli_utils.h"

/** @brief ZCL Frame control field of Zigbee PING commands.
 */

#define ZIGBEE_PING_FRAME_CONTROL_FIELD 0x11

LOG_MODULE_REGISTER(zigbee_shell_ping, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

static uint8_t ping_seq_num;
static ping_time_cb_t ping_ind_cb;

static zb_uint32_t get_request_duration(struct ctx_entry *req_data);

/**@brief Invalidate an entry with ping request data after the timeout.
 *        This function is called as the ZBOSS callback.
 *
 * @param index   Index to context entry with ping data to invalidate.
 */
static void invalidate_ping_entry_cb(zb_uint8_t index)
{
	uint32_t delay_ms;
	struct ctx_entry *ping_entry = ctx_mgr_get_entry_by_index(index);

	if (ping_entry) {
		delay_ms = get_request_duration(ping_entry);
		/* Inform user about timeout event. */
		if (ping_entry->ping_req_data.cb) {
			ping_entry->ping_req_data.cb(PING_EVT_FRAME_TIMEOUT,
						     delay_ms, ping_entry);
		}

		ctx_mgr_delete_entry(ping_entry);
	} else {
		LOG_ERR("Couldn't get ping entry %d- entry not found", index);
	}
}

/**@brief Get the first entry with ping request sent to addr_short.
 *
 * @param addr_short  Short network address to look for.
 *
 * @return  Pointer to the ping request entry, NULL if none.
 */
static struct ctx_entry *find_ping_entry_by_short(zb_uint16_t addr_short)
{
	int i;
	zb_addr_u req_remote_addr;

	for (i = 0; i < CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR; i++) {
		struct ctx_entry *ping_entry = ctx_mgr_get_entry_by_index(i);

		if (!ping_entry) {
			continue;
		}

		if ((ping_entry->type == CTX_MGR_PING_REQ_ENTRY_TYPE) &&
		    ping_entry->taken) {
			req_remote_addr =
				ping_entry->ping_req_data.packet_info.dst_addr;
		} else {
			continue;
		}

		if (ping_entry->ping_req_data.packet_info.dst_addr_mode ==
		    ZB_APS_ADDR_MODE_16_ENDP_PRESENT) {
			if (req_remote_addr.addr_short == addr_short) {
				return ping_entry;
			}
		} else {
			if (zb_address_short_by_ieee(
				req_remote_addr.addr_long) == addr_short) {

				return ping_entry;
			}
		}
	}

	return NULL;
}

/**@brief Function to actually send a ping frame.
 *
 * @param  index  Index of context manager entry with ping data to send.
 */
static void zb_zcl_send_ping_frame(zb_uint8_t index)
{
	zb_ret_t zb_err_code;
	struct zcl_packet_info *packet_info;
	struct ctx_entry *ping_entry = ctx_mgr_get_entry_by_index(index);

	if (!ping_entry) {
		LOG_ERR("Couldn't send ping frame - context entry not found");
		return;
	}

	if (ping_entry->type == CTX_MGR_PING_REQ_ENTRY_TYPE) {
		packet_info = &(ping_entry->ping_req_data.packet_info);

		/* Capture the sending time. */
		ping_entry->ping_req_data.sent_time = k_uptime_ticks();
	} else {
		packet_info = &(ping_entry->ping_reply_data.packet_info);
	}

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

	if (ping_entry->type == CTX_MGR_PING_REQ_ENTRY_TYPE) {
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ping_entry->shell,
					   "Can not send zcl frame", ZB_FALSE);
			zb_buf_free(packet_info->buffer);
			ctx_mgr_delete_entry(ping_entry);
			return;
		}

		zb_err_code = ZB_SCHEDULE_APP_ALARM(
				invalidate_ping_entry_cb, index,
				ZB_MILLISECONDS_TO_BEACON_INTERVAL(
					ping_entry->ping_req_data.timeout_ms));

		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ping_entry->shell,
					   "Can not schedule timeout alarm.",
					   ZB_FALSE);
			ctx_mgr_delete_entry(ping_entry);
			return;
		}

		if (ping_entry->ping_req_data.cb) {
			uint32_t time_diff = get_request_duration(ping_entry);

			ping_entry->ping_req_data.cb(PING_EVT_FRAME_SCHEDULED,
						     time_diff, ping_entry);
		}
	} else {
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ping_entry->shell,
					   "Can not send zcl frame", ZB_FALSE);
			zb_buf_free(packet_info->buffer);
		}
		/* We don't need the entry anymore,
		 * since we're not expecting any reply to a Ping Reply.
		 */
		ctx_mgr_delete_entry(ping_entry);
	}
}

/**@brief Get time difference, in miliseconds between ping request time
 *        and current time.
 *
 * @param[in] req_data  Pointer to the ping request structure,
 *                      from which the time difference should be calculated.
 *
 * @return  Time difference in miliseconds.
 */
static zb_uint32_t get_request_duration(struct ctx_entry *req_data)
{
	uint32_t time_diff_ms;
	int32_t time_diff_ticks;

	/* Calculate the time difference between request being sent
	 * and reply being received.
	 */
	time_diff_ticks = k_uptime_ticks() - req_data->ping_req_data.sent_time;
	time_diff_ms = k_ticks_to_ms_near32(time_diff_ticks);

	return time_diff_ms;
}

static void frame_acked_cb(zb_bufid_t bufid)
{
	if (bufid) {
		zb_buf_free(bufid);
	}
}

/**@brief Default handler for incoming ping request APS acknowledgments.
 *
 * @details  If there is a user callback defined for the acknowledged request,
 *           the callback with PING_EVT_ACK_RECEIVED event will be called.
 *
 * @param[in] bufid  Reference to a ZBOSS buffer containing APC ACK data.
 */
static void dispatch_user_callback(zb_bufid_t bufid)
{
	zb_uint16_t short_addr;
	zb_ret_t zb_err_code = RET_OK;
	struct ctx_entry *req_data = NULL;
	zb_zcl_command_send_status_t *ping_cmd_status;

	if (bufid == 0) {
		return;
	}

	ping_cmd_status = ZB_BUF_GET_PARAM(bufid,
					   zb_zcl_command_send_status_t);

	if (ping_cmd_status->dst_addr.addr_type == ZB_ZCL_ADDR_TYPE_SHORT) {
		short_addr = ping_cmd_status->dst_addr.u.short_addr;
	} else if (ping_cmd_status->dst_addr.addr_type ==
		   ZB_ZCL_ADDR_TYPE_IEEE) {
		short_addr = zb_address_short_by_ieee(
				ping_cmd_status->dst_addr.u.ieee_addr);
	} else {
		LOG_ERR("Ping request acked with an unknown dest address type: %d",
				ping_cmd_status->dst_addr.addr_type);
		zb_buf_free(bufid);
		return;
	}

	req_data = find_ping_entry_by_short(short_addr);

	if (req_data != NULL) {
		uint32_t delay_ms = get_request_duration(req_data);

		if (ping_cmd_status->status == RET_OK) {
			/* Inform user about ACK reception. */
			if (req_data->ping_req_data.cb) {
				if (req_data->ping_req_data.request_ack == 0) {
					req_data->ping_req_data.cb(
						PING_EVT_FRAME_SENT, delay_ms,
						req_data);
				} else {
					req_data->ping_req_data.cb(
						PING_EVT_ACK_RECEIVED, delay_ms,
						req_data);
				}
			}

			/* If only ACK was requested, cancel ongoing alarm. */
			if (req_data->ping_req_data.request_echo == 0) {
				zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(
						invalidate_ping_entry_cb,
						ctx_mgr_get_index_by_entry(
							req_data));

				if (zb_err_code == RET_OK) {
					ctx_mgr_delete_entry(req_data);
				}
			}
		} else {
			LOG_ERR("Ping request returned error status: %d",
				ping_cmd_status->status);
		}
	} else {
		LOG_ERR("Couldn't find entry with ping request sent to %#.4x",
			short_addr);
	}

	zb_buf_free(bufid);
}

/**@brief  Default ping event handler. Prints out measured time on the CLI
 *         and exits.
 *
 * @param[in] evt           Type of received ping acknowledgment
 * @param[in] delay_ms      Time, in miliseconds, between ping request
 *                          and the event.
 * @param[in] req_data      Pointer to the context manager entry with ongoing
 *                          ping request data.
 */
static void ping_cli_evt_handler(enum ping_time_evt evt, zb_uint32_t delay_ms,
				 struct ctx_entry *req_data)
{
	switch (evt) {
	case PING_EVT_FRAME_SCHEDULED:
		break;

	case PING_EVT_FRAME_TIMEOUT:
		shell_error(req_data->shell,
			    "Error: Request timed out after %u ms.", delay_ms);
		break;

	case PING_EVT_ECHO_RECEIVED:
		shell_print(req_data->shell, "Ping time: %u ms", delay_ms);
		zb_cli_print_done(req_data->shell, ZB_FALSE);
		break;

	case PING_EVT_ACK_RECEIVED:
		if (req_data->ping_req_data.request_echo == 0) {
			shell_print(req_data->shell, "Ping time: %u ms",
				    delay_ms);
			zb_cli_print_done(req_data->shell, ZB_FALSE);
		}
		break;

	case PING_EVT_FRAME_SENT:
		if ((req_data->ping_req_data.request_echo == 0) &&
		    (req_data->ping_req_data.request_ack == 0)) {
			zb_cli_print_done(req_data->shell, ZB_FALSE);
		}
		break;

	case PING_EVT_ERROR:
		zb_cli_print_error(req_data->shell,
				   "Unable to send ping request", ZB_FALSE);
		break;

	default:
		LOG_ERR("Unknown ping event received: %d", evt);
		break;
	}
}

void zb_ping_set_ping_indication_cb(ping_time_cb_t cb)
{
	ping_ind_cb = cb;
}

/**@brief Actually construct the Ping Request frame and send it.
 *
 * @param ping_entry  Pointer to the context manager entry with ping
 *                    request data.
 */
static void ping_request_send(struct ctx_entry *ping_entry)
{
	zb_ret_t zb_err_code;
	zb_bufid_t bufid;
	zb_uint8_t *cmd_buf_ptr;
	zb_uint8_t cli_ep = zb_cli_get_endpoint();
	struct zcl_packet_info *packet_info =
		&(ping_entry->ping_req_data.packet_info);

	if (ping_entry->ping_req_data.count > PING_MAX_LENGTH) {
		if (ping_entry->ping_req_data.cb) {
			ping_entry->ping_req_data.cb(PING_EVT_ERROR, 0,
						     ping_entry);
		}
		return;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		if (ping_entry->ping_req_data.cb) {
			ping_entry->ping_req_data.cb(PING_EVT_ERROR, 0,
						     ping_entry);
		}
		return;
	}

	/* Ping Frame is constructed by 'overloading' the common ZCL frame.
	 * Basically every frame which comes addressed to
	 * the PING_CUSTOM_CLUSTER is considered a Ping Frame. The FCF is being
	 * set to 0x00, the sequence number field is being used as
	 * a Ping Sequence Number, while the Command field is used to
	 * distinguish request/reply. The farther payload of the ping is filled
	 * with bytes PING_ECHO_REQUEST_BYTE/PING_ECHO_REPLY_BYTE.
	 */
	cmd_buf_ptr = ZB_ZCL_START_PACKET(bufid);
	*(cmd_buf_ptr++) = ZIGBEE_PING_FRAME_CONTROL_FIELD;
	/* Sequence Number Field. */
	*(cmd_buf_ptr++) = ping_seq_num;

	/* Fill Command Field. */
	if ((ping_entry->ping_req_data.request_echo) &&
	    (ping_entry->ping_req_data.request_ack)) {
		*(cmd_buf_ptr++) = PING_ECHO_REQUEST;
	} else if ((ping_entry->ping_req_data.request_echo) &&
		   (!ping_entry->ping_req_data.request_ack)) {
		*(cmd_buf_ptr++) = PING_ECHO_NO_ACK_REQUEST;
	} else {
		*(cmd_buf_ptr++) = PING_NO_ECHO_REQUEST;
	}

	memset(cmd_buf_ptr, PING_ECHO_REQUEST_BYTE,
	       ping_entry->ping_req_data.count);
	cmd_buf_ptr += ping_entry->ping_req_data.count;
	ping_entry->ping_req_data.ping_seq = ping_seq_num;
	ping_entry->id  = ping_seq_num;
	ping_seq_num++;

	/* Schedule frame to send. */
	packet_info->buffer = bufid;
	packet_info->ptr = cmd_buf_ptr;
	/* DstAddr and Addr mode already set. */
	packet_info->dst_ep = cli_ep;
	packet_info->ep = cli_ep;
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	packet_info->cluster_id = PING_CUSTOM_CLUSTER;
	packet_info->cb = dispatch_user_callback;
	packet_info->disable_aps_ack =
		(ping_entry->ping_req_data.request_ack ? ZB_FALSE : ZB_TRUE);

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zcl_send_ping_frame,
					       ctx_mgr_get_index_by_entry(
							ping_entry));
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(ping_entry->shell,
				   "Can not schedule zcl frame.", ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(packet_info->buffer);
		zb_osif_enable_all_inter();

		ctx_mgr_delete_entry(ping_entry);
		return;
	}
}

/**@brief Actually construct the Ping Reply frame and send it.
 *
 * @param ping_entry  Pointer to the context manager entry with ping reply data.
 */
static void ping_reply_send(struct ctx_entry *ping_entry)
{
	zb_bufid_t bufid;
	zb_uint8_t *cmd_buf_ptr;
	zb_uint8_t cli_ep = zb_cli_get_endpoint();
	zb_ret_t zb_err_code;
	struct zcl_packet_info *packet_info =
		&(ping_entry->ping_reply_data.packet_info);

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		LOG_WRN("Drop ping request due to the lack of output buffers");
		ctx_mgr_delete_entry(ping_entry);
		return;
	}
	LOG_DBG("Send ping reply");

	cmd_buf_ptr = ZB_ZCL_START_PACKET(bufid);
	*(cmd_buf_ptr++) = ZIGBEE_PING_FRAME_CONTROL_FIELD;
	*(cmd_buf_ptr++) = ping_entry->ping_reply_data.ping_seq;
	*(cmd_buf_ptr++) = PING_ECHO_REPLY;
	memset(cmd_buf_ptr, PING_ECHO_REPLY_BYTE,
	       ping_entry->ping_reply_data.count);
	cmd_buf_ptr += ping_entry->ping_reply_data.count;

	/* Schedule frame to send. */
	packet_info->buffer = bufid;
	packet_info->ptr = cmd_buf_ptr;
	/* DstAddr is already set. */
	packet_info->dst_addr_mode =
		ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
	packet_info->dst_ep = cli_ep;
	packet_info->ep = cli_ep;
	packet_info->prof_id = ZB_AF_HA_PROFILE_ID;
	packet_info->cluster_id = PING_CUSTOM_CLUSTER;
	packet_info->cb = frame_acked_cb;
	packet_info->disable_aps_ack =
		(ping_entry->ping_reply_data.send_ack ? ZB_FALSE : ZB_TRUE);

	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zcl_send_ping_frame,
					       ctx_mgr_get_index_by_entry(
							ping_entry));
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(ping_entry->shell,
				   "Can not schedule zcl frame.", ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(packet_info->buffer);
		zb_osif_enable_all_inter();

		ctx_mgr_delete_entry(ping_entry);
	}
}

/**@brief Indicate ping request reception.
 *
 * @param zcl_cmd_bufid  Zigbee buffer id with the received ZCL packet.
 */
static void ping_req_indicate(zb_bufid_t zcl_cmd_bufid)
{
	struct ctx_entry tmp_request;
	zb_zcl_addr_t remote_node_addr;
	zb_zcl_parsed_hdr_t *zcl_hdr = ZB_BUF_GET_PARAM(zcl_cmd_bufid,
							zb_zcl_parsed_hdr_t);

	remote_node_addr = zcl_hdr->addr_data.common_data.source;

	if (ping_ind_cb == NULL) {
		return;
	}

	memset(&tmp_request, 0, sizeof(struct ctx_entry));

	switch (zcl_hdr->cmd_id) {
	case PING_ECHO_REQUEST:
		tmp_request.ping_req_data.request_echo = 1;
		tmp_request.ping_req_data.request_ack = 1;
		break;

	case PING_ECHO_NO_ACK_REQUEST:
		tmp_request.ping_req_data.request_echo = 1;
		break;

	case PING_NO_ECHO_REQUEST:
		break;

	default:
		/* Received frame is not a ping request. */
		return;
	}

	tmp_request.taken = true;
	tmp_request.ping_req_data.ping_seq = zcl_hdr->seq_number;
	tmp_request.ping_req_data.count = zb_buf_len(zcl_cmd_bufid);
	tmp_request.ping_req_data.sent_time = k_uptime_ticks();

	if (remote_node_addr.addr_type != ZB_ZCL_ADDR_TYPE_SHORT) {
		LOG_INF("Ping request received, but indication will not be generated"
				"due to the unsupported address type.");
		/* Not supported. */
		return;
	}
	tmp_request.ping_req_data.packet_info.dst_addr_mode =
		ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
	tmp_request.ping_req_data.packet_info.dst_addr.addr_short =
		remote_node_addr.u.short_addr;

	ping_ind_cb(PING_EVT_REQUEST_RECEIVED,
		    k_ticks_to_us_near32(tmp_request.ping_req_data.sent_time),
		    &tmp_request);
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint.
 *
 * @param bufid    Reference to a ZBOSS buffer
 */
zb_uint8_t cli_agent_ep_handler_ping(zb_bufid_t bufid)
{
	zb_uint32_t time_diff;
	zb_zcl_addr_t remote_node_addr;
	zb_zcl_parsed_hdr_t *zcl_hdr = ZB_BUF_GET_PARAM(bufid,
							zb_zcl_parsed_hdr_t);

	remote_node_addr = zcl_hdr->addr_data.common_data.source;

	if ((zcl_hdr->cluster_id != PING_CUSTOM_CLUSTER) ||
	    (zcl_hdr->profile_id != ZB_AF_HA_PROFILE_ID)) {
		return ZB_FALSE;
	}

	LOG_DBG("New ping frame received, bufid: %d", bufid);
	ping_req_indicate(bufid);

	if (zcl_hdr->cmd_id == PING_ECHO_REPLY) {
		struct zcl_packet_info *packet_info;
		zb_uint16_t remote_short_addr = 0x0000;

		/* We have our ping reply. */
		struct ctx_entry *req_data =
			ctx_mgr_find_ctx_entry(zcl_hdr->seq_number,
					       CTX_MGR_PING_REQ_ENTRY_TYPE);

		if (req_data == NULL) {
			return ZB_FALSE;
		}

		packet_info = &(req_data->ping_req_data.packet_info);

		if (packet_info->dst_addr_mode ==
		    ZB_APS_ADDR_MODE_16_ENDP_PRESENT) {
			remote_short_addr = packet_info->dst_addr.addr_short;
		} else {
			remote_short_addr =
				zb_address_short_by_ieee(
					packet_info->dst_addr.addr_long);
		}

		if (remote_node_addr.addr_type != ZB_ZCL_ADDR_TYPE_SHORT) {
			return ZB_FALSE;
		}
		if (remote_short_addr != remote_node_addr.u.short_addr) {
			return ZB_FALSE;
		}

		/* Catch the timer value. */
		time_diff = get_request_duration(req_data);

		/* Cancel the ongoing alarm which was to erase the row ... */
		zb_ret_t zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(
						invalidate_ping_entry_cb,
						ctx_mgr_get_index_by_entry(
							req_data));
		ZB_ERROR_CHECK(zb_err_code);

		/* Call callback function in order to indicate
		 * echo response reception.
		 */
		if (req_data->ping_req_data.cb) {
			req_data->ping_req_data.cb(PING_EVT_ECHO_RECEIVED,
						   time_diff, req_data);
		}

		/* ... and erase it manually. */
		if (zb_err_code == RET_OK) {
			ctx_mgr_delete_entry(req_data);
		}

	} else if ((zcl_hdr->cmd_id == PING_ECHO_REQUEST) ||
		   (zcl_hdr->cmd_id == PING_ECHO_NO_ACK_REQUEST)) {

		zb_uint8_t len = zb_buf_len(bufid);
		struct zcl_packet_info *packet_info;
		struct ctx_entry *ping_entry =
			ctx_mgr_new_entry(CTX_MGR_PING_REPLY_ENTRY_TYPE);

		if (ping_entry == NULL) {
			LOG_WRN("Cannot obtain new row for incoming"
					"ping request");
			return ZB_FALSE;
		}
		packet_info = &(ping_entry->ping_reply_data.packet_info);

		/* Save the Ping Reply information in the table and schedule
		 * a sending function.
		 */
		ping_entry->ping_reply_data.count = len;
		ping_entry->ping_reply_data.ping_seq = zcl_hdr->seq_number;

		if (zcl_hdr->cmd_id == PING_ECHO_REQUEST) {
			LOG_DBG("PING echo request with APS ACK received");
			ping_entry->ping_reply_data.send_ack = 1;
		} else {
			LOG_DBG("PING echo request without APS ACK received");
			ping_entry->ping_reply_data.send_ack = 0;
		}

		if (remote_node_addr.addr_type == ZB_ZCL_ADDR_TYPE_SHORT) {
			packet_info->dst_addr.addr_short =
				remote_node_addr.u.short_addr;
		} else {
			LOG_WRN("Drop ping request "
					"due to incorrect address type");
			ctx_mgr_delete_entry(ping_entry);
			zb_buf_free(bufid);
			return ZB_TRUE;
		}

		/* Send the Ping Reply, invalidate the row if not possible. */
		ping_reply_send(ping_entry);
	} else if (zcl_hdr->cmd_id == PING_NO_ECHO_REQUEST) {
		LOG_DBG("PING request without ECHO received");
	} else {
		LOG_WRN("Unsupported Ping message received, cmd_id %d",
			zcl_hdr->cmd_id);
	}

	zb_buf_free(bufid);
	return ZB_TRUE;
}

/** @brief ping over ZCL
 *
 * @code
 * zcl ping [--no-echo] [--aps-ack] <h:dst_addr> <d:payload size>
 * @endcode
 *
 * Example:
 * @code
 * zcl ping 0b010eaafd745dfa 32
 * @endcode
 *
 * @pre Ping only after starting @ref zigbee.
 *
 * Issue a ping-style command to another CLI device of the address `dst_addr`
 * by using `payload_size` bytes of payload.<br>
 *
 * Optionally, the device can request an APS acknowledgment (`--aps-ack`) or
 * ask destination not to sent ping reply (`--no-echo`).<br>
 *
 * To implement the ping-like functionality, a new custom cluster has been
 * defined with ID 64. There are four custom commands defined inside it,
 * each with its own ID.
 *
 * See the following flow graphs for details.
 *
 * - <b>Case 1:</b> Ping with echo, without the APS ack (default mode):
 *   @code
 *       App 1          Node 1                 Node 2
 *         |  -- ping ->  |  -- ping request ->  |   (command ID: 0x02 - ping
 *         |              |                      |    request without the APS
 *         |              |                      |    acknowledgment)
 *         |              |    <- MAC ACK --     |
 *         |              | <- ping reply --     |   (command ID: 0x01 - ping
 *         |              |                      |    reply)
 *         |              |    -- MAC ACK ->     |
 *         |  <- Done --  |                      |
 *   @endcode
 *
 *   In this default mode, the `ping` command measures the time needed
 *   for a Zigbee frame to travel between two nodes in the network
 *   (there and back again). The command uses a custom "overloaded" ZCL frame,
 *   which is constructed as a ZCL frame of the new custom ping
 *   ZCL cluster (ID 64).
 *
 * - <b>Case 2:</b> Ping with echo, with the APS acknowledgment:
 *     @code
 *       App 1          Node 1                 Node 2
 *         |  -- ping ->  |  -- ping request ->  |   (command ID: 0x00 - ping
 *         |              |                      |    request with the APS
 *         |              |                      |    acknowledgment)
 *         |              |    <- MAC ACK --     |
 *         |              |    <- APS ACK --     |
 *         |              |    -- MAC ACK ->     |
 *         |              | <- ping reply --     |   (command ID: 0x01 - ping
 *         |              |                      |    reply)
 *         |              |    -- MAC ACK ->     |
 *         |              |    -- APS ACK ->     |
 *         |              |    <- MAC ACK --     |
 *         |  <- Done --  |                      |
 *     @endcode
 *
 * - <b>Case 3:</b> Ping without echo, with the APS acknowledgment:
 *     @code
 *       App 1          Node 1                 Node 2
 *         |  -- ping ->  |  -- ping request ->  |   (command ID: 0x03 - ping
 *         |              |                      |    request without echo)
 *         |              |    <- MAC ACK --     |
 *         |              |    <- APS ACK --     |
 *         |              |    -- MAC ACK ->     |
 *         |  <- Done --  |                      |
 *     @endcode
 *
 * - <b>Case 4:</b> Ping without echo, without the APS acknowledgment:
 *     @code
 *       App 1          Node 1                 Node 2
 *         |  -- ping ->  |  -- ping request ->  |   (command ID: 0x03 - ping
 *         |              |                      |    request without echo)
 *         |  <- Done --  |                      |
 *         |              |    <- MAC ACK --     |
 *     @endcode
 */
int cmd_zb_ping(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t i;
	struct ctx_entry *ping_entry =
		ctx_mgr_new_entry(CTX_MGR_PING_REQ_ENTRY_TYPE);

	if (!ping_entry) {
		zb_cli_print_error(shell, "Couldn't get free entry to store data",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	ping_entry->ping_req_data.cb = ping_cli_evt_handler;
	ping_entry->ping_req_data.request_ack = 0;
	ping_entry->ping_req_data.request_echo = 1;
	ping_entry->ping_req_data.timeout_ms =
		PING_ECHO_REQUEST_TIMEOUT_S * MSEC_PER_SEC;

	for (i = 1; i < (argc - 2); i++) {
		if (strcmp(argv[i], "--aps-ack") == 0) {
			ping_entry->ping_req_data.request_ack = 1;
		} else if (strcmp(argv[i], "--no-echo") == 0) {
			ping_entry->ping_req_data.request_echo = 0;
		}
	}

	ping_entry->ping_req_data.packet_info.dst_addr_mode =
		parse_address(argv[argc - 2],
			      &(ping_entry->ping_req_data.packet_info.dst_addr),
			      ADDR_ANY);

	if (ping_entry->ping_req_data.packet_info.dst_addr_mode ==
	    ADDR_INVALID) {
		zb_cli_print_error(shell, "Wrong address format", ZB_FALSE);
		ctx_mgr_delete_entry(ping_entry);
		return -EINVAL;
	}

	if (!zb_cli_sscan_uint(argv[argc - 1],
			       (uint8_t *)&ping_entry->ping_req_data.count, 2,
			       10)) {

		zb_cli_print_error(shell, "Incorrect ping payload size",
				   ZB_FALSE);
		ctx_mgr_delete_entry(ping_entry);
		return -EINVAL;
	}

	if (ping_entry->ping_req_data.count > PING_MAX_LENGTH) {
		shell_print(shell, "Note: Ping payload size"
							"exceeds maximum possible,"
							"assuming maximum");
		ping_entry->ping_req_data.count = PING_MAX_LENGTH;
	}

	/* Put the shell instance to be used later. */
	ping_entry->shell = shell;

	ping_request_send(ping_entry);
	return 0;
}
