/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <modem/at_cmd.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <power/reboot.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>
#include <net/nrf_cloud_agps.h>
#include <nrf_modem_gnss.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <pm_config.h>
#endif
#if defined(CONFIG_DATE_TIME)
#include <date_time.h>
#endif
#include <debug/stack.h>
#include <kernel.h>

LOG_MODULE_REGISTER(rest_agps_sample, CONFIG_NRF_CLOUD_REST_AGPS_SAMPLE_LOG_LEVEL);

#define REST_RX_BUF_SZ 2100
#define JWT_BUF_SZ 900
#define JWT_DURATION_S (60*5)
#define BUTTON_EVENT_BTN_NUM CONFIG_REST_BUTTON_EVT_NUM
#define LED_NUM CONFIG_REST_LED_NUM
#define JITP_REQ_WAIT_SEC 10
#define FAILURE_LIMIT 10
#define AGPS_WORKQ_THREAD_STACK_SIZE 3072
#define AGPS_WORKQ_THREAD_PRIORITY   5

static K_THREAD_STACK_DEFINE(agps_workq_stack_area, AGPS_WORKQ_THREAD_STACK_SIZE);
#if defined(CONFIG_REST_NMEA)
K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
#endif
static K_SEM_DEFINE(button_press_sem, 0, 1);
static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);

#if defined(CONFIG_DATE_TIME)
static K_SEM_DEFINE(time_obtained_sem, 0, 1);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
static K_SEM_DEFINE(do_pgps_request_sem, 0, 1);
static struct k_work request_pgps_work;
static struct k_work manage_pgps_work;
static struct k_work notify_pgps_work;
static struct nrf_cloud_pgps_prediction *prediction;
static struct gps_pgps_request pgps_req;
static struct nrf_cloud_rest_pgps_request pgps_request;
#endif

static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
static struct lte_lc_cell current_cell;
static char rx_buf[REST_RX_BUF_SZ];
static char jwt[JWT_BUF_SZ];

static bool jitp_requested;
static bool disconnected = true;

static struct nrf_modem_gnss_agps_data_frame agps_request;
static struct nrf_cloud_rest_agps_result agps_result = {
	.buf_sz = 0,
	.buf = NULL
};
#if defined(CONFIG_NRF_CLOUD_AGPS)
static struct lte_lc_cells_info net_info = {
	.ncells_count = 0,
	.neighbor_cells = NULL
};
static struct nrf_cloud_rest_agps_request agps_req = {
	.type = NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
	.agps_req = &agps_request,
	.net_info = &net_info
};
#endif
static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static volatile bool gnss_blocked;
static volatile int failure_count;
static struct k_work_q agps_work_q;
static struct k_work get_agps_data_work;

struct k_poll_event events[] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&pvt_data_sem, 0),
#if defined(CONFIG_REST_NMEA)
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&nmea_queue, 0),
#endif
};
#define NUM_EVENTS ARRAY_SIZE(events)

void agps_print_enable(bool enable);
static int wait_connected(void);
#if defined(CONFIG_NRF_CLOUD_AGPS)
static int get_mobile_network_info(int *mcc_num, int *mnc_num);
#endif

static void cls_home(void)
{
#if defined(CONFIG_REST_TERM_CONTROL)
	LOG_INF("\033[1;1H\033[2J");
#endif
}

static void stack_dump(const struct k_thread *thread, void *user_data)
{
	ARG_UNUSED(user_data);
#if defined(CONFIG_THREAD_STACK_INFO) && defined(CONFIG_THREAD_MONITOR)
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;

	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		LOG_ERR("Unable to determine unused stack size (%d)", ret);
		return;
	}

	tname = k_thread_name_get((struct k_thread *)thread);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

	LOG_INF("%p %-10s (real size %u):\tunused %u\tusage %u / %u (%u %%)",
		      thread,
		      tname ? tname : "NA",
		      size, unused, size - unused, size, pcnt);
#else
	ARG_UNUSED(thread);
#endif
}

static void dump_stack_usage(void)
{
	k_thread_foreach(stack_dump, NULL);
}

static void get_agps_data(struct k_work *item)
{
#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* almanac is optional so skip that */
	agps_request.sv_mask_alm = 0;

	/* AGPS data not expected, so move on to PGPS */
#if defined(CONFIG_NRF_CLOUD_AGPS)
	if (!agps_request.data_flags) {
		k_work_submit_to_queue(&agps_work_q, &notify_pgps_work);
		return;
	}
	/* save ephemeris request map for later P-GPS processing */
	uint32_t sv_mask_ephe = agps_request.sv_mask_ephe;

	/* P-GPS will handle ephemerides; no need to request it */
	agps_request.sv_mask_ephe = 0;
#else
	k_work_submit_to_queue(&agps_work_q, &notify_pgps_work);
#endif
#endif
#if defined(CONFIG_NRF_CLOUD_AGPS)
	ARG_UNUSED(item);
	int err;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = true,
		.timeout_ms = 30000,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0
	};

	if (disconnected) {
		err = wait_connected();
		if (err) {
			failure_count++;
			goto cleanup;
		}
	}

	cls_home();
	LOG_INF("New A-GPS data requested, contacting nRF Cloud at %s, flags 0x%04X\n",
		CONFIG_NRF_CLOUD_REST_HOST_NAME, agps_request.data_flags);

	err = nrf_cloud_jwt_generate(JWT_DURATION_S, jwt, sizeof(jwt));
	if (err) {
		LOG_ERR("Failed to generate JWT, err %d", err);
		failure_count++;
		goto cleanup;
	}
	LOG_DBG("JWT:\n%s", log_strdup(jwt));
	rest_ctx.auth = jwt;

	err = get_mobile_network_info(&current_cell.mcc, &current_cell.mnc);
	if (err) {
		LOG_ERR("Failed to read network info: %d", err);
		failure_count++;
		goto cleanup;
	}

	/* update with latest cell tower info */
	net_info.current_cell = current_cell;

	err = nrf_cloud_rest_agps_data_get(&rest_ctx, &agps_req, &agps_result);
	if (err) {
		LOG_ERR("Failed to get A-GPS data: %d", err);
		failure_count++;
	} else {
		LOG_INF("Successfully received A-GPS data");
		failure_count = 0;
		err = nrf_cloud_agps_process(agps_result.buf, agps_result.agps_sz);
		if (err) {
			LOG_ERR("Failed to process A-GPS data: %d", err);
		}
	}

cleanup:
	if (agps_result.buf) {
		k_free(agps_result.buf);
		agps_result.buf = NULL;
	}
	(void)nrf_cloud_rest_disconnect(&rest_ctx);

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* restore value so that P-GPS can use it next */
	agps_request.sv_mask_ephe = sv_mask_ephe;
	k_work_submit_to_queue(&agps_work_q, &notify_pgps_work);
#endif
#endif
	dump_stack_usage();
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void request_pgps(struct k_work *work)
{
	int err;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = true,
		.timeout_ms = 30000,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0
	};

	err = nrf_cloud_jwt_generate(JWT_DURATION_S, jwt, sizeof(jwt));
	if (err) {
		LOG_ERR("Failed to generate JWT, err %d", err);
		failure_count++;
		return;
	}
	LOG_DBG("JWT:\n%s", log_strdup(jwt));
	rest_ctx.auth = jwt;

	LOG_INF("Requesting P-GPS data");
	pgps_request.pgps_req = &pgps_req;
	err = nrf_cloud_rest_pgps_data_get(&rest_ctx, &pgps_request);
	if (err) {
		LOG_ERR("P-GPS request failed, error: %d", err);
		nrf_cloud_pgps_request_reset();
	} else {
		LOG_INF("Processing P-GPS data");
		err = nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
		if (err) {
			LOG_ERR("P-GPS process failed, error: %d", err);
		}
	}
	(void)nrf_cloud_rest_disconnect(&rest_ctx);
	dump_stack_usage();
}

static void manage_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_INF("Sending prediction to modem...");
	err = nrf_cloud_pgps_inject(prediction, &agps_request);
	if (err) {
		LOG_ERR("Unable to send prediction to modem: %d", err);
	}

	err = nrf_cloud_pgps_preemptive_updates();
	if (err) {
		LOG_ERR("Error requesting updates: %d", err);
	}
	dump_stack_usage();
}

static void notify_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_INF("Notifying P-GPS library that predictions are needed");
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("error requesting notification of prediction availability: %d", err);
	}
	dump_stack_usage();
}

void pgps_handler(struct nrf_cloud_pgps_event *event)
{
	/* GPS unit asked for it, but we didn't have it; check now */
	LOG_INF("P-GPS event type: %d", event->type);

	if (event->type == PGPS_EVT_REQUEST) {
		LOG_INF("PGPS_EVT_REQUEST");
		memcpy(&pgps_req, event->request, sizeof(pgps_req));
		k_work_submit_to_queue(&agps_work_q, &request_pgps_work);
		return;
	} else if (event->type != PGPS_EVT_AVAILABLE) {
		return;
	}

	prediction = event->prediction;
	k_work_submit_to_queue(&agps_work_q, &manage_pgps_work);
}
#endif

static void print_pvt_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	char buf[300];
	size_t len;

	len = snprintf(buf, sizeof(buf), "\r\n\t"
		      "Latitude:       %.06f\r\n\t"
		      "Longitude:      %.06f\r\n\t"
		      "Altitude:       %.01f m\r\n\t"
		      "Accuracy:       %.01f m\r\n\t"
		      "Speed:          %.01f m/s\r\n\t"
		      "Speed accuracy: %.01f m/s\r\n\t"
		      "Heading:        %.01f deg\r\n\t"
		      "Date:           %04u-%02u-%02u\r\n\t"
		      "Time (UTC):     %02u:%02u:%02u\r\n",
		      pvt_data->latitude, pvt_data->longitude, pvt_data->accuracy,
		      pvt_data->altitude, pvt_data->speed, pvt_data->speed_accuracy,
		      pvt_data->heading,
		      pvt_data->datetime.year, pvt_data->datetime.month,
		      pvt_data->datetime.day, pvt_data->datetime.hour,
		      pvt_data->datetime.minute, pvt_data->datetime.seconds);
	if (len < 0) {
		LOG_ERR("Could not construct PVT print");
	} else {
		LOG_INF("%s", log_strdup(buf));
	}
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;
	uint32_t tracked_sats = 0;
	static uint32_t prev_tracked_sats;
	char print_buf[100];
	size_t print_buf_len;

	cls_home();
	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if ((pvt_data->sv[i].sv > 0) &&
		    (pvt_data->sv[i].sv < 33)) {
			tracked++;
			tracked_sats |= BIT(pvt_data->sv[i].sv - 1);
		}
	}

	if ((tracked_sats == 0) || (tracked_sats == prev_tracked_sats)) {
		if (tracked_sats != prev_tracked_sats) {
			prev_tracked_sats = tracked_sats;
			LOG_DBG("Tracking no satellites");
		}

		return;
	}

	prev_tracked_sats = tracked_sats;
	print_buf_len = snprintk(print_buf, sizeof(print_buf), "Tracking:  ");

	for (size_t i = 0; i < 32; i++) {
		if (tracked_sats & BIT(i)) {
			print_buf_len +=
				snprintk(&print_buf[print_buf_len - 1],
					 sizeof(print_buf) - print_buf_len,
					 "%d  ", i + 1);
			if (print_buf_len < 0) {
				LOG_ERR("Failed to print satellite stats");
				break;
			}
		}
	}

	LOG_INF("%s", log_strdup(print_buf));
}

static void gnss_event_handler(int event)
{
	int ret;
#if defined(CONFIG_REST_NMEA)
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;
#endif

	switch (event) {
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		gnss_blocked = true;
		break;
	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		gnss_blocked = false;
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		ret = nrf_modem_gnss_read(&agps_request, sizeof(agps_request),
					  NRF_MODEM_GNSS_DATA_AGPS_REQ);
		if (!ret) {
			k_work_submit_to_queue(&agps_work_q, &get_agps_data_work);
		}
		break;
	case NRF_MODEM_GNSS_EVT_PVT:
		ret = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (!ret) {
			k_sem_give(&pvt_data_sem);
		}
		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
#if defined(CONFIG_REST_NMEA)
		nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
		if (nmea_data == NULL) {
			printk("Failed to allocate memory for NMEA\n");
			break;
		}

		ret = nrf_modem_gnss_read(nmea_data,
					  sizeof(struct nrf_modem_gnss_nmea_data_frame),
					  NRF_MODEM_GNSS_DATA_NMEA);
		if (!ret) {
			ret = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
		}
		if (ret) {
			k_free(nmea_data);
		}
#endif
		break;
	default:
		break;
	}
}

static bool agps_data_download_ongoing(void)
{
	bool ret = false;

#if defined(CONFIG_NRF_CLOUD_AGPS)
	ret |= k_work_is_pending(&get_agps_data_work);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	ret |= k_work_is_pending(&request_pgps_work);
#endif
	return ret;
}

#if defined(CONFIG_DATE_TIME)
static void print_time(void)
{
	int err;
	int64_t unix_time_ms;
	char dst[120];

	err = date_time_now(&unix_time_ms);
	if (!err) {
		time_t unix_time;
		struct tm *time;

		unix_time = unix_time_ms / MSEC_PER_SEC;
		LOG_INF("Unix time: %lld", unix_time);
		time = gmtime(&unix_time);
		if (time) {
			/* 2020-02-19T18:38:50.363Z */
			strftime(dst, sizeof(dst), "%Y-%m-%dT%H:%M:%S.000Z", time);
			LOG_INF("Date/time %s", log_strdup(dst));
		} else {
			LOG_WRN("Time not valid");
		}
	} else {
		LOG_ERR("Date/time not available: %d", err);
	}
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		LOG_INF("DATE_TIME_OBTAINED_MODEM");
		k_sem_give(&time_obtained_sem);
		break;
	case DATE_TIME_OBTAINED_NTP:
		LOG_INF("DATE_TIME_OBTAINED_NTP");
		k_sem_give(&time_obtained_sem);
		break;
	case DATE_TIME_OBTAINED_EXT:
		LOG_INF("DATE_TIME_OBTAINED_EXT");
		k_sem_give(&time_obtained_sem);
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("DATE_TIME_NOT_OBTAINED");
		break;
	default:
		break;
	}
}
#endif

static int set_led(const int state)
{
	if (IS_ENABLED(CONFIG_REST_ENABLE_LED)) {
		int err = dk_set_led(LED_NUM, state);

		if (err) {
			LOG_ERR("Failed to set LED, error: %d", err);
			return err;
		}
	}
	return 0;
}

static int init_led(void)
{
	if (IS_ENABLED(CONFIG_REST_ENABLE_LED)) {
		int err = dk_leds_init();

		if (err) {
			LOG_ERR("LED init failed, error: %d", err);
			return err;
		}
		(void)set_led(0);
	}
	return 0;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states &
	    BIT(BUTTON_EVENT_BTN_NUM - 1)) {
		LOG_DBG("Button %d pressed", BUTTON_EVENT_BTN_NUM);
		k_sem_give(&button_press_sem);
	}
}

static int request_jitp(void)
{
	if (!IS_ENABLED(CONFIG_REST_DO_JITP)) {
		return 0;
	}
	int ret = 0;

	jitp_requested = false;

	(void)k_sem_reset(&button_press_sem);

	LOG_INF("Press button %d to request just-in-time provisioning", BUTTON_EVENT_BTN_NUM);
	LOG_INF("Waiting %d seconds..", JITP_REQ_WAIT_SEC);

	ret = k_sem_take(&button_press_sem, K_SECONDS(JITP_REQ_WAIT_SEC));
	if (ret == -EAGAIN) {
		LOG_INF("JITP will be skipped");
		ret = 0;
	} else if (ret) {
		LOG_ERR("k_sem_take: err %d", ret);
	} else {
		jitp_requested = true;
		LOG_INF("JITP will be performed after network connection is obtained");
		ret = 0;
	}
	return ret;
}

static int do_jitp(void)
{
	if (!IS_ENABLED(CONFIG_REST_DO_JITP)) {
		return 0;
	}
	int ret = 0;

	if (!jitp_requested) {
		return 0;
	}

	ret = wait_connected();
	if (ret) {
		return ret;
	}

	LOG_INF("Performing JITP..");
	ret = nrf_cloud_rest_jitp(CONFIG_NRF_CLOUD_SEC_TAG);

	if (ret == 0) {
		LOG_INF("Waiting 30s for cloud provisioning to complete..");
		k_sleep(K_SECONDS(30));
		k_sem_reset(&button_press_sem);
		LOG_INF("Associate device with nRF Cloud account and press button %d when complete",
			BUTTON_EVENT_BTN_NUM);
		(void)k_sem_take(&button_press_sem, K_FOREVER);
	} else if (ret == 1) {
		LOG_INF("Device already provisioned");
	} else {
		LOG_ERR("Device provisioning failed");
	}
	return ret;
}

#if defined(CONFIG_NRF_CLOUD_AGPS)
static int get_mobile_network_info(int *mcc_num, int *mnc_num)
{
	__ASSERT_NO_MSG((mcc_num != NULL) && (mnc_num != NULL));
	char buf[100];
	char mcc[4];
	char mnc[4];
	int len;

	len = modem_info_string_get(MODEM_INFO_OPERATOR, buf, sizeof(buf));
	if (len < 0) {
		return len; /* it's an error code, not a length */
	}
	if ((len < 5) || (len > 6)) {
		LOG_ERR("unexpected mcc+mnc string length:%d, %s", len, log_strdup(buf));
		return -EINVAL;
	}

	/* mobile country code is required to be 3 decimal digits */
	memcpy(mcc, buf, 3);
	mcc[3] = '\0';
	len -= 3;

	/* mobile network code is either 2 or 3 decimal digits */
	memcpy(mnc, &buf[3], len);
	mnc[len] = '\0';

	*mcc_num = (int)strtol(mcc, NULL, 10);
	*mnc_num = (int)strtol(mnc, NULL, 10);

	LOG_DBG("operator:%s, mcc_num:%d, mnc_num:%d", log_strdup(buf), *mcc_num, *mnc_num);
	return 0;
}
#endif

static int configure_lna(void)
{
	int err;
	const char *xmagpio_command = CONFIG_SAMPLE_AT_MAGPIO;
	const char *xcoex0_command = CONFIG_SAMPLE_AT_COEX0;

	/* Make sure the AT command is not empty */
	if (xmagpio_command[0] != '\0') {
		err = at_cmd_write(xmagpio_command, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to send XMAGPIO command");
			return err;
		}
	}

	if (xcoex0_command[0] != '\0') {
		err = at_cmd_write(xcoex0_command, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to send XCOEX0 command");
			return err;
		}
	}

	return 0;
}

static int init_gnss(void)
{
	int err;
	size_t buf_size = nrf_cloud_agps_get_max_file_size(false);

	agps_result.buf = k_calloc(buf_size, 1);
	if (!agps_result.buf) {
		LOG_ERR("Failed to allocate %u bytes for A-GPS buffer", buf_size);
		failure_count = FAILURE_LIMIT;
		return -ENOMEM;
	}
	agps_result.buf_sz = (uint32_t)buf_size;
	struct k_work_queue_config cfg = {.name = "agps_work_q", .no_yield = false};

	k_work_queue_start(
		&agps_work_q,
		agps_workq_stack_area,
		K_THREAD_STACK_SIZEOF(agps_workq_stack_area),
		AGPS_WORKQ_THREAD_PRIORITY,
		&cfg);

	k_work_init(&get_agps_data_work, get_agps_data);
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&request_pgps_work, request_pgps);
	k_work_init(&manage_pgps_work, manage_pgps);
	k_work_init(&notify_pgps_work, notify_pgps);
#endif

#if defined(CONFIG_DATE_TIME)
	date_time_update_async(date_time_event_handler);
	LOG_INF("Waiting to get valid time..");
	err = k_sem_take(&time_obtained_sem, K_MINUTES(5));
	if (err) {
		LOG_ERR("Failed to get time");
		return err;
	}
	print_time();
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
	struct nrf_cloud_pgps_init_param param = {
		.event_handler = pgps_handler,
		.storage_base = PM_MCUBOOT_SECONDARY_ADDRESS,
		.storage_size = PM_MCUBOOT_SECONDARY_SIZE
	};

	err = nrf_cloud_pgps_init(&param);
	if (err) {
		LOG_ERR("nrf_cloud_pgps_init: %d", err);
		return err;
	}
#endif

	/* Initialize and configure GNSS */
	err = nrf_modem_gnss_init();
	if (err) {
		LOG_ERR("Failed to initialize GNSS interface: %d", err);
		return err;
	}

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_REST_NMEA)) {
		err = nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_RMC_MASK |
						  NRF_MODEM_GNSS_NMEA_GGA_MASK |
						  NRF_MODEM_GNSS_NMEA_GLL_MASK |
						  NRF_MODEM_GNSS_NMEA_GSA_MASK |
						  NRF_MODEM_GNSS_NMEA_GSV_MASK);
		if (err) {
			LOG_ERR("Failed to set GNSS NMEA mask: %d", err);
			return err;
		}
	}

	err = nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_FIX_RETRY);
	if (err) {
		LOG_ERR("Failed to set GNSS fix retry: %d", err);
		return err;
	}

	err = nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_FIX_INTERVAL);
	if (err) {
		LOG_ERR("Failed to set GNSS fix interval: %d", err);
		return err;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS: %d", err);
		return err;
	}

	return 0;
}

static int init(void)
{
	int err;

	err = init_led();
	if (err) {
		LOG_ERR("LED initialization failed");
		return err;
	}

	err = at_cmd_init();
	if (err) {
		LOG_ERR("Failed to initialize AT commands, err %d", err);
		return err;
	}

	err = modem_info_init();
	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d", err);
		return err;
	}

	err = configure_lna();
	if (err) {
		return err;
	}

	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to set device ID, err %d", err);
		return err;
	}

	LOG_INF("Device ID: %s", log_strdup(device_id));

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: err %d", err);
		return err;
	}

	/* If configured, prompt the user to press a button to invoke JITP after
	 * the device is connected to the cloud.
	 */
	err = request_jitp();
	if (err) {
		LOG_WRN("User JITP request failed");
		err = 0;
	}

	return 0;
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			(void)k_sem_reset(&lte_connected);
			disconnected = true;
			(void)set_led(0);
			break;
		}
		disconnected = false;

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		current_cell.id = evt->cell.id;
		current_cell.tac = evt->cell.tac;
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
	case LTE_LC_EVT_PSM_UPDATE:
	case LTE_LC_EVT_EDRX_UPDATE:
	case LTE_LC_EVT_RRC_UPDATE:
	default:
		break;
	}
}

static int connect_to_network(void)
{
	int err;

	LOG_INF("Waiting for network..");

	(void)k_sem_reset(&lte_connected);

#if defined(CONFIG_LTE_POWER_SAVING_MODE)
	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("PSM request failed, error: %d", err);
		return err;
	}
	LOG_INF("PSM mode requested");
#endif

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, err %d", err);
	} else {
		err = wait_connected();
	}

	return err;
}

static int wait_connected(void)
{
	int err;

	do {
		err = k_sem_take(&lte_connected, K_SECONDS(10));
		if (err == -EAGAIN) {
			LOG_WRN("Timeout waiting for LTE: asking to connect...");
			err = lte_lc_connect();
			if (err) {
				LOG_ERR("Error on lte_lc_connect():%d", err);
				failure_count++;
			}
		}
	} while (err);

	LOG_INF("Connected");
	(void)set_led(1);

	return err;
}

void main(void)
{
	int err;
	uint64_t fix_timestamp = 0;

	cls_home();
	LOG_INF("Starting nRF Cloud REST A-GPS API Sample");

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		failure_count = FAILURE_LIMIT;
		goto error;
	}

	err = connect_to_network();
	if (err) {
		failure_count++;
	}

	err = init_gnss();
	if (err) {
		failure_count = FAILURE_LIMIT;
		goto error;
	}
	LOG_INF("GPS started");

	agps_print_enable(false);

	(void)do_jitp();

	for (; failure_count < FAILURE_LIMIT;) {
		(void)k_poll(events, NUM_EVENTS, K_FOREVER);

		if (events[0].state == K_POLL_STATE_SEM_AVAILABLE &&
		    k_sem_take(events[0].sem, K_NO_WAIT) == 0) {
			/* New PVT data available */

			events[0].state = K_POLL_STATE_NOT_READY;
			if (!IS_ENABLED(CONFIG_GPS_SAMPLE_NMEA_ONLY) &&
			    !agps_data_download_ongoing()) {
				print_satellite_stats(&last_pvt);

				if (gnss_blocked) {
					LOG_INF("GNSS operation blocked by LTE");
				}
				LOG_INF("---------------------------------\n");

				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
					fix_timestamp = k_uptime_get();
					print_pvt_data(&last_pvt);
				} else {
					LOG_INF("Seconds since last fix: %lld",
							(k_uptime_get() - fix_timestamp) / 1000);
				}
#if defined(CONFIG_REST_NMEA)
				LOG_INF("NMEA strings:");
#endif
			}
		}
#if defined(CONFIG_REST_NMEA)
		struct nrf_modem_gnss_nmea_data_frame *nmea_data;

		if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE &&
		    k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0) {
			/* New NMEA data available */

			events[1].state = K_POLL_STATE_NOT_READY;
			if (!agps_data_download_ongoing()) {
				if (nmea_data && nmea_data->nmea_str) {
					char *nmea = nmea_data->nmea_str;

					if ((nmea[strlen(nmea) - 2] == '\r') ||
					    (nmea[strlen(nmea) - 2] == '\n')) {
						nmea[strlen(nmea) - 2] = '\0';
					}
					LOG_INF("%s", nmea_data->nmea_str);
				}
			}
			k_free(nmea_data);
		}
#endif
	}

error:
	LOG_INF("Rebooting in 30s..");
	k_sleep(K_SECONDS(30));
	sys_reboot(SYS_REBOOT_COLD);
}
