/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud_rest.h>
#include <modem/modem_jwt.h>
#include <power/reboot.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>
#include <net/nrf_cloud_agps.h>
#include <nrf_modem_gnss.h>
#include <modem/agps.h>

#if defined(CONFIG_REST_ID_SRC_COMPILE_TIME)
BUILD_ASSERT(sizeof(CONFIG_REST_DEVICE_ID) > 1, "Device ID must be specified");
#elif defined(CONFIG_REST_ID_SRC_IMEI)
#define CGSN_RSP_LEN 19
#define IMEI_LEN 15
#define IMEI_CLIENT_ID_LEN (sizeof(CONFIG_REST_ID_PREFIX) \
			    - 1 + IMEI_LEN)
BUILD_ASSERT(IMEI_CLIENT_ID_LEN <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"REST_ID_PREFIX plus IMEI must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

LOG_MODULE_REGISTER(rest_agps_sample, CONFIG_NRF_CLOUD_REST_AGPS_SAMPLE_LOG_LEVEL);

#define REST_RX_BUF_SZ 2100
#define BUTTON_EVENT_BTN_NUM CONFIG_REST_BUTTON_EVT_NUM
#define LED_NUM CONFIG_REST_LED_NUM
#define JITP_REQ_WAIT_SEC 10
#define NCELL_MEAS_CNT 17
#define FAILURE_LIMIT 10

static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

static K_SEM_DEFINE(button_press_sem, 0, 1);
static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(cell_data_ready, 0, 1);
static K_SEM_DEFINE(do_agps_request_sem, 0, 1);

static struct modem_param_info modem_info;
static struct lte_lc_cell current_cell;

static struct nrf_modem_gnss_agps_data_frame agps_request;
static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static struct nrf_modem_gnss_nmea_data_frame last_nmea;
static bool jitp_requested;

static bool disconnected = true;

void agps_print_enable(bool enable);

static int set_device_id(void)
{
	int err = 0;

#if defined(CONFIG_REST_ID_SRC_COMPILE_TIME)

	memcpy(device_id, CONFIG_REST_DEVICE_ID, strlen(CONFIG_REST_DEVICE_ID));
	return 0;

#elif defined(CONFIG_REST_ID_SRC_IMEI)

	char imei_buf[CGSN_RSP_LEN + 1];

	err = at_cmd_write("AT+CGSN", imei_buf, sizeof(imei_buf), NULL);
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		return err;
	}

	imei_buf[IMEI_LEN] = 0;

	err = snprintk(device_id, sizeof(device_id), "%s%.*s",
		       CONFIG_REST_ID_PREFIX,
		       IMEI_LEN, imei_buf);
	if (err < 0 || err >= sizeof(device_id)) {
		return -EIO;
	}

	return 0;

#elif defined(CONFIG_REST_ID_SRC_INTERNAL_UUID)

	struct nrf_device_uuid dev_id;

	err = modem_jwt_get_uuids(&dev_id, NULL);
	if (err) {
		LOG_ERR("Failed to get device UUID: %d", err);
		return err;
	}
	memcpy(device_id, dev_id.str, sizeof(dev_id.str));

	return 0;

#endif

	return -ENOTRECOVERABLE;
}

static void print_pvt_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
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

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;
	uint32_t tracked_sats = 0;
	static uint32_t prev_tracked_sats;
	char print_buf[100];
	size_t print_buf_len;

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

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_INF("GNSS_EVT_PERIODIC_WAKEUP");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_INF("GNSS_EVT_SLEEP_AFTER_FIX");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_INF("GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		break;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_INF("GNSS_EVT_BLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		LOG_INF("GNSS_EVT_UNBLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		LOG_INF("GNSS_EVT_AGPS_REQ");
		ret = nrf_modem_gnss_read(&agps_request, sizeof(agps_request),
					  NRF_MODEM_GNSS_DATA_AGPS_REQ);
		if (ret) {
			break;
		}
		k_sem_give(&do_agps_request_sem);
		break;
	case NRF_MODEM_GNSS_EVT_PVT:
		ret = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (!ret) {
			print_satellite_stats(&last_pvt);
		}
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_INF("GNSS_EVT_FIX");
		ret = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (!ret) {
			print_pvt_data(&last_pvt);
		}
		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
		if (IS_ENABLED(CONFIG_REST_NMEA)) {
			ret = nrf_modem_gnss_read(&last_nmea, sizeof(last_nmea),
						     NRF_MODEM_GNSS_DATA_NMEA);
			if (!ret) {
				LOG_INF("NMEA: %s", log_strdup(last_nmea.nmea_str));
			}
		}
		break;
	default:
		break;
	}
}

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

	(void)k_sem_take(&button_press_sem, K_NO_WAIT);

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

static int get_modem_info(struct modem_param_info *const modem_info)
{
	__ASSERT_NO_MSG(modem_info != NULL);

	int err = modem_info_init();

	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d", err);
		return err;
	}

	err = modem_info_params_init(modem_info);
	if (err) {
		LOG_ERR("Could not initialize modem info parameters, error: %d", err);
		return err;
	}

	err = modem_info_params_get(modem_info);
	if (err) {
		LOG_ERR("Could not obtain cell information, error: %d", err);
		return err;
	}

	return 0;
}

static int init_gnss(void)
{
	int err;

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
	if (err) {
		LOG_ERR("Unable to activate GPS mode: %d", err);
		return err;
	}

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

	err = nrf_modem_gnss_fix_retry_set(120);
	if (err) {
		LOG_ERR("Failed to set GNSS fix retry: %d", err);
		return err;
	}

	err = nrf_modem_gnss_fix_interval_set(240);
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

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		LOG_ERR("Failed to initialize modem library: %d", err);
		LOG_ERR("This can occur after a modem FOTA.");
		return err;
	}

	err = at_cmd_init();
	if (err) {
		LOG_ERR("Failed to initialize AT commands, err %d", err);
		return err;
	}

	err = init_gnss();
	if (err) {
		return err;
	}
	LOG_INF("GPS started");

	err = set_device_id();
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
			disconnected = true;
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

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, err %d", err);
	} else {
		k_sem_take(&lte_connected, K_FOREVER);
		LOG_INF("Connected");
		(void)set_led(1);

		/* grab network info for part of A-GPS request later */
		err = get_modem_info(&modem_info);
		if (err) {
			LOG_ERR("Error getting network information: %d", err);
		} else {
			current_cell.mcc = modem_info.network.mcc.value;
			current_cell.mnc = modem_info.network.mnc.value;
		}
	}

	return err;
}


void main(void)
{
	LOG_INF("Starting nRF Cloud REST A-GPS API Sample");

	char rx_buf[REST_RX_BUF_SZ];
	int agps_sz;
	struct jwt_data jwt = {
#if defined(CONFIG_REST_ID_SRC_INTERNAL_UUID)
		.subject = NULL,
#else
		.subject = device_id,
#endif
		.audience = NULL,
		.exp_delta_s = 300,
		.sec_tag = CONFIG_REST_JWT_SEC_TAG,
		.key = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.jwt_buf = NULL
	};
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = true,
		.timeout_ms = 30000,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0
	};
	struct lte_lc_cells_info net_info = {
		.ncells_count = 0,
		.neighbor_cells = NULL
	};
	struct nrf_cloud_rest_agps_request agps_req = {
		.type = NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
		.agps_req = &agps_request,
		.net_info = &net_info
	};
	struct nrf_cloud_rest_agps_result agps_result = {
		.buf_sz = 0,
		.buf = NULL
	};
	int err;
	int failure_count = 0;

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		failure_count = FAILURE_LIMIT;
		goto cleanup;
	}

	err = connect_to_network();
	if (err) {
		failure_count++;
		goto cleanup;
	}

	agps_print_enable(false);

	(void)do_jitp();

retry:
	LOG_INF("Waiting for AGPS request event..");
	k_sem_take(&do_agps_request_sem, K_FOREVER);

	if (disconnected) {
		err = connect_to_network();
		if (err) {
			failure_count++;
			goto cleanup;
		}
	}

	err = modem_jwt_generate(&jwt);
	if (err) {
		LOG_ERR("Failed to generate JWT, err %d", err);
		failure_count++;
		goto cleanup;
	}
	LOG_DBG("JWT:\n%s", log_strdup(jwt.jwt_buf));
	rest_ctx.auth = jwt.jwt_buf;

	LOG_INF("\n********************* A-GPS API *********************");
	net_info.current_cell = current_cell; /* update with latest cell tower info */
	rest_ctx.fragment_size = 1; /* set size small so we query buffer size needed */
	agps_sz = nrf_cloud_rest_agps_data_get(&rest_ctx, &agps_req, NULL);
	if (agps_sz < 0) {
		LOG_ERR("Failed to get A-GPS data: %d", agps_sz);
		failure_count++;
		goto cleanup;
	} else if (agps_sz == 0) {
		LOG_WRN("A-GPS data size is zero, skipping");
		goto cleanup;
	}

	LOG_INF("Additional buffer required to download A-GPS data of %d bytes",
		agps_sz);

	agps_result.buf_sz = (uint32_t)agps_sz;
	agps_result.buf = k_calloc(agps_result.buf_sz, 1);
	if (!agps_result.buf) {
		LOG_ERR("Failed to allocate %u bytes for A-GPS buffer", agps_result.buf_sz);
		failure_count = FAILURE_LIMIT;
		goto cleanup;
	}
	/* Use the default configured fragment size */
	rest_ctx.fragment_size = 0;
	err = nrf_cloud_rest_agps_data_get(&rest_ctx, &agps_req, &agps_result);
	if (err) {
		LOG_ERR("Failed to get A-GPS data: %d", err);
		failure_count++;
	} else {
		LOG_INF("Successfully received A-GPS data");
		failure_count = 0;
		err = agps_cloud_data_process(agps_result.buf, agps_result.agps_sz);
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
	modem_jwt_free(jwt.jwt_buf);
	jwt.jwt_buf = NULL;

	if (err && (failure_count >= FAILURE_LIMIT)) {
		LOG_INF("Rebooting in 30s..");
		k_sleep(K_SECONDS(30));
	} else {
		if (failure_count) {
			LOG_INF("Retrying in %d minute(s)..",
				CONFIG_REST_RETRY_WAIT_TIME_MIN);
			k_sleep(K_MINUTES(CONFIG_REST_RETRY_WAIT_TIME_MIN));
		}
		goto retry;
	}

	sys_reboot(SYS_REBOOT_COLD);
}
