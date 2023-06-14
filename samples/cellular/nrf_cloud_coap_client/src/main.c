/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <modem/location.h>
#include <zephyr/random/rand32.h>
#include <dk_buttons_and_leds.h>
#include <nrf_socket.h>
#include <nrf_modem_at.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <version.h>
#include "handle_fota.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_cloud_coap_client, CONFIG_NRF_CLOUD_COAP_CLIENT_LOG_LEVEL);

#define SAMPLE_SIGNON_FMT "nRF Cloud CoAP Client Sample, version: %s"

#define CREDS_REQ_WAIT_SEC 10
#define BTN_NUM 1
#define APP_COAP_SEND_INTERVAL_MS 20000
#define APP_COAP_INTERVAL_LIMIT 60

/* Uncomment to incrementally increase time between coap packets */
/* #define DELAY_INTERPACKET_PERIOD */

static bool connected;

/* Modem info struct used for modem FW version and cell info used for single-cell requests */
#if defined(CONFIG_MODEM_INFO)
static struct modem_param_info mdm_param;
#endif

/* Current RRC mode */
static enum lte_lc_rrc_mode cur_rrc_mode = LTE_LC_RRC_MODE_IDLE;

static volatile bool button_pressed;

static struct nrf_cloud_gnss_data gnss;

static K_SEM_DEFINE(location_event, 0, 1);

static int update_shadow(void);
static void button_handler(uint32_t button_states, uint32_t has_changed);

static void check_modem_fw_version(void)
{
	char mfwv_str[128];

#if defined(CONFIG_MODEM_INFO)
	if (modem_info_string_get(MODEM_INFO_FW_VERSION, mfwv_str, sizeof(mfwv_str)) <= 0) {
		LOG_WRN("Failed to get modem FW version");
		return;
	}
#else
	strncpy(mfwv_str, "1.3.0", sizeof(mfwv_str));
#endif
	LOG_INF("Modem FW version: %s", mfwv_str);
}

#if defined(CONFIG_NRF_MODEM_LIB)
/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("Modem library recoverable error: %u", (unsigned int)err);
}
#endif /* defined(CONFIG_NRF_MODEM_LIB) */

K_SEM_DEFINE(lte_ready, 0, 1);

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_DBG("Connected to LTE network");
			k_sem_give(&lte_ready);
		} else {
			LOG_DBG("reg status %d", evt->nw_reg_status);
		}
		break;

	case LTE_LC_EVT_RRC_UPDATE:
		cur_rrc_mode = evt->rrc_mode;
		if (cur_rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			LOG_DBG("RRC mode: idle");
		} else {
			LOG_DBG("RRC mode: connected");
		}
		break;

	default:
		LOG_DBG("LTE event %d (0x%x)", evt->type, evt->type);
		break;
	}
}

static void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_INF("Got location:");
		LOG_INF("  method: %s", location_method_str(event_data->method));
		LOG_INF("  latitude: %.06f", event_data->location.latitude);
		LOG_INF("  longitude: %.06f", event_data->location.longitude);
		LOG_INF("  accuracy: %.01f m", event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			LOG_INF("  date: %04d-%02d-%02d",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			LOG_INF("  time: %02d:%02d:%02d.%03d UTC",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
		}
		LOG_INF("  Google maps URL: https://maps.google.com/?q=%.06f,%.06f\n",
			event_data->location.latitude, event_data->location.longitude);
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_INF("Getting location timed out");
		break;

	case LOCATION_EVT_ERROR:
		LOG_INF("Getting location failed: num satellites: %d",
			event_data->error.details.gnss.satellites_tracked);
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		LOG_INF("Getting location assistance requested (A-GPS). Not doing anything.");
		break;

	case LOCATION_EVT_GNSS_PREDICTION_REQUEST:
		LOG_INF("Getting location assistance requested (P-GPS). Not doing anything.");
		break;

	default:
		LOG_INF("Getting location: Unknown event: %d", event_data->id);
		break;
	}

	k_sem_give(&location_event);
}

static void location_event_wait(void)
{
	k_sem_take(&location_event, K_FOREVER);
}

/**
 * @brief Retrieve location so that fallback is applied.
 *
 * @details This is achieved by setting GNSS as first priority method and giving it too short
 * timeout. Then a fallback to next method, which is cellular in this example, occurs.
 */
static int location_cellular_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);
	/* Default cellular configuration may be overridden here. */
	config.methods[1].cellular.timeout = 40 * MSEC_PER_SEC;

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return err;
	}

	location_event_wait();
	return 0;
}

/**
 * @brief Retrieve location with GNSS high accuracy.
 */
static int location_gnss_high_accuracy_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_HIGH;

	printk("Requesting high accuracy GNSS location...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return err;
	}

	location_event_wait();
	return 0;
}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
/**
 * @brief Retrieve location with Wi-Fi positioning as first priority, GNSS as second
 * and cellular as third.
 */
static int location_wifi_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {
		LOCATION_METHOD_WIFI};

	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

	printk("Requesting Wi-Fi location...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return err;
	}

	location_event_wait();
	return 0;
}
#endif

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err < 0) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	} else if (err == NRF_MODEM_DFU_RESULT_OK) {
		LOG_DBG("Modem library initialized after "
			"successful modem firmware update.");
	} else if (err > 0) {
		LOG_ERR("Modem library initialized after "
			"failed modem firmware update, error: %d", err);
		return err;
	} else {
		LOG_DBG("Modem library initialized.");
	}

	lte_lc_register_handler(lte_handler);
	LOG_INF("LTE Link Connecting ...");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	k_sem_take(&lte_ready, K_FOREVER);
	LOG_INF("LTE Link Connected");
	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("Unable to enter PSM mode: %d", err);
	}

	err = nrf_modem_at_printf("AT+CEREG=5");
	if (err) {
		LOG_ERR("Can't subscribe to +CEREG events.");
	}

	/* Modem info library is used to obtain the modem FW version
	 * and network info for single-cell requests
	 */
#if defined(CONFIG_MODEM_INFO)
	err = modem_info_init();
	if (err) {
		LOG_ERR("Modem info initialization failed, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&mdm_param);
	if (err) {
		LOG_ERR("Modem info params initialization failed, error: %d", err);
		return err;
	}

	err = modem_info_params_get(&mdm_param);
	if (err) {
		LOG_ERR("Modem info params reading failed, error: %d", err);
	}
#endif

	/* Check modem FW version */
	check_modem_fw_version();

	return err;
}

int init(void)
{
	int err;

	err = handle_fota_init();
	if (err) {
		LOG_ERR("Error initializing FOTA: %d", err);
		return err;
	}

	err = modem_configure();
	if (err) {
		return err;
	}

	err = handle_fota_begin();
	if (err) {
		return err;
	}

	err = nrf_cloud_coap_init();
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}

	err = nrf_cloud_coap_connect();
	if (err) {
		LOG_ERR("Failed to connect and get authorized: %d", err);
		return err;
	}
	connected = true;

	err = update_shadow();
	if (err) {
		LOG_ERR("Error updating shadow: %d", err);
		return err;
	}
	LOG_INF("Shadow updated");

	err = location_init(location_event_handler);
	if (err) {
		LOG_ERR("Error initializing location library: %d", err);
	}
	LOG_INF("Location library initialized");

	/* Init the button */
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: error: %d", err);
	}

	return err;
}

static int update_shadow(void)
{
	/* Enable FOTA for modem and application */
	struct nrf_cloud_svc_info_fota fota = {
		.modem = 1,
		.application = 1
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gnss = true,
		.temperature = true
	};
	struct nrf_cloud_svc_info service_info = {
		.fota = &fota,
		.ui = &ui_info
	};
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM) ? NRF_CLOUD_INFO_SET : 0,
		/* Use the modem info already obtained */
#if defined(CONFIG_MODEM_INFO)
		.mpi = &mdm_param,
#endif
		/* Include the application version */
		.application_version = CONFIG_NRF_CLOUD_COAP_CLIENT_SAMPLE_VERSION
	};
	struct nrf_cloud_device_status device_status = {
		.modem = &modem_info,
		.svc = &service_info
	};

	return nrf_cloud_coap_shadow_device_status_update(&device_status);
}

static int do_next_test(void)
{
	static double temp = 21.5;
	static int cur_test = 1;
	int err = 0;
	char buf[512];

	if (!gnss.type) {
		gnss.type = NRF_CLOUD_GNSS_TYPE_PVT;
		gnss.pvt.lat = 45.525616;
		gnss.pvt.lon = -122.685978;
		gnss.pvt.accuracy = 30;
	}

	LOG_INF("\n***********************************************");
	switch (cur_test) {
	case 1:
		LOG_INF("**** %d. Getting pending FOTA job execution ****\n", cur_test);
		err = handle_fota_process();
		if (err != -EAGAIN) {
			LOG_INF("FOTA check completed.");
		}
		break;
	case 2:
		LOG_INF("*** %d. Getting shadow delta *******************\n", cur_test);
		buf[0] = '\0';
		err = nrf_cloud_coap_shadow_get(buf, sizeof(buf), true);
		if (err) {
			LOG_ERR("Failed to request shadow delta: %d", err);
		} else {
			size_t len = strlen(buf);

			LOG_INF("Delta: %s", len ? buf : "None");
			if (len) {
				err = nrf_cloud_coap_shadow_state_update(buf);
				if (err) {
					LOG_ERR("Failed to acknowledge delta: %d", err);
				} else {
					LOG_INF("Delta acknowledged");
				}
			}
		}
		break;
	case 3:
		LOG_INF("*** %d. Sending temperature ********************\n", cur_test);
		err = nrf_cloud_coap_sensor_send(NRF_CLOUD_JSON_APPID_VAL_TEMP, temp,
						 NRF_CLOUD_NO_TIMESTAMP);
		if (err) {
			LOG_ERR("Error sending sensor data: %d", err);
			break;
		}
		LOG_INF("Sent %.1f C", temp);
		temp += 0.1;
		break;
	case 4:
		LOG_INF("*** %d. Getting Cellular Location ****************\n", cur_test);
		err = location_cellular_get();
		break;
	case 5:
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		LOG_INF("*** %d. Getting Wi-Fi Location *******************\n", cur_test);
		k_sleep(K_SECONDS(3));
		err = location_wifi_get();
		break;
#else
		cur_test++;
#endif
	case 6:
		LOG_INF("*** %d. Getting GNSS Location ******************\n", cur_test);
		err = location_gnss_high_accuracy_get();
		break;
	case 7:
		LOG_INF("*** %d. Sending GNSS PVT ***********************\n", cur_test);
		err = nrf_cloud_coap_location_send(&gnss);
		if (err) {
			LOG_ERR("Error sending GNSS PVT data: %d", err);
		} else {
			LOG_INF("PVT sent");
		}
		break;
	}

	if (++cur_test > 7) {
		cur_test = 1;
	}
	return err;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(BTN_NUM - 1)) {
		LOG_INF("Button %d pressed", BTN_NUM);
		button_pressed = true;
	}
}

int main(void)
{
	int64_t next_msg_time;
	int delta_ms = APP_COAP_SEND_INTERVAL_MS;
	int err;
	int i = 1;

	LOG_INF(SAMPLE_SIGNON_FMT,
		CONFIG_NRF_CLOUD_COAP_CLIENT_SAMPLE_VERSION);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed. Stopping.");
		return 0;
	}

	next_msg_time = k_uptime_get() + delta_ms;

	while (1) {
		if (k_uptime_get() >= next_msg_time) {

			if (!connected) {
				LOG_INF("Going online");
				err = lte_lc_normal();
				if (err) {
					LOG_ERR("Error going online: %d", err);
				} else {
					k_sem_take(&lte_ready, K_FOREVER);
					err = nrf_cloud_coap_connect();
					if (err) {
						LOG_ERR("Failed to connect and get authorized: %d",
							err);
						next_msg_time += delta_ms;
						continue;
					}
					connected = true;
				}
			}

			if (connected) {
				err = do_next_test();

				if ((err == -EAGAIN) || (err == -EFAULT) || button_pressed) {
					connected = false;
					if (button_pressed) {
						LOG_INF("Disconnecting due to button press...");
						button_pressed = false;
					}
					LOG_INF("Disconnecting CoAP");
					err = nrf_cloud_coap_disconnect();
					if (err) {
						LOG_ERR("Error closing socket: %d", err);
					} else {
						LOG_INF("Socket closed.");
					}
					LOG_INF("LTE going offline");
					err = lte_lc_offline();
					if (err) {
						LOG_ERR("Error going offline: %d", err);
					} else {
						LOG_INF("Offline.");
					}
					k_sleep(K_SECONDS(5));
				}
			}

			delta_ms = APP_COAP_SEND_INTERVAL_MS * i;
			next_msg_time += delta_ms;
#if defined(DELAY_INTERPACKET_PERIOD)
			LOG_INF("Next transfer in %d minutes, %d seconds",
				delta_ms / 60000, (delta_ms / 1000) % 60);
			if (++i > APP_COAP_INTERVAL_LIMIT) {
				i = APP_COAP_INTERVAL_LIMIT;
			}
#endif
		}
		k_sleep(K_MSEC(100));
	}
	return err;
}
