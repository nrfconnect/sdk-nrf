/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <modem/lte_lc.h>
#include <net/aws_iot.h>
#include <power/reboot.h>
#include <date_time.h>
#include <dfu/mcuboot.h>

#define SHADOW_STATE_UPDATE \
"{\"state\":{\"reported\":{\"app_version\":\"%s\"}}}"

static struct k_delayed_work update_routine_work;

K_SEM_DEFINE(lte_connected, 0, 1);

static int update_device_shadow_version(void)
{
	int err;
	char shadow_update_payload[CONFIG_DEVICE_SHADOW_PAYLOAD_SIZE];

	err = snprintf(shadow_update_payload,
		       sizeof(shadow_update_payload),
		       SHADOW_STATE_UPDATE,
		       CONFIG_APP_VERSION);
	u32_t shadow_update_payload_len = err;

	if (err >= sizeof(shadow_update_payload)) {
		return -ENOMEM;
	} else if (err < 0) {
		return err;
	}

	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
		.ptr = shadow_update_payload,
		.len = shadow_update_payload_len
	};

	printk("Publishing: %s to AWS IoT broker\n", shadow_update_payload);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}

	return err;
}

static void update_routine_work_fn(struct k_work *work)
{
	int err;
	char message[50];
	s64_t tx_data_ts;

	err = date_time_now(&tx_data_ts);
	if (err) {
		printk("date_time_now, error: %d\n", err);
		return;
	}

	sprintf(message, "%lld", tx_data_ts);

	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
		.ptr = message,
		.len = sizeof(message)
	};

	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}

	k_delayed_work_submit(&update_routine_work,
			      K_SECONDS(CONFIG_UPDATE_INTERVAL));
}

void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	int err;

	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		printk("AWS_IOT_EVT_CONNECTING\n");
		break;
	case AWS_IOT_EVT_CONNECTED:
		printk("AWS_IOT_EVT_CONNECTED\n");

		/** Successfully connected to AWS IoT broker, mark image as
		 *  working to avoid reverting to the former image upon reboot.
		 */
		boot_write_img_confirmed();

		/** Send version number to AWS IoT broker to verify that the FOTA
		 *  update worked.
		 */
		err = update_device_shadow_version();
		if (err) {
			printk("Updating app version, error: %d\n", err);
		}

		k_delayed_work_submit(&update_routine_work, K_NO_WAIT);
		break;
	case AWS_IOT_EVT_DISCONNECTED:
		printk("AWS_IOT_EVT_DISCONNECTED\n");
		k_delayed_work_cancel(&update_routine_work);
		break;
	case AWS_IOT_EVT_FOTA_DONE:
		printk("AWS_IOT_EVT_FOTA_DONE\n");
		printk("FOTA done, rebooting device");
		sys_reboot(0);
		break;
	case AWS_IOT_EVT_DATA_RECEIVED:
		printk("AWS_IOT_EVT_DATA_RECEIVED\n");
		break;
	default:
		printk("Unknown AWS IoT event type: %d\n", evt->type);
		break;
	}
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_delayed_work_init(&update_routine_work, update_routine_work_fn);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		printk("Network registration status: %s\n",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			printk("Modem could not be configured, error: %d\n",
				err);
			return;
		}
	}
#endif
}

void main(void)
{
	int err;

	printk("AWS IoT simple started, version: %s\n", CONFIG_APP_VERSION);

	err = aws_iot_init(NULL, aws_iot_event_handler);
	if (err) {
		printk("AWS IoT library could not be initialized, error: %d\n",
		       err);
	}

	work_init();
	modem_configure();

	k_sem_take(&lte_connected, K_FOREVER);

	date_time_update();

	/** Sleep to ensure that time has been obtained before
	 *  communication with AWS IoT.
	 */
	k_sleep(K_SECONDS(10));

	err = aws_iot_connect(NULL);
	if (err) {
		printk("aws_iot_connect failed: %d\n", err);
	}
}
