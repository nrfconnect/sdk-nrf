/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <net/azure_iot_hub.h>
#include <net/azure_fota.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/random/rand32.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(azure_fota_sample, CONFIG_AZURE_FOTA_SAMPLE_LOG_LEVEL);

static K_SEM_DEFINE(network_connected_sem, 0, 1);
static K_SEM_DEFINE(cloud_connected_sem, 0, 1);
static bool network_connected;
static struct k_work_delayable reboot_work;

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			 "This sample does not support auto init and connect");

NRF_MODEM_LIB_ON_INIT(azure_fota_init_hook, on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}

static void azure_event_handler(struct azure_iot_hub_evt *const evt)
{
	switch (evt->type) {
	case AZURE_IOT_HUB_EVT_CONNECTING:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTING");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTED:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTED");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTION_FAILED:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTION_FAILED");
		LOG_INF("Error code received from IoT Hub: %d",
			evt->data.err);
		break;
	case AZURE_IOT_HUB_EVT_DISCONNECTED:
		LOG_INF("AZURE_IOT_HUB_EVT_DISCONNECTED");
		break;
	case AZURE_IOT_HUB_EVT_READY:
		LOG_INF("AZURE_IOT_HUB_EVT_READY");
		k_sem_give(&cloud_connected_sem);
		break;
	case AZURE_IOT_HUB_EVT_DATA_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_DATA_RECEIVED");
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RECEIVED");
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED");
		LOG_INF("Desired device property: %.*s",
			evt->data.msg.payload.size,
			evt->data.msg.payload.ptr);
		break;
	case AZURE_IOT_HUB_EVT_DIRECT_METHOD:
		LOG_INF("AZURE_IOT_HUB_EVT_DIRECT_METHOD");
		LOG_INF("Method name: %s", evt->data.method.name.ptr);
		LOG_INF("Payload: %.*s", evt->data.method.payload.size,
			evt->data.method.payload.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: %s",
			evt->data.result.request_id.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL, ID %s, status %d",
			evt->data.result.request_id.ptr, evt->data.result.status);
		break;
	case AZURE_IOT_HUB_EVT_PUBACK:
		LOG_INF("AZURE_IOT_HUB_EVT_PUBACK");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_START:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_START");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_DONE:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_DONE");
		LOG_INF("The device will reboot in 5 seconds to apply update");
		k_work_schedule(&reboot_work, K_SECONDS(5));
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE");
		break;
	case AZURE_IOT_HUB_EVT_ERROR:
		LOG_INF("AZURE_IOT_HUB_EVT_ERROR");
		break;
	default:
		LOG_WRN("Unknown Azure IoT Hub event type: %d", evt->type);
		break;
	}
}

static void reboot_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	sys_reboot(SYS_REBOOT_COLD);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			if (!network_connected) {
				break;
			}

			network_connected = false;

			LOG_INF("LTE network is disconnected");
			LOG_INF("Subsequent sending data may block or fail");
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&network_connected_sem);

		network_connected = true;
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
				"eDRX parameter update: eDRX: %f, PTW: %f",
				evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_INF("%s", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
	int err;

	err = lte_lc_init();
	if (err) {
		LOG_ERR("Failed initializing the link controller, error: %d", err);
		return;
	}

	/* Explicitly disable PSM so that don't go to sleep after the initial connection
	 * establishment. Needed to be able to pick up new FOTA jobs more quickly.
	 */
	err = lte_lc_psm_req(false);
	if (err) {
		LOG_ERR("Failed disabling PSM, error: %d", err);
		return;
	}

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Modem could not be configured, error: %d", err);
		return;
	}
}

void main(void)
{
	int err;

	LOG_INF("Azure FOTA sample started");
	LOG_INF("This may take a while if a modem firmware update is pending");

	switch (modem_lib_init_result) {
	case MODEM_DFU_RESULT_OK:
		LOG_INF("Modem firmware update successful");
		LOG_INF("Modem will run the new firmware after reboot");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_INF("Modem firmware update failed");
		LOG_INF("Modem will run non-updated firmware on reboot.");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_INF("Modem firmware update failed");
		LOG_INF("Fatal error.");
		break;
	case MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_INF("Modem firmware update failed");
		LOG_INF("Please reboot once you have sufficient power for the DFU.");
		break;
	case -1:
		LOG_INF("Could not initialize modem library");
		LOG_INF("Fatal error.");
		return;
	default:
		break;
	}

	k_work_init_delayable(&reboot_work, reboot_work_fn);

	err = azure_iot_hub_init(azure_event_handler);
	if (err) {
		LOG_ERR("Azure IoT Hub could not be initialized, error: %d",
		       err);
		return;
	}

	LOG_INF("Connecting to LTE network");
	modem_configure();
	k_sem_take(&network_connected_sem, K_FOREVER);

	static struct azure_iot_hub_config config = {
		.use_dps = IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS),
	};

	err = azure_iot_hub_connect(&config);
	if (err < 0) {
		LOG_ERR("azure_iot_hub_connect failed: %d", err);
		return;
	}

	k_sem_take(&cloud_connected_sem, K_FOREVER);

	/* All initializations and cloud connection were successful, now mark
	 * image as working so that we will not revert upon reboot.
	 */
	boot_write_img_confirmed();
}
