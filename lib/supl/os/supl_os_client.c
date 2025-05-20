/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <nrf_errno.h>
#include <nrf_modem_at.h>

#include <supl_session.h>
#include <supl_os_client.h>
#include <lte_params.h>
#include <utils.h>

#define AT_CDGCONT       "AT+CGDCONT?"
#define AT_XMONITOR      "AT%%XMONITOR"

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

static int set_device_id(void)
{
	int err;
	char response[128] = { 0 };

	device_id_init(&supl_client_ctx);

	err = nrf_modem_at_cmd(response, sizeof(response), AT_CDGCONT);
	if (err != 0 && err != -NRF_E2BIG) {
		LOG_ERR("Reading IP address failed");
		return -1;
	}

	if (parse_at_resp(&supl_client_ctx,
			  response,
			  sizeof(response),
			  handler_at_cgdcont)) {
		LOG_ERR("Parsing IP address failed");
		return -1;
	}

	return 0;
}

static int set_lte_params(void)
{
	char response[192] = { 0 };

	lte_params_init(&supl_client_ctx.lte_params);

	if (nrf_modem_at_cmd(response, sizeof(response), AT_XMONITOR) != 0) {
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

int supl_session(const struct nrf_modem_gnss_agnss_data_frame *const agnss_request)
{
	set_device_id();
	set_lte_params();

	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(agnss_request->system_count > 0,
		 "GNSS system data need not found");
	__ASSERT(agnss_request->system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

	supl_client_ctx.agps_types.data_flags = agnss_request->data_flags;
	supl_client_ctx.agps_types.sv_mask_alm = agnss_request->system[0].sv_mask_alm;
	supl_client_ctx.agps_types.sv_mask_ephe = agnss_request->system[0].sv_mask_ephe;

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
