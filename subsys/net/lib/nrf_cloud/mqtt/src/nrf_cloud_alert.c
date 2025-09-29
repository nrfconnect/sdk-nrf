/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <date_time.h>
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_transport.h"
#include <net/nrf_cloud_alert.h>
#include "nrf_cloud_alert_internal.h"
#include <cJSON.h>

LOG_MODULE_REGISTER(nrf_cloud_alert_mqtt, CONFIG_NRF_CLOUD_ALERT_LOG_LEVEL);

int nrf_cloud_alert_send(enum nrf_cloud_alert_type type, float value, const char *description)
{
	int err;
	struct nrf_cloud_tx_data output = {.qos = MQTT_QOS_1_AT_LEAST_ONCE,
					   .topic_type = NRF_CLOUD_TOPIC_MESSAGE};

	if (!nrf_cloud_alert_control_get()) {
		return 0;
	}

	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		LOG_ERR("Cannot send alert; not connected");
		return -EACCES;
	}

	err = alert_prepare(&output.data, type, value, description);
	if (!err) {
		LOG_DBG("Encoded alert: %s", (const char *)output.data.ptr);
	} else {
		LOG_ERR("Error encoding alert: %d", err);
		return err;
	}

	err = nrf_cloud_send(&output);
	if (!err) {
		LOG_DBG("Sent alert via MQTT");
	} else {
		LOG_ERR("Error sending alert via MQTT: %d", err);
	}

	if (output.data.ptr != NULL) {
		cJSON_free((void *)output.data.ptr);
	}

	return err;
}
