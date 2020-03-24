/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <nrf_socket.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gps.h"

LOG_MODULE_REGISTER(gps, CONFIG_SLM_LOG_LEVEL);

#define IS_FIX(_flag)       (_flag & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
#define IS_UNHEALTHY(_flag) (_flag & NRF_GNSS_SV_FLAG_UNHEALTHY)

#define INVALID_SOCKET	-1

/**@brief List of supported AT commands. */
enum slm_gps_mode {
	GPS_MODE_STANDALONE,
	GPS_MODE_PSM,
	GPS_MODE_EDRX,
	GPS_MODE_AGPS
};

#define AT_GPS	"AT#XGPS"

static struct gps_client {
	int sock; /* Socket descriptor. */
	u16_t mask; /* NMEA mask */
	bool running; /* GPS running status */
	bool has_fix; /* At least one fix is got */
} client;

static nrf_gnss_data_frame_t gps_data;

#define THREAD_STACK_SIZE	KB(1)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

static struct k_thread gps_thread;
static k_tid_t gps_thread_id;
static K_THREAD_STACK_DEFINE(gps_thread_stack, THREAD_STACK_SIZE);
static u64_t ttft_start;

/* global functions defined in different files */
void rsp_send(const u8_t *str, size_t len);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];

static void gps_satellite_stats(void)
{
	static u8_t last_tracked;
	u8_t tracked = 0;
	u8_t in_fix = 0;
	u8_t unhealthy = 0;

	if (gps_data.data_id != NRF_GNSS_PVT_DATA_ID || client.has_fix) {
		return;
	}

	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i) {
		if ((gps_data.pvt.sv[i].sv > 0) &&
		    (gps_data.pvt.sv[i].sv < 33)) {
			LOG_DBG("GPS tracking: %d", gps_data.pvt.sv[i].sv);
			tracked++;
			if (IS_FIX(gps_data.pvt.sv[i].flags)) {
				in_fix++;
			}
			if (IS_UNHEALTHY(gps_data.pvt.sv[i].flags)) {
				unhealthy++;
			}
		}
	}

	if (last_tracked != tracked) {
		sprintf(rsp_buf, "#XGPSS: trackg %d use %d unhealthy %d\r\n",
			tracked, in_fix, unhealthy);
		rsp_send(rsp_buf, strlen(rsp_buf));
		last_tracked = tracked;
	}
}

static void gps_pvt_notify(void)
{
	sprintf(rsp_buf, "#XGPSP: long %f lat %f\r\n",
		gps_data.pvt.longitude,
		gps_data.pvt.latitude);
	rsp_send(rsp_buf, strlen(rsp_buf));
	sprintf(rsp_buf, "#XGPSP: %04u-%02u-%02u %02u:%02u:%02u\r\n",
		gps_data.pvt.datetime.year,
		gps_data.pvt.datetime.month,
		gps_data.pvt.datetime.day,
		gps_data.pvt.datetime.hour,
		gps_data.pvt.datetime.minute,
		gps_data.pvt.datetime.seconds);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static void gps_thread_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		if (nrf_recv(client.sock, &gps_data, sizeof(gps_data), 0)
			<= 0) {
			LOG_ERR("GPS nrf_recv(): %d", -errno);
			sprintf(rsp_buf, "#XGPSRUN: %d\r\n", -errno);
			rsp_send(rsp_buf, strlen(rsp_buf));
			nrf_close(client.sock);
			client.running = false;
			break;
		}
		gps_satellite_stats();
		switch (gps_data.data_id) {
		case NRF_GNSS_PVT_DATA_ID:
			if (IS_FIX(gps_data.pvt.flags)) {
				gps_pvt_notify();
				if (!client.has_fix) {
					u64_t now = k_uptime_get();

					sprintf(rsp_buf, "#XGPSP: TTFF %ds\r\n",
						(int)(now - ttft_start)/1000);
					rsp_send(rsp_buf, strlen(rsp_buf));
					client.has_fix = true;
				}
			}
			break;
		case NRF_GNSS_NMEA_DATA_ID:
			if (client.has_fix) {
				rsp_send(gps_data.nmea,
					strlen(gps_data.nmea));
			}
			break;
		default:
			break;
		}
	}
}

static int do_gps_start(void)
{
	int ret = -EINVAL;

	nrf_gnss_fix_retry_t    fix_retry    = 0; /* unlimited retry period */
	nrf_gnss_fix_interval_t fix_interval = 1; /* 1s delay between fixes */
	nrf_gnss_delete_mask_t  delete_mask  = 0;
	nrf_gnss_nmea_mask_t    nmea_mask = (nrf_gnss_nmea_mask_t)client.mask;

	client.sock = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM, NRF_PROTO_GNSS);
	if (client.sock < 0) {
		LOG_ERR("Could not init socket (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_FIX_RETRY,
			&fix_retry, sizeof(fix_retry));
	if (ret != 0) {
		LOG_ERR("Failed to set fix retry value (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS,
		NRF_SO_GNSS_FIX_INTERVAL, &fix_interval, sizeof(fix_interval));
	if (ret != 0) {
		LOG_ERR("Failed to set fix interval value (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_NMEA_MASK,
			&nmea_mask, sizeof(nmea_mask));
	if (ret != 0) {
		LOG_ERR("Failed to set nmea mask (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_START,
			&delete_mask, sizeof(delete_mask));
	if (ret != 0) {
		LOG_ERR("Failed to start GPS (err: %d)", -errno);
		goto error;
	}

	/* Start GPS listening thread */
	if (gps_thread_id != NULL) {
		k_thread_resume(gps_thread_id);
	} else {
		gps_thread_id = k_thread_create(&gps_thread, gps_thread_stack,
				K_THREAD_STACK_SIZEOF(gps_thread_stack),
				gps_thread_fn, NULL, NULL, NULL,
				THREAD_PRIORITY, 0, K_NO_WAIT);
	}

	client.running = true;
	LOG_DBG("GPS started");

	sprintf(rsp_buf, "#XGPS: 1,%d\r\n", client.mask);
	rsp_send(rsp_buf, strlen(rsp_buf));
	ttft_start = k_uptime_get();
	return 0;

error:
	LOG_ERR("GPS start failed: %d", ret);
	sprintf(rsp_buf, "#XGPS: %d\r\n", ret);
	rsp_send(rsp_buf, strlen(rsp_buf));
	client.running = false;

	return -errno;
}

static int do_gps_stop(void)
{
	int ret = 0;
	nrf_gnss_delete_mask_t	delete_mask  = 0;

	if (client.sock != INVALID_SOCKET) {
		ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS,
			NRF_SO_GNSS_STOP, &delete_mask, sizeof(delete_mask));
		if (ret != 0) {
			LOG_ERR("Failed to stop GPS (err: %d)", -errno);
			ret = -errno;
		} else {
			k_thread_suspend(gps_thread_id);
			nrf_close(client.sock);
			client.running = false;
			sprintf(rsp_buf, "#XGPS: 0\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
			LOG_DBG("GPS stopped");
		}

	}

	return ret;
}

/**@brief handle AT#XGPSRUN commands
 *  AT#XGPSRUN=<op>[,<mask>]
 *  AT#XGPSRUN?
 *  AT#XGPSRUN=? TEST command not supported
 */
static int handle_at_gpsrun(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == 1) {
			if (at_params_valid_count_get(&at_param_list) > 2) {
				err = at_params_short_get(&at_param_list, 2,
							&client.mask);
				if (err < 0) {
					return err;
				}
			}
			if (client.running) {
				LOG_WRN("GPS is running");
			} else {
				err = do_gps_start();
			}
		} else if (op == 0) {
			if (!client.running) {
				LOG_WRN("GPS is not running");
			} else {
				err = do_gps_stop();
			}
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.running) {
			sprintf(rsp_buf, "#XGPS: 1,%d\r\n", client.mask);
		} else {
			sprintf(rsp_buf, "#XGPS: 0\r\n");
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle GPS AT commands
 */
int slm_at_gps_parse(const char *at_cmd)
{
	int ret = -ENOTSUP;

	if (slm_util_cmd_casecmp(at_cmd, AT_GPS)) {
		ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
		if (ret < 0) {
			LOG_ERR("Failed to parse AT command %d", ret);
			return -EINVAL;
		}
		ret = handle_at_gpsrun(at_parser_cmd_type_get(at_cmd));
	}

	return ret;
}

/**@brief API to list GPS AT commands
 */
void slm_at_gps_clac(void)
{
	sprintf(rsp_buf, "%s\r\n", AT_GPS);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

/**@brief API to initialize GPS AT commands handler
 */
int slm_at_gps_init(void)
{
	client.sock = INVALID_SOCKET;
	client.mask =  NRF_GNSS_NMEA_GSV_MASK |
		       NRF_GNSS_NMEA_GSA_MASK |
		       NRF_GNSS_NMEA_GLL_MASK |
		       NRF_GNSS_NMEA_GGA_MASK |
		       NRF_GNSS_NMEA_RMC_MASK;
	client.running = false;
	client.has_fix = false;
	gps_thread_id = NULL;

	return 0;
}

/**@brief API to uninitialize GPS AT commands handler
 */
int slm_at_gps_uninit(void)
{
	if (gps_thread_id != NULL) {
		do_gps_stop();
		k_thread_abort(gps_thread_id);
		gps_thread_id = NULL;
	}

	return 0;
}
