/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <dk_buttons_and_leds.h>
#include <drivers/gps.h>
#include <sys/reboot.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <date_time.h>
#include <pm_config.h>
#endif

#include <logging/log.h>

#define SERVICE_INFO_GPS "{\"state\":{\"reported\":{\"device\": \
			  {\"serviceInfo\":{\"ui\":[\"GPS\"]}}}}}"

LOG_MODULE_REGISTER(agps_sample, CONFIG_AGPS_SAMPLE_LOG_LEVEL);

static struct cloud_backend *cloud_backend;
static const struct device *gps_dev;
static uint64_t start_search_timestamp;
static uint64_t fix_timestamp;
static struct k_work gps_start_work;
static struct k_work_delayable reboot_work;
#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work manage_pgps_work;
static struct k_work notify_pgps_work;
static struct gps_agps_request agps_request;
#endif

static void gps_start_work_fn(struct k_work *work);

#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct nrf_cloud_pgps_prediction *prediction;

void pgps_handler(enum nrf_cloud_pgps_event event,
		  struct nrf_cloud_pgps_prediction *p)
{
	/* GPS unit asked for it, but we didn't have it; check now */
	LOG_INF("event: %d", event);
	if (event == PGPS_EVT_AVAILABLE) {
		prediction = p;
		k_work_submit(&manage_pgps_work);
	}
}

static void manage_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_INF("Sending prediction to modem...");
	err = nrf_cloud_pgps_inject(prediction, &agps_request, NULL);
	if (err) {
		LOG_ERR("Unable to send prediction to modem: %d", err);
	}

	err = nrf_cloud_pgps_preemptive_updates();
	if (err) {
		LOG_ERR("Error requesting updates: %d", err);
	}
}

static void notify_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("error requesting notification of prediction availability: %d", err);
	}
}
#endif

static void cloud_send_msg(void)
{
	int err;

	LOG_DBG("Publishing message: %s", log_strdup(CONFIG_CLOUD_MESSAGE));

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_MSG,
		.buf = CONFIG_CLOUD_MESSAGE,
		.len = sizeof(CONFIG_CLOUD_MESSAGE)
	};

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}
}

static void gps_start_work_fn(struct k_work *work)
{
	int err;
	struct gps_config gps_cfg = {
		.nav_mode = GPS_NAV_MODE_PERIODIC,
		.power_mode = GPS_POWER_MODE_DISABLED,
		.timeout = 120,
		.interval = 240,
		.priority = true,
	};

	ARG_UNUSED(work);

#if defined(CONFIG_NRF_CLOUD_PGPS)
	static bool initialized;

	if (!initialized) {
		struct nrf_cloud_pgps_init_param param = {
			.event_handler = pgps_handler,
			.storage_base = PM_MCUBOOT_SECONDARY_ADDRESS,
			.storage_size = PM_MCUBOOT_SECONDARY_SIZE};

		err = nrf_cloud_pgps_init(&param);
		if (err) {
			LOG_ERR("Error from PGPS init: %d", err);
		} else {
			initialized = true;
		}
	}
#endif

	err = gps_start(gps_dev, &gps_cfg);
	if (err) {
		LOG_ERR("Failed to start GPS, error: %d", err);
		return;
	}

	LOG_INF("Periodic GPS search started with interval %d s, timeout %d s",
		gps_cfg.interval, gps_cfg.timeout);
}

static void on_agps_needed(struct gps_agps_request request)
{
#if defined(CONFIG_AGPS)
	int err = gps_agps_request_send(request, GPS_SOCKET_NOT_PROVIDED);

	if (err) {
		LOG_ERR("Failed to request A-GPS data, error: %d", err);
		return;
	}
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* AGPS data not expected, so move on to PGPS */
	if (!nrf_cloud_agps_request_in_progress()) {
		k_work_submit(&notify_pgps_work);
	}
#endif
}

static void send_service_info(void)
{
	int err;
	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_STATE,
		.buf = SERVICE_INFO_GPS,
		.len = strlen(SERVICE_INFO_GPS)
	};

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("Failed to send message to cloud, error: %d", err);
		return;
	}

	LOG_INF("Service info sent to cloud");
}

static void cloud_event_handler(const struct cloud_backend *const backend,
				const struct cloud_event *const evt,
				void *user_data)
{
	int err = 0;

	ARG_UNUSED(backend);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTING:
		LOG_INF("CLOUD_EVT_CONNECTING");
		break;
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		/* Update nRF Cloud with GPS service info signifying that it
		 * has GPS capabilities. */
		send_service_info();
		k_work_submit(&gps_start_work);
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");

		/* Convenience functionality for remote testing.
		 * The device is reset if it receives "{"reboot":true}"
		 * from the cloud. The command can be sent using the terminal
		 * card on the device page on nrfcloud.com.
		 */
		if (evt->data.msg.buf[0] == '{') {
			int ret = strncmp(evt->data.msg.buf,
				      "{\"reboot\":true}",
				      strlen("{\"reboot\":true}"));

			if (ret == 0) {
				/* The work item may already be scheduled
				 * because of the button being pressed,
				 * so use rescheduling here to get the work
				 * submitted with no delay.
				 */
				k_work_reschedule(&reboot_work, K_NO_WAIT);
			}
			break;
		}

#if defined(CONFIG_AGPS)
		err = gps_process_agps_data(evt->data.msg.buf, evt->data.msg.len);
		if (!err) {
			LOG_INF("A-GPS data processed");
#if defined(CONFIG_NRF_CLOUD_PGPS)
			/* call us back when prediction is ready */
			k_work_submit(&notify_pgps_work);
#endif
			/* data was valid; no need to pass to other handlers */
			break;
		}
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
		err = nrf_cloud_pgps_process(evt->data.msg.buf, evt->data.msg.len);
		if (err) {
			LOG_ERR("Error processing PGPS packet: %d", err);
		}
#else
		if (err) {
			LOG_INF("Unable to process agps data, error: %d", err);
		}
#endif
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		break;
	case CLOUD_EVT_FOTA_ERROR:
		LOG_INF("CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_INF("Unknown cloud event type: %d", evt->type);
		break;
	}
}

static void print_pvt_data(struct gps_pvt *pvt_data)
{
	char buf[300];
	size_t len;

	len = snprintf(buf, sizeof(buf),
		      "\r\n\tLongitude:  %f\r\n\t"
		      "Latitude:   %f\r\n\t"
		      "Altitude:   %f\r\n\t"
		      "Speed:      %f\r\n\t"
		      "Heading:    %f\r\n\t"
		      "Date:       %02u-%02u-%02u\r\n\t"
		      "Time (UTC): %02u:%02u:%02u\r\n",
		      pvt_data->longitude, pvt_data->latitude,
		      pvt_data->altitude, pvt_data->speed, pvt_data->heading,
		      pvt_data->datetime.year, pvt_data->datetime.month,
		      pvt_data->datetime.day, pvt_data->datetime.hour,
		      pvt_data->datetime.minute, pvt_data->datetime.seconds);
	if (len < 0) {
		LOG_ERR("Could not construct PVT print");
	} else {
		LOG_INF("%s", log_strdup(buf));
	}
}

static void print_satellite_stats(struct gps_pvt *pvt_data)
{
	uint8_t tracked = 0;
	uint32_t tracked_sats = 0;
	static uint32_t prev_tracked_sats;
	char print_buf[100];
	size_t print_buf_len;

	for (int i = 0; i < GPS_PVT_MAX_SV_COUNT; ++i) {
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
	LOG_DBG("Searching for %lld seconds",
		(k_uptime_get() - start_search_timestamp) / 1000);
}

static void send_nmea(char *nmea)
{
	int err;
	char buf[150];
	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_MSG,
		.buf = buf,
	};

	msg.len = snprintf(buf, sizeof(buf),
		"{"
			"\"appId\":\"GPS\","
			"\"data\":\"%s\","
			"\"messageType\":\"DATA\""
		"}", nmea);
	if (msg.len < 0) {
		LOG_ERR("Failed to create GPS cloud message");
		return;
	}

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("Failed to send message to cloud, error: %d", err);
		return;
	}

	LOG_INF("GPS position sent to cloud");
}

static void gps_handler(const struct device *dev, struct gps_event *evt)
{
	ARG_UNUSED(dev);

	switch (evt->type) {
	case GPS_EVT_SEARCH_STARTED:
		LOG_INF("GPS_EVT_SEARCH_STARTED");
		start_search_timestamp = k_uptime_get();
		break;
	case GPS_EVT_SEARCH_STOPPED:
		LOG_INF("GPS_EVT_SEARCH_STOPPED");
		break;
	case GPS_EVT_SEARCH_TIMEOUT:
		LOG_INF("GPS_EVT_SEARCH_TIMEOUT");
		break;
	case GPS_EVT_OPERATION_BLOCKED:
		LOG_INF("GPS_EVT_OPERATION_BLOCKED");
		break;
	case GPS_EVT_OPERATION_UNBLOCKED:
		LOG_INF("GPS_EVT_OPERATION_UNBLOCKED");
		break;
	case GPS_EVT_AGPS_DATA_NEEDED:
		LOG_INF("GPS_EVT_AGPS_DATA_NEEDED");
#if defined(CONFIG_NRF_CLOUD_PGPS)
		LOG_INF("A-GPS request from modem; emask:0x%08X amask:0x%08X utc:%u "
			"klo:%u neq:%u tow:%u pos:%u int:%u",
			evt->agps_request.sv_mask_ephe, evt->agps_request.sv_mask_alm,
			evt->agps_request.utc, evt->agps_request.klobuchar,
			evt->agps_request.nequick, evt->agps_request.system_time_tow,
			evt->agps_request.position, evt->agps_request.integrity);
		memcpy(&agps_request, &evt->agps_request, sizeof(agps_request));
#endif
		on_agps_needed(evt->agps_request);
		break;
	case GPS_EVT_PVT:
		print_satellite_stats(&evt->pvt);
		break;
	case GPS_EVT_PVT_FIX:
		fix_timestamp = k_uptime_get();

		LOG_INF("---------       FIX       ---------");
		LOG_INF("Time to fix: %d seconds",
			(uint32_t)(fix_timestamp - start_search_timestamp) / 1000);
		print_pvt_data(&evt->pvt);
		LOG_INF("-----------------------------------");
#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_PGPS_STORE_LOCATION)
		nrf_cloud_pgps_set_location(evt->pvt.latitude, evt->pvt.longitude);
#endif
		break;
	case GPS_EVT_NMEA_FIX:
		send_nmea(evt->nmea.buf);
		break;
	default:
		break;
	}
}

static void reboot_work_fn(struct k_work *work)
{
	LOG_WRN("Rebooting in 2 seconds...");
	k_sleep(K_SECONDS(2));
	sys_reboot(0);
}

static void work_init(void)
{
	k_work_init(&gps_start_work, gps_start_work_fn);
	k_work_init_delayable(&reboot_work, reboot_work_fn);
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&manage_pgps_work, manage_pgps);
	k_work_init(&notify_pgps_work, notify_pgps);
#endif
}

static int modem_configure(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		LOG_INF("Connecting to LTE network. This may take minutes.");

#if defined(CONFIG_LTE_POWER_SAVING_MODE)
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("PSM request failed, error: %d", err);
			return err;
		}

		LOG_INF("PSM mode requested");
#endif

		err = lte_lc_init_and_connect();
		if (err) {
			LOG_ERR("LTE link could not be established, error: %d",
				err);
			return err;
		}

		LOG_INF("Connected to LTE network");
	}

	return err;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		cloud_send_msg();
		k_work_schedule(&reboot_work, K_SECONDS(3));
	} else if (has_changed & ~button_states & DK_BTN1_MSK) {
		k_work_cancel_delayable(&reboot_work);
	}
}

#if defined(CONFIG_DATE_TIME)
static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		LOG_INF("DATE_TIME_OBTAINED_MODEM");
		break;
	case DATE_TIME_OBTAINED_NTP:
		LOG_INF("DATE_TIME_OBTAINED_NTP");
		break;
	case DATE_TIME_OBTAINED_EXT:
		LOG_INF("DATE_TIME_OBTAINED_EXT");
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("DATE_TIME_NOT_OBTAINED");
		break;
	default:
		break;
	}
}
#endif

void main(void)
{
	int err;

	LOG_INF("A-GPS sample has started");

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend, "Could not get binding to cloud backend");

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			err);
		return;
	}

	work_init();

	err = modem_configure();
	if (err) {
		LOG_ERR("Modem configuration failed with error %d",
			err);
		return;
	}

	gps_dev = device_get_binding("NRF9160_GPS");
	if (gps_dev == NULL) {
		LOG_ERR("Could not get binding to nRF9160 GPS");
		return;
	}

	err = gps_init(gps_dev, gps_handler);
	if (err) {
		LOG_ERR("Could not initialize GPS, error: %d", err);
		return;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Buttons could not be initialized, error: %d", err);
		LOG_WRN("Continuing without button funcitonality");
	}

#if defined(CONFIG_DATE_TIME)
	date_time_update_async(date_time_event_handler);
#endif

	err = cloud_connect(cloud_backend);
	if (err) {
		LOG_ERR("Cloud connection failed, error: %d", err);
		return;
	}

	/* The cloud connection is polled in a thread in the backend.
	 * Events will be received to cloud_event_handler() when data is
	 * received from the cloud.
	 */
}
