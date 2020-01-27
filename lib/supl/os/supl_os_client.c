/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#include <zephyr.h>

#include <supl_session.h>
#include <supl_os_client.h>
#include <lte_params.h>
#include <utils.h>

#include <at_cmd.h>

#define AT_IMSI          "AT+CIMI"
#define AT_XMONITOR      "AT\%XMONITOR"
#define SUPL_IMSI_LENGTH 8

LOG_MODULE_REGISTER(supl_client, LOG_LEVEL_DBG);

typedef int (*handler_at_resp)(struct supl_session_ctx *session_ctx,
			       int col_number,
			       const char *buf,
			       int buf_len);

static char pri_buf[LIBSUPL_PRI_BUF_SIZE];
static char sec_buf[LIBSUPL_SEC_BUF_SIZE];

static struct supl_session_ctx supl_client_ctx;

static struct supl_buffers bufs = {
					.pri_buf = pri_buf,
					.pri_buf_size = sizeof(pri_buf),
					.sec_buf = sec_buf,
					.sec_buf_size = sizeof(sec_buf),
				};

static int parse_at_resp(struct supl_session_ctx *session_ctx,
			 const char *resp,
			 int resp_len,
			 handler_at_resp handler)
{
	int len = get_line_len(resp, resp_len);

	if (len < 0) {
		/* string not null terminated */
		return -1;
	}

	const char *idx = resp;
	int col_number = 0;

	while (1) {
		/* find column end */
		const char *col_end = strchr(idx, ',');
		int col_len = 0;

		if (col_end == NULL) {
			/* last column */
			col_len = (resp + len) - idx;
		} else {
			col_len = col_end - idx;
		}

		if (handler(session_ctx, col_number, idx, col_len) != 0) {
			return -1;
		}

		if (col_end == NULL) {
			break;
		}

		/* advance to next column (skip current ',') */
		idx = col_end + 1;
		++col_number;
	}
	return 0;
}

static int create_device_id(void)
{
	char response[128] = { 0 };

	if (at_cmd_write(AT_IMSI, response, sizeof(response), NULL) != 0) {
		LOG_ERR("Fetching IMSI failed");
		return -1;
	}

	if (hexstr2hex(response,
		       get_line_len(response, sizeof(response)),
		       supl_client_ctx.device_id,
		       SUPL_IMSI_LENGTH) != SUPL_IMSI_LENGTH) {
		LOG_ERR("Parsing IMSI failed");
		return -1;
	}

	supl_client_ctx.device_id_choice = LIBSUPL_ID_CHOICE_IMSI;

	memset(response, 0, sizeof(response));

	if (at_cmd_write(AT_XMONITOR, response, sizeof(response), NULL) != 0) {
		LOG_ERR("Fetching LTE info failed");
		return -1;
	}

	if (parse_at_resp(&supl_client_ctx,
			  response,
			  sizeof(response),
			  handler_at_xmonitor)) {
		LOG_ERR("Parsing LTE info failed");
		return -1;
	}
	return 0;
}

int supl_session(const nrf_gnss_agps_data_frame_t *const agps_request)
{
	create_device_id();

	supl_client_ctx.agps_types.data_flags = agps_request->data_flags;
	supl_client_ctx.agps_types.sv_mask_alm = agps_request->sv_mask_alm;
	supl_client_ctx.agps_types.sv_mask_ephe = agps_request->sv_mask_ephe;

	return supl_client_session(&supl_client_ctx);
}

int supl_init(const struct supl_api *const api)
{
	if (supl_client_init(api, &bufs) != 0) {
		LOG_ERR("Failed to initialize libsupl");
		return -1;
	}

	return 0;
}
