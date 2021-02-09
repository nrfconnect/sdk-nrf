/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <nrf_socket.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gps.h"
#ifdef CONFIG_SUPL_CLIENT_LIB
#include <net/socket.h>
#include <supl_os_client.h>
#include <supl_session.h>
#endif

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
	uint16_t mask; /* NMEA mask */
	bool running; /* GPS running status */
	bool has_fix; /* At least one fix is got */
} client;

static nrf_gnss_data_frame_t gps_data;

#define THREAD_STACK_SIZE	KB(1)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

static struct k_thread gps_thread;
static k_tid_t gps_thread_id;
static K_THREAD_STACK_DEFINE(gps_thread_stack, THREAD_STACK_SIZE);
static uint64_t ttft_start;

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern struct k_work_q slm_work_q;

#ifdef CONFIG_SUPL_CLIENT_LIB
static int supl_fd;
int connect_supl_server(void)
{
	int ret;
	struct sockaddr_in server;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
		/* Either a valid,
		 * NULL-terminated access point name or NULL.
		 */
		.ai_canonname = NULL,
	};

	ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret < 0) {
		LOG_ERR("socket() error: %d", -errno);
		return -errno;
	}

	supl_fd = ret;
	ret = getaddrinfo(CONFIG_SLM_SUPL_SERVER, NULL, &hints, &res);
	if (ret || res == NULL) {
		LOG_ERR("getaddrinfo() error: %d", ret);
		close(supl_fd);
		return ret;
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(CONFIG_SLM_SUPL_PORT);
	server.sin_addr.s_addr =
		((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
	ret = connect(supl_fd, (struct sockaddr *)&server,
		sizeof(struct sockaddr_in));
	if (ret) {
		LOG_ERR("connect() error: %d", -errno);
		close(supl_fd);
		ret = -errno;
	}

	freeaddrinfo(res);
	return ret;
}

ssize_t supl_write(const void *buff, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);
	return send(supl_fd, buff, nbytes, 0);
}

int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256] = { 0 };
	va_list args;

	va_start(args, fmt);
	int ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		LOG_ERR("[SUPL] Encoding error");
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		LOG_WRN("[SUPL] Too long message");
	}

	LOG_DBG("[SUPL] %s", log_strdup(buffer));

	return ret;
}

ssize_t supl_read(void *buff, size_t nbytes, void *user_data)
{
	ssize_t rc = recv(supl_fd, buff, nbytes, 0);

	ARG_UNUSED(user_data);

	if (rc < 0 && (errno == ETIMEDOUT)) {
		return 0;
	}

	return rc;
}

int supl_inject(void *agps, size_t agps_size, nrf_gnss_agps_data_type_t type,
	     void *user_data)
{
	int ret;

	ARG_UNUSED(user_data);

	ret = nrf_sendto(client.sock, agps, agps_size, 0, &type, sizeof(type));
	if (ret) {
		LOG_ERR("Inject error, type: %d (err: %d)", type, errno);
		return ret;
	}

	LOG_INF("Injected AGPS data, flags: %d, size: %d", type, agps_size);
	return 0;
}

void supl_handler(struct k_work *work)
{
	int ret;
	nrf_gnss_delete_mask_t delete_mask = 0;

	ARG_UNUSED(work);

	/* suspend GPS */
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_STOP,
			&delete_mask, sizeof(delete_mask));
	if (ret) {
		LOG_ERR("Failed to suspend GPS (err: %d)", -errno);
		return;
	}
	k_thread_suspend(gps_thread_id);
	sprintf(rsp_buf, "#XGPSS: \"GPS suspended\"\r\n");
	rsp_send(rsp_buf, strlen(rsp_buf));

	/* SUPL injection */
	ret = connect_supl_server();
	if (ret == 0) {
		LOG_DBG("SUPL session start");
		supl_session(&gps_data.agps);
		LOG_DBG("SUPL session done");
		close(supl_fd);
	}
	sprintf(rsp_buf, "#XGPSS: \"SUPL injection done\"\r\n");
	rsp_send(rsp_buf, strlen(rsp_buf));

	/* Resume GPS */
	k_thread_resume(gps_thread_id);
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_START,
			&delete_mask, sizeof(delete_mask));
	if (ret) {
		LOG_ERR("Failed to resume GPS (err: %d)", -errno);
	} else {
		ttft_start = k_uptime_get();
		sprintf(rsp_buf, "#XGPSS: \"GPS resumed\"\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

K_WORK_DEFINE(supl_work, supl_handler);

#endif /* CONFIG_SUPL_CLIENT_LIB */

static void gps_satellite_stats(void)
{
	static uint8_t last_tracked;
	uint8_t tracked = 0;
	uint8_t in_fix = 0;
	uint8_t unhealthy = 0;

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
		sprintf(rsp_buf,
			"#XGPSS: \"track %d use %d unhealthy %d\"\r\n",
			tracked, in_fix, unhealthy);
		rsp_send(rsp_buf, strlen(rsp_buf));
		last_tracked = tracked;
	}
}

static void gps_pvt_notify(void)
{
	sprintf(rsp_buf, "#XGPSP: \"long %f lat %f\"\r\n",
		gps_data.pvt.longitude,
		gps_data.pvt.latitude);
	rsp_send(rsp_buf, strlen(rsp_buf));
	sprintf(rsp_buf, "#XGPSP: \"%04u-%02u-%02u %02u:%02u:%02u\"\r\n",
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
			sprintf(rsp_buf, "#XGPS: %d\r\n", -errno);
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
					uint64_t now = k_uptime_get();

					sprintf(rsp_buf,
						"#XGPSP: \"TTFF %ds\"\r\n",
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

		case NRF_GNSS_AGPS_DATA_ID:
#ifdef CONFIG_SUPL_CLIENT_LIB
			LOG_INF("New AGPS data requested, flags 0x%08x",
			       gps_data.agps.data_flags);
			k_work_submit_to_queue(&slm_work_q, &supl_work);
#endif
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
	if (ret) {
		LOG_ERR("Failed to set fix retry value (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS,
		NRF_SO_GNSS_FIX_INTERVAL, &fix_interval, sizeof(fix_interval));
	if (ret) {
		LOG_ERR("Failed to set fix interval value (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_NMEA_MASK,
			&nmea_mask, sizeof(nmea_mask));
	if (ret) {
		LOG_ERR("Failed to set nmea mask (err: %d)", -errno);
		goto error;
	}
	ret = nrf_setsockopt(client.sock, NRF_SOL_GNSS, NRF_SO_GNSS_START,
			&delete_mask, sizeof(delete_mask));
	if (ret) {
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
	nrf_close(client.sock);
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

/**@brief handle AT#XGPS commands
 *  AT#XGPS=<op>[,<mask>]
 *  AT#XGPS?
 *  AT#XGPS=? TEST command not supported
 */
static int handle_at_gps(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
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

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XGPS: (0,1),(bitmask)\r\n");
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
	int ret = -ENOENT;

	if (slm_util_cmd_casecmp(at_cmd, AT_GPS)) {
		ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
		if (ret < 0) {
			LOG_ERR("Failed to parse AT command %d", ret);
			return -EINVAL;
		}
		ret = handle_at_gps(at_parser_cmd_type_get(at_cmd));
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
#ifdef CONFIG_SUPL_CLIENT_LIB
	int ret;
	static struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = supl_inject,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	ret = supl_init(&supl_api);
	if (ret) {
		LOG_ERR("SUPL init error: %d", ret);
		return ret;
	}
#endif

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
