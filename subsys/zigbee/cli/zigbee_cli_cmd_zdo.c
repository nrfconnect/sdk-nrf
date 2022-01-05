/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include "zigbee_cli.h"

#define BIND_ON_HELP \
	("Create bind entry.\n" \
	"Usage: on <h:source_eui64> <d:source_ep> <h:destination_addr> " \
		"<d:destination_ep> <h:source_cluster_id> <h:request_dst_addr>")

#define BIND_OFF_HELP \
	("Remove bind entry.\n" \
	"Usage: off <h:source_eui64> <d:source_ep> <h:destination_addr> " \
		"<d:destination_ep> <h:source_cluster_id> <h:request_dst_addr>")

#define ACTIVE_EP_HELP \
	("Send active endpoint request.\n" \
	"Usage: active_ep <h:16-bit destination_address>")

#define SIMPLE_DESC_HELP \
	("Send simple descriptor request.\n" \
	"Usage: simple_desc_req <h:16-bit destination_address> <d:endpoint>")

#define MATCH_DESC_HELP \
	("Send match descriptor request.\n" \
	"Usage: match_desc <h:16-bit destination_address> " \
		"<h:requested address/type> <h:profile ID> " \
		"<d:number of input clusters> [<h:input cluster IDs> ...] " \
		"<d:number of output clusters> [<h:output cluster IDs> ...] " \
		"[-t | --timeout d:number of seconds to wait for answers]")

#define NWK_ADDR_HELP \
	("Resolve EUI64 address to short network address.\n" \
	"Usage: nwk_addr <h:EUI64>")

#define IEEE_ADDR_HELP \
	("Resolve network short address to EUI64 address.\n" \
	"Usage: ieee_addr <h:short_addr>")

#define EUI64_HELP \
	("Get/set the eui64 address of the node.\n" \
	"Usage: eui64 [<h:eui64>]")

#define MGMT_BIND_HELP \
	("Get binding table (see spec. 2.4.3.3.4)\n" \
	"Usage: <h:short> [d:start_index]")

#define MGMT_LEAVE_HELP \
	("Perform mgmt_leave_req (see spec. 2.4.3.3.5)\n" \
	"Usage: mgmt_leave <h:16-bit dst_addr> [h:device_address eui64] " \
		"[--children] [--rejoin]\n" \
	"--children - Device should also remove its children when leaving.\n" \
	"--rejoin - Device should rejoin network after leave.")

#define MGMT_LQI_HELP \
	("Perform mgmt_lqi request.\n" \
	"Usage: mgmt_lqi <h:short> [d:start index]")

/* Defines how long to wait, in seconds, for Match Descriptor Response. */
#define ZIGBEE_CLI_MATCH_DESC_RESP_TIMEOUT 5
/* Defines how long to wait, in seconds, for Bind Response. */
#define ZIGBEE_CLI_BIND_RESP_TIMEOUT       5
/* Defines how long to wait, in seconds, for Network Addrees Response. */
#define ZIGBEE_CLI_NWK_ADDR_RESP_TIMEOUT   5
/* Defines how long to wait, in seconds, for IEEE (EUI64) Addrees Response. */
#define ZIGBEE_CLI_IEEE_ADDR_RESP_TIMEOUT  5
/* Defines how long to wait, in seconds, for mgmt_leave response. */
#define ZIGBEE_CLI_MGMT_LEAVE_RESP_TIMEOUT 5

LOG_MODULE_DECLARE(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);


/* Forward declarations. */
static void ctx_timeout_cb(zb_uint8_t ctx_timeout_cb);

/**@brief Parse a list of cluster IDs.
 *
 * @param[in]  argv  Pointer to argument table.
 * @param[in]  num   Number of cluster IDs to scan.
 * @param[out] ids   Pointer to an array (uint16_t) to store cluster IDs.
 *                   The array may be unaligned.
 *
 * @retval  1 Parsing succeeded.
 * @retval  0 Parsing failed.
 */
static int sscan_cluster_list(char **argv, uint8_t num, void *ids)
{
	uint16_t len = 0;
	uint16_t id;
	uint16_t *ptr = ids;

	while ((len < num) && parse_hex_u16(argv[len], &id)) {
		UNALIGNED_PUT(id, ptr);
		len += 1;
		ptr += 1;
	}

	return (len == num);
}

/**@brief Function to be executed in Zigbee thread to ensure that
 *        non-thread-safe ZDO API is safely called.
 *
 * @param[in] idx Index of context manager entry in which zdo request
 *                information is stored.
 */
static void zb_zdo_req(uint8_t idx)
{
	struct ctx_entry *ctx_entry = ctx_mgr_get_entry_by_index(idx);
	struct zdo_req_info *req_ctx = &(ctx_entry->zdo_data.zdo_req);
	zb_ret_t zb_err_code;

	/* Call the actual request function. */
	ctx_entry->id = req_ctx->req_fn(req_ctx->buffer_id, req_ctx->req_cb_fn);

	if (ctx_entry->id == ZB_ZDO_INVALID_TSN) {
		zb_cli_print_error(ctx_entry->shell, "Failed to send request",
				   ZB_FALSE);
		zb_buf_free(req_ctx->buffer_id);
		ctx_mgr_delete_entry(ctx_entry);

	} else if (req_ctx->ctx_timeout && req_ctx->timeout_cb_fn) {
		zb_err_code = ZB_SCHEDULE_APP_ALARM(req_ctx->timeout_cb_fn,
						    ctx_entry->id,
						    req_ctx->ctx_timeout *
						     ZB_TIME_ONE_SECOND);
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ctx_entry->shell, "Unable to schedule timeout callback",
					   ZB_FALSE);
			ctx_mgr_delete_entry(ctx_entry);
		}
	}
}

/**@brief Handles timeout error and invalidates match descriptor request
 *        transaction.
 *
 * @param[in] tsn ZBOSS transaction sequence number.
 */
static void cmd_zb_match_desc_timeout(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (!ctx_entry) {
		return;
	}

	zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
	ctx_mgr_delete_entry(ctx_entry);
}

/**@brief A callback called on match descriptor response.
 *
 * @param[in] bufid Reference number to ZBOSS memory buffer.
 */
static void cmd_zb_match_desc_cb(zb_bufid_t bufid)
{
	zb_zdo_match_desc_resp_t *match_desc_resp;
	zb_apsde_data_indication_t *data_ind;
	struct ctx_entry *ctx_entry;

	match_desc_resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(bufid);
	data_ind = ZB_BUF_GET_PARAM(bufid, zb_apsde_data_indication_t);
	ctx_entry = ctx_mgr_find_ctx_entry(match_desc_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);

	if (ctx_entry) {
		if (match_desc_resp->status == ZB_ZDP_STATUS_SUCCESS) {
			zb_uint8_t *matched_ep =
				(zb_uint8_t *)(match_desc_resp + 1);

			shell_print(ctx_entry->shell, "");
			while (match_desc_resp->match_len > 0) {
				/* Match EP list follows right after
				 * response header.
				 */
				shell_print(ctx_entry->shell,
					    "src_addr=%04hx ep=%d",
					    data_ind->src_addr, *matched_ep);

				matched_ep += 1;
				match_desc_resp->match_len -= 1;
			}

			if (!ctx_entry->zdo_data.is_broadcast) {
				zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
				ctx_mgr_delete_entry(ctx_entry);
			}
		} else if (match_desc_resp->status == ZB_ZDP_STATUS_TIMEOUT) {
			zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
			ctx_mgr_delete_entry(ctx_entry);
		}
	}

	zb_buf_free(bufid);
}

static void cmd_zb_active_ep_cb(zb_bufid_t bufid)
{
	struct ctx_entry *ctx_entry;
	zb_zdo_ep_resp_t *active_ep_resp =
		(zb_zdo_ep_resp_t *)zb_buf_begin(bufid);

	ctx_entry = ctx_mgr_find_ctx_entry(active_ep_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_buf_free(bufid);
		return;
	}

	if (active_ep_resp->status == ZB_ZDP_STATUS_SUCCESS) {
		char text_buffer[150] = "";

		sprintf(text_buffer, "src_addr=%04hx ",
			active_ep_resp->nwk_addr);

		PRINT_LIST(text_buffer, "ep=", "%d", zb_uint8_t,
			   ((zb_uint8_t *)active_ep_resp +
			    sizeof(zb_zdo_ep_resp_t)),
			   active_ep_resp->ep_count);

		shell_print(ctx_entry->shell, "%s", text_buffer);

		zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
	} else {
		zb_cli_print_error(ctx_entry->shell, "Active ep request failed",
				   ZB_FALSE);
	}

	ctx_mgr_delete_entry(ctx_entry);
	zb_buf_free(bufid);
}

static void cmd_zb_simple_desc_req_cb(zb_bufid_t bufid)
{
	struct ctx_entry *ctx_entry;
	zb_zdo_simple_desc_resp_t *simple_desc_resp;
	zb_uint8_t in_cluster_cnt;
	zb_uint8_t out_cluster_cnt;
	void *cluster_list;

	simple_desc_resp = (zb_zdo_simple_desc_resp_t *)zb_buf_begin(bufid);
	in_cluster_cnt = simple_desc_resp->simple_desc.app_input_cluster_count;
	out_cluster_cnt =
		simple_desc_resp->simple_desc.app_output_cluster_count;
	cluster_list =
		(zb_uint16_t *)simple_desc_resp->simple_desc.app_cluster_list;

	ctx_entry = ctx_mgr_find_ctx_entry(simple_desc_resp->hdr.tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_buf_free(bufid);
		return;
	}

	if (simple_desc_resp->hdr.status == ZB_ZDP_STATUS_SUCCESS) {
		char text_buffer[150] = "";

		sprintf(text_buffer,
			"src_addr=0x%04hx ep=%d profile_id=0x%04hx"
			" app_dev_id=0x%0hx app_dev_ver=0x%0hx ",
			simple_desc_resp->hdr.nwk_addr,
			simple_desc_resp->simple_desc.endpoint,
			simple_desc_resp->simple_desc.app_profile_id,
			simple_desc_resp->simple_desc.app_device_id,
			simple_desc_resp->simple_desc.app_device_version);

		PRINT_LIST(text_buffer, "in_clusters=", "0x%04hx", zb_uint16_t,
			   (zb_uint16_t *)cluster_list, in_cluster_cnt);

		PRINT_LIST(text_buffer, "out_clusters=", "0x%04hx", zb_uint16_t,
			   (zb_uint16_t *)cluster_list + in_cluster_cnt, out_cluster_cnt);

		shell_print(ctx_entry->shell, "%s", text_buffer);

		zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
	} else {
		zb_cli_print_error(ctx_entry->shell,
				   "Simple descriptor request failed",
				   ZB_FALSE);
	}

	ctx_mgr_delete_entry(ctx_entry);
	zb_buf_free(bufid);
}

/**@brief Handles timeout error and invalidates binding transaction.
 *
 * @param[in] tsn ZBOSS transaction sequence number.
 */
static void cmd_zb_bind_unbind_timeout(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (!ctx_entry) {
		return;
	}

	zb_cli_print_error(ctx_entry->shell, "Bind/unbind request timed out",
			   ZB_FALSE);
	ctx_mgr_delete_entry(ctx_entry);
}

/**@brief A callback called on bind/unbind response.
 *
 * @param[in] bufid Reference number to ZBOSS memory buffer.
 */
void cmd_zb_bind_unbind_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;
	struct ctx_entry *ctx_entry;
	zb_zdo_bind_resp_t *bind_resp =
		(zb_zdo_bind_resp_t *)zb_buf_begin(bufid);

	ctx_entry = ctx_mgr_find_ctx_entry(bind_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_buf_free(bufid);
		return;
	}

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_bind_unbind_timeout,
						   ZB_ALARM_ANY_PARAM);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(ctx_entry->shell,
				   "Unable to cancel timeout timer", ZB_FALSE);
	}

	if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS) {
		zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
	} else {
		shell_error(ctx_entry->shell,
			    "Error: Unable to modify binding. Status: %d",
			    bind_resp->status);
	}

	ctx_mgr_delete_entry(ctx_entry);
	zb_buf_free(bufid);
}

/**@brief Handles timeout error and invalidates network address
 *        request transaction.
 *
 * @param[in] tsn ZBOSS transaction sequence number.
 */
static void cmd_zb_nwk_addr_timeout(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (!ctx_entry) {
		return;
	}

	zb_cli_print_error(ctx_entry->shell,
			   "Network address request timed out", ZB_FALSE);
	ctx_mgr_delete_entry(ctx_entry);
}

/**@brief A callback called on network address response.
 *
 * @param[in] bufid Reference number to ZBOSS memory buffer.
 */
void cmd_zb_nwk_addr_cb(zb_bufid_t bufid)
{
	zb_zdo_nwk_addr_resp_head_t *nwk_addr_resp;
	struct ctx_entry *ctx_entry;
	zb_ret_t zb_err_code;

	nwk_addr_resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(bufid);
	ctx_entry = ctx_mgr_find_ctx_entry(nwk_addr_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_buf_free(bufid);
		return;
	}

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_nwk_addr_timeout,
						   ZB_ALARM_ANY_PARAM);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(ctx_entry->shell,
				   "Unable to cancel timeout timer", ZB_FALSE);
	}

	if (nwk_addr_resp->status == ZB_ZDP_STATUS_SUCCESS) {
		zb_uint16_t nwk_addr;

		ZB_LETOH16(&nwk_addr, &(nwk_addr_resp->nwk_addr));
		shell_print(ctx_entry->shell, "%04hx", nwk_addr);
		zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
	} else {
		shell_error(ctx_entry->shell, "Error: Unable to resolve EUI64 source address. Status: %d",
			    nwk_addr_resp->status);
	}

	ctx_mgr_delete_entry(ctx_entry);
	zb_buf_free(bufid);
}

/**@brief Handles timeout error and invalidates IEEE (EUI64) address
 *        request transaction.
 *
 * @param[in] tsn ZBOSS transaction sequence number.
 */
static void cmd_zb_ieee_addr_timeout(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (ctx_entry) {
		zb_cli_print_error(ctx_entry->shell,
				   "IEEE address request timed out", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);
	}
}

/**@brief A callback called on IEEE (EUI64) address response.
 *
 * @param[in] bufid Reference number to ZBOSS memory buffer.
 */
void cmd_zb_ieee_addr_cb(zb_bufid_t bufid)
{
	zb_zdo_ieee_addr_resp_t *ieee_addr_resp;
	struct ctx_entry *ctx_entry;
	zb_ret_t zb_err_code;

	ieee_addr_resp = (zb_zdo_ieee_addr_resp_t *)zb_buf_begin(bufid);
	ctx_entry = ctx_mgr_find_ctx_entry(ieee_addr_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_buf_free(bufid);
		return;
	}

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(cmd_zb_ieee_addr_timeout,
						   ZB_ALARM_ANY_PARAM);
	if (zb_err_code != RET_OK) {
		zb_cli_print_error(ctx_entry->shell,
				   "Unable to cancel timeout timer", ZB_FALSE);
	}

	if (ieee_addr_resp->status == ZB_ZDP_STATUS_SUCCESS) {
		zb_address_ieee_ref_t addr_ref;
		zb_ieee_addr_t ieee_addr;
		zb_uint16_t nwk_addr;
		zb_ret_t ret;

		ZB_LETOH64(ieee_addr, ieee_addr_resp->ieee_addr_remote_dev);
		ZB_LETOH16(&nwk_addr, &(ieee_addr_resp->nwk_addr_remote_dev));

		/* Update local IEEE address resolution table. */
		ret = zb_address_update(ieee_addr, nwk_addr, ZB_TRUE,
					&addr_ref);
		if (ret == RET_OK) {
			zb_cli_print_eui64(ctx_entry->shell, ieee_addr);
			/* Prepend newline because `zb_cli_print_eui64`
			 * does not print LF.
			 */
			zb_cli_print_done(ctx_entry->shell, ZB_TRUE);
		} else {
			shell_error(ctx_entry->shell, "Error: Failed to updated address table. Status: %d",
				    ret);
		}
	} else {
		shell_error(ctx_entry->shell,
			    "Error: Unable to resolve IEEE address. Status: %d",
			    ieee_addr_resp->status);
	}

	ctx_mgr_delete_entry(ctx_entry);
	zb_buf_free(bufid);
}

/**@brief Send Active Endpoint Request.
 *
 * @code
 * zdo active_ep <h:16-bit destination_address>
 * @endcode
 *
 * Send Active Endpoint Request to the node addressed by the short address.
 *
 * Example:
 * @code
 * > zdo active_ep 0xb4fc
 * > src_addr=B4FC ep=10,11,12
 * Done
 * @endcode
 *
 */
static int cmd_zb_active_ep(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	zb_zdo_active_ep_req_t *active_ep_req;
	struct ctx_entry *ctx_entry;
	zb_bufid_t bufid;
	uint16_t addr;
	zb_ret_t zb_err_code;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	active_ep_req = zb_buf_initial_alloc(bufid, sizeof(*active_ep_req));

	if (!parse_hex_u16(argv[1], &addr)) {
		zb_cli_print_error(shell, "Incorrect network address",
				   ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -EINVAL;
	}
	active_ep_req->nwk_addr = addr;

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_active_ep_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_active_ep_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout = 0;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = NULL;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}

	return 0;
}

/**@brief Send Simple Descriptor Request.
 *
 * @code
 * zdo simple_desc_req <h:16-bit destination_address> <d:endpoint>
 * @endcode
 *
 * Send Simple Descriptor Request to the given node and endpoint.
 *
 * Example:
 * @code
 * > zdo simple_desc_req 0xefba 10
 * > src_addr=0xEFBA ep=260 profile_id=0x0102 app_dev_id=0x0 app_dev_ver=0x5
 *   in_clusters=0x0000,0x0003,0x0004,0x0005,0x0006,0x0008,0x0300
 *   out_clusters=0x0300
 * Done
 * @endcode
 *
 */
static int cmd_zb_simple_desc(const struct shell *shell, size_t argc,
			      char **argv)
{
	ARG_UNUSED(argc);

	zb_zdo_simple_desc_req_t *simple_desc_req;
	struct ctx_entry *ctx_entry;
	zb_ret_t zb_err_code;
	zb_bufid_t bufid;
	zb_uint16_t addr;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	simple_desc_req = zb_buf_initial_alloc(bufid, sizeof(*simple_desc_req));

	if (!parse_hex_u16(argv[1], &addr)) {
		zb_cli_print_error(shell, "Invalid network address", ZB_FALSE);
		goto error;
	}
	simple_desc_req->nwk_addr = addr;

	if (!zb_cli_sscan_uint8(argv[2], &(simple_desc_req->endpoint))) {
		zb_cli_print_error(shell, "Invalid endpoint", ZB_FALSE);
		goto error;
	}

	if ((simple_desc_req->endpoint < 1) ||
	    (simple_desc_req->endpoint > 254)) {
		zb_cli_print_error(shell,
				   "Invalid endpoint value, should be <1-254>",
				   ZB_FALSE);
		goto error;
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_simple_desc_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_simple_desc_req_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout = 0;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = NULL;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}

	return 0;

error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(bufid);
	zb_osif_enable_all_inter();

	return -EINVAL;
}

/**@brief Send match descriptor request.
 *
 * @code
 * zdo match_desc <h:16-bit destination_address>
	<h:requested address/type> <h:profile ID>
	<d:number of input clusters> [<h:input cluster IDs> ...]
	<d:number of output clusters> [<h:output cluster IDs> ...]
	[-t | --timeout <n seconds>]
 *
 * @endcode
 *
 * Send Match Descriptor Request to the `dst_addr` node that is a
 * query about the `req_addr` node of the `prof_id` profile ID,
 * which must have at least one of `n_input_clusters`(whose IDs are listed
 * in `{...}`) or `n_output_clusters` (whose IDs are listed in `{...}`).
 * The IDs can be either decimal values or hexadecimal strings.
 * Set the timeout of request with `-t` of `--timeout` optional parameter.
 *
 * Example:
 * @code
 * zdo match_desc 0xfffd 0xfffd 0x0104 1 6 0
 * @endcode
 *
 * In this example, the command sends a Match Descriptor Request to all
 * non-sleeping nodes regarding all non-sleeping nodes that have
 * 1 input cluster ON/OFF (ID 6) and 0 output clusters.
 *
 */
static int cmd_zb_match_desc(const struct shell *shell, size_t argc,
			     char **argv)
{
	zb_zdo_match_desc_param_t *match_desc_req;
	struct ctx_entry *ctx_entry;
	zb_ret_t zb_err_code;
	int timeout_offset;
	zb_uint16_t temp;
	zb_bufid_t bufid;
	int ret_err = 0;
	uint16_t *cluster_list = NULL;
	zb_bool_t use_timeout = ZB_FALSE;
	uint8_t len = sizeof(match_desc_req->cluster_list);
	zb_uint16_t timeout = ZIGBEE_CLI_MATCH_DESC_RESP_TIMEOUT;

	/* We use cluster_list for calls to ZBOSS API but we're not using
	 * cluster_list value in any way.
	 */
	(void)(cluster_list);

	if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--timeout")) {
		zb_cli_print_error(shell, "Place option 'timeout' at the end of input parameters",
				   ZB_FALSE);
		return -EINVAL;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	match_desc_req = zb_buf_initial_alloc(bufid, sizeof(*match_desc_req));

	if (!parse_hex_u16(argv[1], &temp)) {
		zb_cli_print_error(shell, "Incorrect network address",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}
	match_desc_req->nwk_addr = temp;

	if (!parse_hex_u16(argv[2], &temp)) {
		zb_cli_print_error(shell, "Incorrect address of interest",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}
	match_desc_req->addr_of_interest = temp;

	if (!parse_hex_u16(argv[3], &temp)) {
		zb_cli_print_error(shell, "Incorrect profile id", ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}
	match_desc_req->profile_id = temp;

	/* The following functions don't perform any checks on the cluster list
	 * assuming that the Shell isn't abused. In practice the list length
	 * is limited by @p SHELL_ARGC_MAX which defaults to 12 arguments.
	 */

	if (!zb_cli_sscan_uint8(argv[4], &(match_desc_req->num_in_clusters))) {
		zb_cli_print_error(shell, "Incorrect number of input clusters",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	/* Check that number of output clusters is present in argv. */
	if ((6 + match_desc_req->num_in_clusters) > argc) {
		zb_cli_print_error(shell, "Incorrect number of input clusters",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (match_desc_req->num_in_clusters) {
		/* Allocate additional space for cluster IDs. Space for one
		 * cluster ID is already in the structure,
		 * hence we subtract len.
		 */
		cluster_list = zb_buf_alloc_right(
					bufid,
					match_desc_req->num_in_clusters *
					 sizeof(uint16_t) - len);

		/* We have used the space, set to 0 so that space for output
		 * clusters is calculated correctly.
		 */
		len = 0;

		/* Use match_desc_req->cluster_list as destination rather than
		 * cluster_list which points to the second element.
		 */
		uint8_t result =
			sscan_cluster_list(
				argv + 5,
				match_desc_req->num_in_clusters,
				match_desc_req->cluster_list);

		if (!result) {
			zb_cli_print_error(shell,
					   "Failed to parse input cluster list",
					   ZB_FALSE);
			ret_err = -EINVAL;
			goto error;
		}
	}

	if (!zb_cli_sscan_uint8(argv[5 + match_desc_req->num_in_clusters],
				&(match_desc_req->num_out_clusters))) {

		zb_cli_print_error(shell, "Incorrect number of output clusters",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	/* Check if enough output clusters IDs are provided. */
	if ((6 + match_desc_req->num_in_clusters +
	     match_desc_req->num_out_clusters) > argc) {
		zb_cli_print_error(shell, "Incorrect number of output clusters",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (match_desc_req->num_out_clusters) {
		cluster_list = zb_buf_alloc_right(
					bufid,
					(match_desc_req->num_out_clusters *
					 sizeof(uint16_t) - len));

		uint8_t result =
			sscan_cluster_list(
				(argv + 5 + match_desc_req->num_in_clusters +
				 1),
				match_desc_req->num_out_clusters,
				((uint16_t *)match_desc_req->cluster_list +
				 match_desc_req->num_in_clusters));

		if (!result) {
			zb_cli_print_error(shell, "Failed to parse output cluster list",
					   ZB_FALSE);
			ret_err = -EINVAL;
			goto error;
		}
	}

	/* Now let's check for timeout option. */
	timeout_offset = (6 + match_desc_req->num_in_clusters +
			  match_desc_req->num_out_clusters);

	if (argc == timeout_offset + 2) {
		if (!strcmp(argv[timeout_offset], "-t") ||
		    !strcmp(argv[timeout_offset], "--timeout")) {

			use_timeout = ZB_TRUE;
			if (zb_cli_sscan_uint(argv[timeout_offset + 1],
					      (uint8_t *)&timeout,
					      2, 10) != 1) {

				/* Let's set the timeout to default. */
				timeout = ZIGBEE_CLI_MATCH_DESC_RESP_TIMEOUT;
				shell_warn(shell, "Could not parse the timeout value, setting to default.");
			}
			shell_print(shell, "Timeout set to %d.", timeout);
		}
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.is_broadcast =
		ZB_NWK_IS_ADDRESS_BROADCAST(match_desc_req->nwk_addr);
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_match_desc_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_match_desc_cb;

	if (use_timeout || !ctx_entry->zdo_data.is_broadcast) {
		ctx_entry->zdo_data.zdo_req.ctx_timeout = timeout;
		ctx_entry->zdo_data.zdo_req.timeout_cb_fn =
			cmd_zb_match_desc_timeout;
	} else {
		ctx_entry->zdo_data.zdo_req.ctx_timeout = 0;
		ctx_entry->zdo_data.zdo_req.timeout_cb_fn = NULL;
	}

	shell_print(shell, "Sending %s request.",
		    (ctx_entry->zdo_data.is_broadcast ? "broadcast" : "unicast"));
	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);
		ret_err = -ENOEXEC;
		goto error;
	}

	return 0;

error:
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(bufid);
	zb_osif_enable_all_inter();

	return ret_err;
}

/**@brief Create or remove a binding between two endpoints on two nodes.
 *
 * @code
 * zdo bind {on,off} <h:source_eui64> <d:source_ep> <h:destination_addr>
 *                   <d:destination_ep> <h:source_cluster_id>
 *                   <h:request_dst_addr>`
 * @endcode
 *
 * Create bound connection between a device identified by `source_eui64` and
 * endpoint `source_ep`, and a device identified by `destination_addr` and
 * endpoint `destination_ep`. The connection is created for ZCL commands and
 * attributes assigned to the ZCL cluster `source_cluster_id` on the
 * `request_dst_addr` node (usually short address corresponding to
 * `source_eui64` argument).
 *
 * Example:
 * @code
 * zdo bind on 0B010E0405060708 1 0B010E4050607080 2 8
 * @endcode
 *
 */
static int cmd_zb_bind(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	zb_zdo_bind_req_param_t *bind_req;
	zb_ret_t zb_err_code;
	zb_bufid_t bufid;
	zb_bool_t bind;
	int ret_err = 0;
	struct ctx_entry *ctx_entry = NULL;

	if (strcmp(argv[0], "on") == 0) {
		bind = ZB_TRUE;
	} else {
		bind = ZB_FALSE;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	bind_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_bind_req_param_t);

	if (!parse_long_address(argv[1], bind_req->src_address)) {
		zb_cli_print_error(shell,
				   "Incorrect EUI64 source address format",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (!zb_cli_sscan_uint8(argv[2], &(bind_req->src_endp))) {
		zb_cli_print_error(shell, "Incorrect source endpoint",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	bind_req->dst_addr_mode = parse_address(argv[3],
						&(bind_req->dst_address),
						ADDR_ANY);
	if (bind_req->dst_addr_mode == ADDR_INVALID) {
		zb_cli_print_error(shell,
				   "Incorrect destination address format",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (!zb_cli_sscan_uint8(argv[4], &(bind_req->dst_endp))) {
		zb_cli_print_error(shell, "Incorrect destination endpoint",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (!parse_hex_u16(argv[5], &(bind_req->cluster_id))) {
		zb_cli_print_error(shell, "Incorrect cluster ID", ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (!parse_short_address(argv[6], &(bind_req->req_dst_addr))) {
		zb_cli_print_error(shell, "Incorrect destination network address for the request",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	if (bind) {
		ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_bind_req;
		ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_bind_unbind_cb;
	} else {
		ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_unbind_req;
		ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_bind_unbind_cb;
	}
	ctx_entry->zdo_data.zdo_req.ctx_timeout = ZIGBEE_CLI_BIND_RESP_TIMEOUT;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = cmd_zb_bind_unbind_timeout;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	return ret_err;

error:
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(bufid);
	zb_osif_enable_all_inter();

	return ret_err;
}

/**@brief Resolve eui64 address to a short network address.
 *
 * @code
 * zdo nwk_addr <h:eui64>
 * @endcode
 *
 * Example:
 * @code
 * zdo nwk_addr 0B010E0405060708
 * @endcode
 *
 */
static int cmd_zb_nwk_addr(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	zb_zdo_nwk_addr_req_param_t *nwk_addr_req;
	zb_ret_t zb_err_code;
	zb_bufid_t bufid;
	int ret_err = 0;
	struct ctx_entry *ctx_entry = NULL;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	nwk_addr_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_nwk_addr_req_param_t);

	if (!parse_long_address(argv[1], nwk_addr_req->ieee_addr)) {
		zb_cli_print_error(shell, "Incorrect EUI64 address format",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	/* Construct network address request. */
	nwk_addr_req->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
	nwk_addr_req->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
	nwk_addr_req->start_index = 0;

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_nwk_addr_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_nwk_addr_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout =
		ZIGBEE_CLI_NWK_ADDR_RESP_TIMEOUT;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = cmd_zb_nwk_addr_timeout;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	return ret_err;

error:
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(bufid);
	zb_osif_enable_all_inter();

	return ret_err;
}

/**@brief Resolve EUI64 by sending IEEE address request.
 *
 * @code
 * zdo ieee_addr <h:short_addr>
 * @endcode
 *
 */
static int cmd_zb_ieee_addr(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	zb_ret_t zb_err_code;
	zb_bufid_t bufid;
	zb_uint16_t addr;
	int ret_err = 0;
	struct ctx_entry *ctx_entry = NULL;
	zb_zdo_ieee_addr_req_param_t *ieee_addr_req = NULL;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		return -ENOEXEC;
	}

	/* Create new IEEE address request and fill with default values. */
	ieee_addr_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_ieee_addr_req_param_t);
	ieee_addr_req->start_index = 0;
	ieee_addr_req->request_type = 0;

	if (!parse_hex_u16(argv[1], &addr)) {
		zb_cli_print_error(shell, "Incorrect network address",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}
	ieee_addr_req->nwk_addr = addr;
	ieee_addr_req->dst_addr = ieee_addr_req->nwk_addr;

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (!ctx_entry) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_ieee_addr_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_ieee_addr_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout =
		ZIGBEE_CLI_IEEE_ADDR_RESP_TIMEOUT;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = cmd_zb_ieee_addr_timeout;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	return ret_err;

error:
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	zb_buf_free(bufid);
	zb_osif_enable_all_inter();

	return ret_err;
}

/**@brief Get the short 16-bit address of the Zigbee device.
 *
 * @code
 * > zdo short
 * 0000
 * Done
 * @endcode
 */
static int cmd_zb_short(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	zb_uint16_t short_addr;
	zb_ieee_addr_t addr;
	int i;

	zb_get_long_address(addr);
	short_addr = zb_address_short_by_ieee(addr);
	if (short_addr != ZB_UNKNOWN_SHORT_ADDR) {
		/* We got a valid address. */
		for (i = sizeof(zb_uint16_t) - 1; i >= 0; i--) {
			shell_fprintf(shell, SHELL_NORMAL, "%02x",
				      *((zb_uint8_t *)(&short_addr) + i));
		}

		zb_cli_print_done(shell, ZB_TRUE);
		return 0;
	} else {
		/* Most probably there was no network to join. */
		zb_cli_print_error(shell, "Check if device was commissioned",
				   ZB_FALSE);
		return -ENOEXEC;
	}
}

/**@brief Get or set the EUI64 address of the Zigbee device.
 *
 * @code
 * > zdo eui64 [<h:eui64>]
 * 0b010eaafd745dfa
 * Done
 * @endcode
 */
static int cmd_zb_eui64(const struct shell *shell, size_t argc, char **argv)
{
	zb_ieee_addr_t addr;

	if (argc == 1) {
		zb_get_long_address(addr);
	} else {
		if (zigbee_is_stack_started()) {
			zb_cli_print_error(shell,
					   "Stack already started",
					   ZB_FALSE);
			return -ENOEXEC;
		}

		if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
		    && zb_cli_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
		    ) {
			shell_warn(
				shell,
				"Zigbee stack has been configured in the past.\r\n"
				"Please disable NVRAM to change the EUI64.");

			zb_cli_print_error(shell,
					   "Can't change EUI64 - NVRAM not empty",
					   ZB_FALSE);
			return -ENOEXEC;
		}

		if (parse_long_address(argv[1], addr)) {
			zb_set_long_address(addr);
		} else {
			zb_cli_print_error(shell,
					   "Incorrect EUI64 address format",
					   ZB_FALSE);
			return -EINVAL;
		}
	}

	zb_cli_print_eui64(shell, addr);
	/* Prepend newline because `zb_cli_print_eui64` does not print LF. */
	zb_cli_print_done(shell, ZB_TRUE);

	return 0;
}

/**@brief Callback called when mgmt_leave operation takes too long.
 *
 * @param[in] tsn    ZBOSS transaction sequence number obtained as result
 *                   of zdo_mgmt_leave_req.
 */
static void cmd_zb_mgmt_leave_timeout_cb(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (ctx_entry == NULL) {
		return;
	}

	zb_cli_print_error(ctx_entry->shell, "mgmt_leave request timed out",
			   ZB_FALSE);
	ctx_mgr_delete_entry(ctx_entry);
}

/**@brief Callback called when response to mgmt_leave is received.
 *
 * @param[in] bufid ZBOSS buffer reference.
 */
static void cmd_zb_mgmt_leave_cb(zb_bufid_t bufid)
{
	zb_zdo_mgmt_leave_res_t *mgmt_leave_resp;
	struct ctx_entry *ctx_entry;

	mgmt_leave_resp = (zb_zdo_mgmt_leave_res_t *)zb_buf_begin(bufid);
	ctx_entry = ctx_mgr_find_ctx_entry(mgmt_leave_resp->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (ctx_entry != NULL) {
		zb_ret_t zb_err_code;

		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(
					cmd_zb_mgmt_leave_timeout_cb,
					mgmt_leave_resp->tsn);
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ctx_entry->shell,
					   "Unable to cancel timeout timer",
					   ZB_TRUE);
		}

		if (mgmt_leave_resp->status == ZB_ZDP_STATUS_SUCCESS) {
			zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
		} else {
			shell_error(ctx_entry->shell, "Error: Unable to remove device. Status: %u",
				    (uint32_t)mgmt_leave_resp->status);
		}

		ctx_mgr_delete_entry(ctx_entry);
	}

	zb_buf_free(bufid);
}

/**@brief Parses command line arguments for zdo mgmt_leave command.
 *
 * @param[out]  mgmt_leave_req  Request do be filled in according to command
 *                              line arguments.
 * @param[in]  shell            Pointer to the shell instance, used to print
 *                              errors if necessary.
 * @param[in]  argc             Number of arguments in argv.
 * @param[in]  argv             Arguments from shell to the command.
 *
 * @return true, if arguments were parsed correctly and mgmt_leave_req
 *         has been filled up.
 *         false, if arguments were incorrect.
 *
 * @sa @ref cmd_zb_mgmt_leave
 */
static bool cmd_zb_mgmt_leave_parse(zb_zdo_mgmt_leave_param_t *mgmt_leave_req,
				    const struct shell *shell, size_t argc,
				    char **argv)
{
	zb_uint16_t addr;
	size_t arg_idx;

	ZB_MEMSET(mgmt_leave_req, 0, sizeof(*mgmt_leave_req));

	/* Let it be index of the first argument to parse. */
	arg_idx = 1U;
	if (arg_idx >= argc) {
		zb_cli_print_error(shell, "Lack of dst_addr parameter",
				   ZB_FALSE);
		return false;
	}

	if (parse_hex_u16(argv[arg_idx], &addr) != 1) {
		zb_cli_print_error(shell, "Incorrect dst_addr", ZB_FALSE);
		return false;
	}

	mgmt_leave_req->dst_addr = addr;
	arg_idx++;

	/* Try parse device_address. */
	if (arg_idx < argc) {
		const char *curr_arg = argv[arg_idx];

		if (curr_arg[0] != '-') {
			bool result = parse_long_address(
						curr_arg,
						mgmt_leave_req->device_address);
			if (!result) {
				zb_cli_print_error(shell,
						   "Incorrect device_address",
						   ZB_FALSE);
				return false;
			}

			arg_idx++;
		} else {
			/* No device_address field. */
		}
	}

	/* Parse optional fields. */
	while (arg_idx < argc) {
		const char *curr_arg = argv[arg_idx];

		if (strcmp(curr_arg, "--children") == 0) {
			mgmt_leave_req->remove_children = ZB_TRUE;
		} else if (strcmp(curr_arg, "--rejoin") == 0) {
			mgmt_leave_req->rejoin = ZB_TRUE;
		} else {
			zb_cli_print_error(shell, "Incorrect argument",
					   ZB_FALSE);
			return false;
		}
		arg_idx++;
	}

	return true;
}

/**@brief Send a request to a remote device in order to leave network
 *        through zdo mgmt_leave_req (see spec. 2.4.3.3.5).
 *
 * @code
 * zdo mgmt_leave <h:16-bit dst_addr> [h:device_address eui64]
 *                [--children] [--rejoin]
 * @endcode
 *
 * Send @c mgmt_leave_req to a remote node specified by @c dst_addr.
 * If @c device_address is omitted or it has value @c 0000000000000000,
 * the remote device at address @c dst_addr will remove itself from the network.
 * If @c device_address has other value, it must be a long address
 * corresponding to @c dst_addr or a long address of child node of @c dst_addr.
 * The request is sent with <em>Remove Children</em> and <em>Rejoin</em> flags
 * set to @c 0 by default. Use options:
 * @c \--children or @c \--rejoin do change respective flags to @c 1.
 * For more details, see section 2.4.3.3.5 of the specification.
 *
 * Examples:
 * @code
 * zdo mgmt_leave 0x1234
 * @endcode
 * Sends @c mgmt_leave_req to the device with short address @c 0x1234,
 * asking it to remove itself from the network. @n
 * @code
 * zdo mgmt_leave 0x1234 --rejoin
 * @endcode
 * Sends @c mgmt_leave_req to device with short address @c 0x1234, asking it
 * to remove itself from the network and perform rejoin.@n
 * @code
 * zdo mgmt_leave 0x1234 0b010ef8872c633e
 * @endcode
 * Sends @c mgmt_leave_req to device with short address @c 0x1234, asking it
 * to remove device @c 0b010ef8872c633e from the network.
 * If the target device with short address @c 0x1234 has also a long address
 * @c 0b010ef8872c633e, it will remove itself from the network
 * If the target device with short address @c 0x1234 has a child with long
 * address @c 0b010ef8872c633e, it will remove the child from the network.@n
 * @code
 * zdo mgmt_leave 0x1234 --children
 * @endcode
 * Sends @c mgmt_leave_req to the device with short address @c 0x1234,
 * asking it to remove itself and all its children from the network.@n
 */
static int cmd_zb_mgmt_leave(const struct shell *shell, size_t argc,
			     char **argv)
{
	zb_zdo_mgmt_leave_param_t *mgmt_leave_req;
	zb_ret_t zb_err_code;
	zb_bufid_t bufid = 0;
	struct ctx_entry *ctx_entry = NULL;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (bufid == 0) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		goto error;
	}

	mgmt_leave_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_mgmt_leave_param_t);
	if (!cmd_zb_mgmt_leave_parse(mgmt_leave_req, shell, argc, argv)) {
		/* Make sure ZBOSS buffer API is called safely.
		 * Also, the error message has already been printed
		 * by cmd_zb_mgmt_leave_parse.
		 */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -EINVAL;
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (ctx_entry == NULL) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		goto error;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zdo_mgmt_leave_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_mgmt_leave_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout =
		ZIGBEE_CLI_MGMT_LEAVE_RESP_TIMEOUT;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn =
		cmd_zb_mgmt_leave_timeout_cb;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		goto error;
	}

	return 0;

error:
	if (bufid != 0) {
		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();
	}
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
	return -ENOEXEC;
}

/**@brief Request timeout callback.
 *
 * @param[in] tsn ZDO transaction sequence number returned by request.
 */
static void ctx_timeout_cb(zb_uint8_t tsn)
{
	struct ctx_entry *ctx_entry =
		ctx_mgr_find_ctx_entry(tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (ctx_entry == NULL) {
		LOG_ERR("Unable to find context for ZDO request %u.", tsn);
		return;
	}

	shell_error(ctx_entry->shell, "Error: ZDO request %u timed out.", tsn);
	ctx_mgr_delete_entry(ctx_entry);
}

/**@brief A generic ZDO request callback.
 *
 * This will print status code for the message and, if not overridden, free
 * resources associated with the request.
 *
 * @param[in] bufid ZBOSS buffer id.
 */
static void zdo_request_cb(zb_bufid_t bufid)
{
	struct ctx_entry *ctx_entry;
	zb_zdo_callback_info_t *cb_info;
	bool is_request_complete;
	zb_ret_t zb_err_code;

	cb_info = (zb_zdo_callback_info_t *)zb_buf_begin(bufid);
	ctx_entry = ctx_mgr_find_ctx_entry(cb_info->tsn,
					   CTX_MGR_ZDO_ENTRY_TYPE);
	if (ctx_entry == NULL) {
		LOG_ERR("Unable to find context for TSN %d", cb_info->tsn);
		zb_buf_free(bufid);
		return;
	}

	zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(ctx_timeout_cb,
						   cb_info->tsn);
	ZB_ERROR_CHECK(zb_err_code);

	/* Call custom callback if set. If the callback returns false,
	 * i.e.,request isn't complete, then don't print status,
	 * invalidate context, or free input buffer. Request might not be
	 * complete if more messages must be send, e.g., to get multiple
	 * table entries from a remote device.
	 */
	if (ctx_entry->zdo_data.app_cb_fn != NULL) {
		is_request_complete =
			ctx_entry->zdo_data.app_cb_fn(ctx_entry, bufid);
	} else {
		is_request_complete = true;
	}

	if (is_request_complete) {
		/* We can free all resources. */
		if (cb_info->status == ZB_ZDP_STATUS_SUCCESS) {
			shell_print(ctx_entry->shell, "ZDO request %u complete",
				    cb_info->tsn);
			zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
		} else {
			shell_error(ctx_entry->shell, "Error: ZDO request %u failed with status %u",
				    (uint32_t)cb_info->tsn,
				    (uint32_t)cb_info->status);
		}
	} else {
		/* The request isn't complete, i.e., another ZDO transaction
		 * went out, hence we need to reschedule a timeout callback.
		 */
		zb_err_code = ZB_SCHEDULE_APP_ALARM(
					ctx_timeout_cb, ctx_entry->id,
					(ZIGBEE_CLI_MGMT_LEAVE_RESP_TIMEOUT *
					 ZB_TIME_ONE_SECOND));
		if (zb_err_code != RET_OK) {
			zb_cli_print_error(ctx_entry->shell, "Unable to schedule timeout callback",
					   ZB_FALSE);
			is_request_complete = true;
		}
	}

	if (is_request_complete) {
		ctx_mgr_delete_entry(ctx_entry);
		zb_buf_free(bufid);
	}
}

/**@brief Prints one binding table record.
 *
 * @param[out] shell    The Shell the output is printed to.
 * @param[in]  idx      Record index in binding table.
 * @param[in]  tbl_rec  Record to be printed out.
 */
static void print_bind_resp_record(const struct shell *shell, uint32_t idx,
				   const zb_zdo_binding_table_record_t *tbl_rec)
{
	char ieee_address_str[sizeof(tbl_rec->src_address) * 2U + 1U];

	if (ieee_addr_to_str(ieee_address_str, sizeof(ieee_address_str),
			     tbl_rec->src_address) <= 0) {

		strcpy(ieee_address_str, "(error)         ");
	}
	/* Ensure null-terminated string. */
	ieee_address_str[sizeof(ieee_address_str) - 1U] = '\0';

	/* Note: Fields in format string are scattered to match position
	 * in the header, printed by print_bind_resp_records_header.
	 */
	shell_fprintf(shell, SHELL_NORMAL, "[%3u] %s %8u %#10.4x",
		      (uint32_t)idx, ieee_address_str,
		      (uint32_t)tbl_rec->src_endp,
		      (uint32_t)tbl_rec->cluster_id);

	shell_fprintf(shell, SHELL_NORMAL, "%14.3u ",
		      (uint32_t)tbl_rec->dst_addr_mode);

	switch (tbl_rec->dst_addr_mode) {
	/* 16-bit group address for DstAddr and DstEndp not present. */
	case ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT:
		shell_fprintf(shell, SHELL_NORMAL, "%#16.4x      N/A",
			      (uint32_t)tbl_rec->dst_address.addr_short);
		break;

	/* 64-bit extended address for DstAddr and DstEndp present. */
	case ZB_APS_ADDR_MODE_64_ENDP_PRESENT:
		if (ieee_addr_to_str(ieee_address_str, sizeof(ieee_address_str),
				     tbl_rec->dst_address.addr_long) <= 0) {

			strcpy(ieee_address_str, "(error)         ");
		}
		/* Ensure null-terminated string. */
		ieee_address_str[sizeof(ieee_address_str) - 1U] = '\0';

		shell_fprintf(shell, SHELL_NORMAL, "%s %8.3u", ieee_address_str,
			      (uint32_t)tbl_rec->dst_endp);
		break;

	default:
		/* This should not happen, as the above case values
		 * are the only ones allowed by R21 Zigbee spec.
		 */
		shell_fprintf(shell, SHELL_NORMAL, "            N/A      N/A");
		break;
	}

	shell_print(shell, "");
}

/**@brief Prints a header for binding records table.
 *
 * @param[out] shell      The Shell the output is printed to.
 */
static void print_bind_resp_records_header(const struct shell *shell)
{
	/* Note: Position of fields matches corresponding fields printed
	 * by print_bind_resp_record.
	 */
	shell_print(shell, "[idx] src_address      src_endp cluster_id dst_addr_mode dst_addr         dst_endp");
}

/**@brief Prints records of binding table received from zdo_mgmt_bind_resp.
 *
 * @param[out] shell      The Shell the output is printed to.
 * @param[in]  bind_resp  Response received from remote device to be printed
 *                        out.
 *
 * @note Records of type @ref zb_zdo_binding_table_record_t are located
 *       just after the @ref zb_zdo_mgmt_bind_resp_t structure pointed
 *       by bind_resp parameter.
 */
static void print_bind_resp(const struct shell *shell,
			    const zb_zdo_mgmt_bind_resp_t *bind_resp)
{
	const zb_zdo_binding_table_record_t *tbl_rec;
	uint32_t idx;
	uint32_t next_start_index = ((uint32_t)bind_resp->start_index +
				     bind_resp->binding_table_list_count);

	tbl_rec = (const zb_zdo_binding_table_record_t *)(bind_resp + 1);

	for (idx = bind_resp->start_index; idx < next_start_index; ++idx) {
		print_bind_resp_record(shell, idx, tbl_rec);
		++tbl_rec;
	}
}

/**@brief Callback terminating single mgmt_bind_req transaction.
 *
 * @note When the binding table is too large to fit into a single mgmt_bind_rsp
 *       command frame, this function will issue a new mgmt_bind_req_t
 *       with start_index increased by the number of just received entries
 *       to download remaining part of the binding table. This process may
 *       involve several round trips of mgmt_bind_req followed
 *       by mgmt_bind_rsp until the whole binding table is downloaded.
 *
 * @param bufid     Reference to ZBOSS buffer (as required by Zigbee stack API).
 */
static void cmd_zb_mgmt_bind_cb(zb_bufid_t bufid)
{
	zb_zdo_mgmt_bind_resp_t *resp;
	struct ctx_entry *ctx_entry;

	resp = (zb_zdo_mgmt_bind_resp_t *)zb_buf_begin(bufid);
	ctx_entry = ctx_mgr_find_ctx_entry(resp->tsn, CTX_MGR_ZDO_ENTRY_TYPE);

	if (ctx_entry != NULL) {
		if (resp->status == ZB_ZDP_STATUS_SUCCESS) {
			if ((resp->start_index ==
			     ctx_entry->zdo_data.req_seq.start_index)) {
				print_bind_resp_records_header(
					ctx_entry->shell);
			}
			print_bind_resp(ctx_entry->shell, resp);

			uint32_t next_start_index =
					(resp->start_index +
					 resp->binding_table_list_count);

			if (next_start_index < resp->binding_table_entries &&
			    (next_start_index < 0xFFU) &&
			    (resp->binding_table_list_count != 0U)) {

				/* We have more entries to get. */
				(void)(zb_buf_reuse(bufid));
				zb_zdo_mgmt_bind_param_t *bind_req =
					ZB_BUF_GET_PARAM(
						bufid,
						zb_zdo_mgmt_bind_param_t);

				bind_req->dst_addr =
					ctx_entry->zdo_data.req_seq.dst_addr;
				bind_req->start_index = next_start_index;

				ctx_entry->id = zb_zdo_mgmt_bind_req(
							bufid,
							cmd_zb_mgmt_bind_cb);
				if (ctx_entry->id == ZB_ZDO_INVALID_TSN) {
					zb_cli_print_error(
						ctx_entry->shell,
						"Failed to send request",
						ZB_FALSE);
					goto finish;
				}

				/* bufid reused, mark NULL not to free it. */
				bufid = 0;
				/* Ctx entry reused, set NULL not to free it. */
				ctx_entry = NULL;
			} else {
				shell_print(
					ctx_entry->shell,
					"Total entries for the binding table: %u",
					(uint32_t)resp->binding_table_entries);
				zb_cli_print_done(ctx_entry->shell, ZB_FALSE);
			}
		} else {
			shell_error(ctx_entry->shell, "Error: Unable to get binding table. Status: %u",
				    (uint32_t)resp->status);
		}
	}

finish:
	if (bufid != 0) {
		zb_buf_free(bufid);
	}

	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
}

/**@brief Send a request to a remote device in order to read the binding table
 *        through zdo mgmt_bind_req (see spec. 2.4.3.3.4).
 *
 * @note If whole binding table does not fit into single @c mgmt_bind_resp
 *       frame, the request initiates a series of requests performing full
 *       binding table download.
 *
 * @code
 * zdo mgmt_bind <h:short> [d:start_index]
 * @endcode
 *
 * Example:
 * @code
 * zdo mgmt_bind 0x1234
 * @endcode
 * Sends @c mgmt_bind_req to the device with short address @c 0x1234,
 * asking it to return its binding table.
 */
static int cmd_zb_mgmt_bind(const struct shell *shell, size_t argc, char **argv)
{
	zb_zdo_mgmt_bind_param_t *bind_req;
	zb_ret_t zb_err_code;
	int ret_err = 0;
	size_t arg_idx = 1U;
	zb_bufid_t bufid = 0;
	struct ctx_entry *ctx_entry = NULL;

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (ctx_entry == NULL) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}
	ctx_entry->shell = shell;

	if (arg_idx < argc) {
		if (!parse_short_address(
			argv[arg_idx],
			&(ctx_entry->zdo_data.req_seq.dst_addr))) {

			zb_cli_print_error(shell, "Incorrect dst_addr",
					   ZB_FALSE);
			ret_err = -EINVAL;
			goto error;
		}
		arg_idx++;
	} else {
		zb_cli_print_error(shell, "dst_addr parameter missing",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (arg_idx < argc) {
		if (!zb_cli_sscan_uint8(
			argv[arg_idx],
			&ctx_entry->zdo_data.req_seq.start_index)) {

			zb_cli_print_error(shell, "Incorrect start_index",
					   ZB_FALSE);
			ret_err = -EINVAL;
			goto error;
		}
		arg_idx++;
	} else {
		/* This parameter was optional, no error. */
		ctx_entry->zdo_data.req_seq.start_index = 0;
	}

	if (arg_idx < argc) {
		zb_cli_print_error(shell, "Unexpected extra parameters",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to execute command (buf alloc failed)",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	bind_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_mgmt_bind_param_t);
	ZB_BZERO(bind_req, sizeof(*bind_req));
	bind_req->start_index = ctx_entry->zdo_data.req_seq.start_index;
	bind_req->dst_addr = ctx_entry->zdo_data.req_seq.dst_addr;

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_mgmt_bind_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = cmd_zb_mgmt_bind_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout = 0;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = NULL;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	return ret_err;

error:
	if (bufid != 0) {
		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();
	}
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}
	return ret_err;
}

/**@brief Callback for a single mgmt_lqi_req transaction.
 *
 * @note When the lqi table is too large to fit into a single mgmt_bind_rsp
 *       command frame, this function will issue a new mgmt_lqi_req to download
 *       reminder of the table. This process may involve several round trips
 *       of mgmt_lqi_req followed by mgmt_lqi_resp until the whole binding table
 *       is downloaded.
 *
 * @param bufid     Reference to ZBOSS buffer (as required by Zigbee stack API).
 */
static bool zdo_mgmt_lqi_cb(struct ctx_entry *ctx_entry, zb_bufid_t bufid)
{
	zb_zdo_neighbor_table_record_t *tbl_rec;
	zb_zdo_mgmt_lqi_resp_t *resp;
	bool result = true;

	resp = (zb_zdo_mgmt_lqi_resp_t *)zb_buf_begin(bufid);

	if (resp->status == ZB_ZDP_STATUS_SUCCESS) {
		if (resp->start_index ==
		    ctx_entry->zdo_data.req_seq.start_index) {
			shell_print(ctx_entry->shell,
				    "[idx] ext_pan_id       ext_addr         short_addr flags permit_join depth lqi");
		}

		tbl_rec = (zb_zdo_neighbor_table_record_t *)((uint8_t *)resp +
							     sizeof(*resp));

		for (uint8_t i = 0; i < resp->neighbor_table_list_count; i++) {
			shell_fprintf(ctx_entry->shell, SHELL_NORMAL,
				      "[%3u] ", resp->start_index + i);

			zb_cli_print_eui64(ctx_entry->shell,
					   tbl_rec->ext_pan_id);

			shell_fprintf(ctx_entry->shell, SHELL_NORMAL, " ");
			zb_cli_print_eui64(ctx_entry->shell,
					   tbl_rec->ext_addr);

			shell_print(ctx_entry->shell,
				    "%#7.4x %#8.2x  %u %11u %5u",
				    tbl_rec->network_addr, tbl_rec->type_flags,
				    tbl_rec->permit_join,
				    tbl_rec->depth, tbl_rec->lqi);
			tbl_rec++;
		}

		uint16_t next_index = (resp->start_index +
				       resp->neighbor_table_list_count);

		/* Get next portion of lqi table if needed. */
		if ((next_index < resp->neighbor_table_entries) &&
		    (next_index < 0xff) &&
		    (resp->neighbor_table_list_count > 0)) {
			zb_zdo_mgmt_lqi_param_t *request;

			/* Make sure ZBOSS buffer API is called safely. */
			zb_osif_disable_all_inter();
			(void)(zb_buf_reuse(bufid));
			zb_osif_enable_all_inter();

			request = ZB_BUF_GET_PARAM(bufid,
						   zb_zdo_mgmt_lqi_param_t);

			request->start_index = next_index;
			request->dst_addr =
				ctx_entry->zdo_data.req_seq.dst_addr;

			ctx_entry->id = zb_zdo_mgmt_lqi_req(bufid,
							    zdo_request_cb);
			if (ctx_entry->id != ZB_ZDO_INVALID_TSN) {
				/* The request requires further communication,
				 * hence the outer callback shoudn't free
				 * resources.
				 */
				result = false;
			}
		}
	}

	return result;
}

/**@brief Send a ZDO Mgmt_Lqi_Req command to a remote device.
 *
 * @code
 * zdo mgmt_lqi <h:short> [d:start index]
 * @endcode
 *
 * Example:
 * @code
 * zdo mgmt_lqi 0x1234
 * @endcode
 * Sends @c mgmt_lqi_req to the device with short address @c 0x1234,
 * asking it to return its neighbor table.
 */
static int cmd_zb_mgmt_lqi(const struct shell *shell, size_t argc, char **argv)
{
	zb_zdo_mgmt_lqi_param_t *request;
	zb_ret_t zb_err_code;
	int ret_err = 0;
	zb_bufid_t bufid = 0;
	struct ctx_entry *ctx_entry = NULL;

	/* Make sure ZBOSS buffer API is called safely. */
	zb_osif_disable_all_inter();
	bufid = zb_buf_get_out();
	zb_osif_enable_all_inter();

	if (!bufid) {
		zb_cli_print_error(shell, "Failed to allocate request buffer",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	request = ZB_BUF_GET_PARAM(bufid, zb_zdo_mgmt_lqi_param_t);
	if (!parse_short_address(argv[1], &(request->dst_addr))) {
		zb_cli_print_error(shell, "Failed to parse destination address",
				   ZB_FALSE);
		ret_err = -EINVAL;
		goto error;
	}

	if (argc >= 3) {
		if (!zb_cli_sscan_uint8(argv[2], &(request->start_index))) {
			zb_cli_print_error(shell, "Failed to parse start index",
					   ZB_FALSE);
			ret_err = -EINVAL;
			goto error;
		}
	} else {
		request->start_index = 0;
	}

	ctx_entry = ctx_mgr_new_entry(CTX_MGR_ZDO_ENTRY_TYPE);
	if (ctx_entry == NULL) {
		zb_cli_print_error(shell, "Too many ZDO transactions",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	/* Initialize context and send a request. */
	ctx_entry->shell = shell;
	ctx_entry->zdo_data.zdo_req.buffer_id = bufid;
	ctx_entry->zdo_data.app_cb_fn = zdo_mgmt_lqi_cb;
	ctx_entry->zdo_data.zdo_req.req_fn = zb_zdo_mgmt_lqi_req;
	ctx_entry->zdo_data.zdo_req.req_cb_fn = zdo_request_cb;
	ctx_entry->zdo_data.zdo_req.ctx_timeout =
		ZIGBEE_CLI_MGMT_LEAVE_RESP_TIMEOUT;
	ctx_entry->zdo_data.zdo_req.timeout_cb_fn = ctx_timeout_cb;

	uint8_t ctx_entry_index = ctx_mgr_get_index_by_entry(ctx_entry);

	if (ctx_entry_index == CTX_MGR_ENTRY_IVALID_INDEX) {
		zb_cli_print_error(shell, "Invalid index of entry", ZB_FALSE);
		ctx_mgr_delete_entry(ctx_entry);

		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();

		return -ENOEXEC;
	}
	zb_err_code = ZB_SCHEDULE_APP_CALLBACK(zb_zdo_req, ctx_entry_index);

	if (zb_err_code != RET_OK) {
		zb_cli_print_error(shell, "Unable to schedule zdo request",
				   ZB_FALSE);
		ret_err = -ENOEXEC;
		goto error;
	}

	return ret_err;

error:
	if (bufid != 0) {
		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		zb_buf_free(bufid);
		zb_osif_enable_all_inter();
	}
	if (ctx_entry != NULL) {
		ctx_mgr_delete_entry(ctx_entry);
	}

	return ret_err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bind,
	SHELL_CMD_ARG(on, NULL, BIND_ON_HELP, cmd_zb_bind, 7, 0),
	SHELL_CMD_ARG(off, NULL, BIND_OFF_HELP, cmd_zb_bind, 7, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_zdo,
	SHELL_CMD_ARG(active_ep, NULL, ACTIVE_EP_HELP, cmd_zb_active_ep, 2, 0),
	SHELL_CMD_ARG(simple_desc_req, NULL, SIMPLE_DESC_HELP,
		      cmd_zb_simple_desc, 3, 0),
	SHELL_CMD_ARG(match_desc, NULL, MATCH_DESC_HELP, cmd_zb_match_desc, 6,
		      SHELL_OPT_ARG_CHECK_SKIP),
	SHELL_CMD_ARG(nwk_addr, NULL, NWK_ADDR_HELP, cmd_zb_nwk_addr, 2, 0),
	SHELL_CMD_ARG(ieee_addr, NULL, IEEE_ADDR_HELP, cmd_zb_ieee_addr, 2, 0),
	SHELL_CMD_ARG(eui64, NULL, EUI64_HELP, cmd_zb_eui64, 1, 1),
	SHELL_CMD_ARG(short, NULL, "Get the short address of the node.",
		      cmd_zb_short, 1, 0),
	SHELL_CMD(bind, &sub_bind,
		  "Create/remove the binding entry in the remote node.", NULL),
	SHELL_CMD_ARG(mgmt_bind, NULL, MGMT_BIND_HELP, cmd_zb_mgmt_bind, 2, 1),
	SHELL_CMD_ARG(mgmt_leave, NULL, MGMT_LEAVE_HELP, cmd_zb_mgmt_leave,
		      2, 3),
	SHELL_CMD_ARG(mgmt_lqi, NULL, MGMT_LQI_HELP, cmd_zb_mgmt_lqi, 2, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(zdo, &sub_zdo, "ZDO manipulation.", NULL);
