/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <ctype.h>
#include <modem/lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <net/nrf_cloud.h>
#include <sys/reboot.h>
#include <dk_buttons_and_leds.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(cloud_client, CONFIG_CLOUD_CLIENT_LOG_LEVEL);

static struct cloud_backend *cloud_backend;
static struct k_work_delayable cloud_update_work;
static struct k_work_delayable connect_work;
static struct k_work device_status_work;

static K_SEM_DEFINE(lte_connected, 0, 1);

/* Flag to signify if the cloud client is connected or not connected to cloud,
 * used to abort/allow cloud publications.
 */
static bool cloud_connected;

static void device_status_send(struct k_work *work)
{
	static bool sent;

	if (sent) {
		return;
	}

	struct nrf_cloud_svc_info_fota fota = {
		.bootloader = false,
		.modem = true,
		.application = true
	};
	struct nrf_cloud_svc_info svc = {
		.fota = &fota,
		.ui = NULL
	};
	struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = &svc
	};
	int ret;

	ret = nrf_cloud_shadow_device_status_update(&dev_status);
	if (ret) {
		LOG_ERR("Unable to encode cloud data: %d", ret);
	} else {
		sent = true;
		LOG_INF("Updated device shadow");
	}
}

static void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	err = cloud_connect(cloud_backend);
	if (err) {
		LOG_ERR("cloud_connect, error: %d", err);
	}

	LOG_INF("Next connection retry in %d seconds",
	       CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work,
		K_SECONDS(CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS));
}

static void cloud_update_work_fn(struct k_work *work)
{
	int err;

	if (!cloud_connected) {
		LOG_INF("Not connected to cloud, abort cloud publication");
		return;
	}

	LOG_INF("Publishing message: %s", log_strdup(CONFIG_CLOUD_MESSAGE));

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = CONFIG_CLOUD_MESSAGE,
		.len = strlen(CONFIG_CLOUD_MESSAGE)
	};

	/* When using the nRF Cloud backend data is sent to the message topic.
	 * This is in order to visualize the data in the web UI terminal.
	 * For Azure IoT Hub and AWS IoT, messages are addressed directly to the
	 * device twin (Azure) or device shadow (AWS).
	 */
	if (strcmp(CONFIG_CLOUD_BACKEND, "NRF_CLOUD") == 0) {
		msg.endpoint.type = CLOUD_EP_MSG;
	} else {
		msg.endpoint.type = CLOUD_EP_STATE;
	}

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}

#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
	k_work_schedule(&cloud_update_work,
			K_SECONDS(CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL));
#endif
}

void fota_done_handler(const struct cloud_event *const evt)
{
#if defined(CONFIG_NRF_CLOUD_FOTA)
	enum nrf_cloud_fota_type fota_type = NRF_CLOUD_FOTA_TYPE__INVALID;

	if (evt && evt->data.msg.buf) {
		fota_type = *(enum nrf_cloud_fota_type *)evt->data.msg.buf;
		LOG_INF("FOTA type: %d", fota_type);
	}
#endif
#if defined(CONFIG_LTE_LINK_CONTROL)
	lte_lc_power_off();
#endif
	LOG_INF("Rebooting to complete FOTA update");
	sys_reboot(SYS_REBOOT_COLD);
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);
	ARG_UNUSED(backend);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTING:
		LOG_INF("CLOUD_EVT_CONNECTING");
		break;
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&connect_work);
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
#if defined(CONFIG_BOOTLOADER_MCUBOOT) && !defined(CONFIG_NRF_CLOUD_FOTA)
		/* Mark image as good to avoid rolling back after update */
		LOG_INF("Marking any FOTA image as confirmed")
		boot_write_img_confirmed();
#endif
		k_work_submit(&device_status_work);
#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
		k_work_reschedule(&cloud_update_work, K_SECONDS(5));
#endif
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		cloud_connected = false;
		k_work_reschedule(&connect_work, K_NO_WAIT);
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		LOG_INF("Data received from cloud: %.*s",
			evt->data.msg.len,
			log_strdup(evt->data.msg.buf));
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		fota_done_handler(evt);
		break;
	case CLOUD_EVT_FOTA_ERROR:
		LOG_INF("CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_INF("Unknown cloud event type: %d", evt->type);
		break;
	}
}

static void work_init(void)
{
	k_work_init_delayable(&cloud_update_work, cloud_update_work_fn);
	k_work_init_delayable(&connect_work, connect_work_fn);
	k_work_init(&device_status_work, device_status_send);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("Active LTE mode changed: %s",
			evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			"Unknown");
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		LOG_INF("Modem domain event, type: %s",
			evt->modem_evt == LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE ?
				"Light search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_SEARCH_DONE ?
				"Search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP ?
				"Reset loop detected" :
			evt->modem_evt == LTE_LC_MODEM_EVT_BATTERY_LOW ?
				"Low battery" :
			evt->modem_evt == LTE_LC_MODEM_EVT_OVERHEATED ?
				"Modem is overheated" :
				"Unknown");
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err;

#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d",
				err);
		}

		LOG_INF("PSM mode requested");
#endif

		err = lte_lc_modem_events_enable();
		if (err) {
			LOG_ERR("lte_lc_modem_events_enable failed, error: %d",
				err);
			return;
		}

		err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d",
				err);
			return;
		}

		/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
		 * lte_handler() to determine when the LTE link is up.
		 */
	}
#endif
}

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		k_work_reschedule(&cloud_update_work, K_NO_WAIT);
	}
}
#endif

void main(void)
{
	int err;

	LOG_INF("Cloud client has started");

	cloud_backend = cloud_get_binding(CONFIG_CLOUD_BACKEND);
	__ASSERT(cloud_backend != NULL, "%s backend not found",
		 CONFIG_CLOUD_BACKEND);

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			err);
		LOG_INF("Rebooting");
		sys_reboot(SYS_REBOOT_COLD);
	}

	work_init();
	modem_configure();

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
#endif
	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);

	LOG_INF("Connected to LTE network");
	LOG_INF("Connecting to cloud");

	k_work_schedule(&connect_work, K_NO_WAIT);
}
