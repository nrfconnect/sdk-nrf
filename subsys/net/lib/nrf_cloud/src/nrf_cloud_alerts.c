/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <date_time.h>
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_alerts.h>

LOG_MODULE_REGISTER(nrf_cloud_alerts, CONFIG_NRF_CLOUD_ALERTS_LOG_LEVEL);

static int alerts_enabled = IS_ENABLED(CONFIG_NRF_CLOUD_ALERTS);

#if defined(CONFIG_NRF_CLOUD_ALERTS)
static atomic_t alert_sequence;

static int alert_prepare(struct nrf_cloud_data *output,
			 enum nrf_cloud_alert_type type,
			 float value,
			 const char *description)
{
	struct nrf_cloud_alert_info alert = {.ts_ms = 0, .type = type, .value = value,
					     .description = description};

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		/* If date_time_now() fails, alert.ts_ms remains unchanged. */
		date_time_now(&alert.ts_ms);
	}

	if (!alert.ts_ms || IS_ENABLED(CONFIG_NRF_CLOUD_ALERTS_SEQ_ALWAYS)) {
		alert.sequence = ++alert_sequence;
	}

	LOG_DBG("Alert! type: %d, value: %f, description: %s, ts: %lld, seq: %d",
		alert.type, alert.value, alert.description ? alert.description : "",
		alert.ts_ms, alert.sequence);

	return nrf_cloud_alert_encode(&alert, output);
}
#endif /* CONFIG_NRF_CLOUD_ALERTS */

#if defined(CONFIG_NRF_CLOUD_MQTT)
int nrf_cloud_alert_send(enum nrf_cloud_alert_type type,
			 float value,
			 const char *description)
{
#if defined(CONFIG_NRF_CLOUD_ALERTS)
	struct nrf_cloud_tx_data output = {
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE
	};
	int err;

	if (!alerts_enabled) {
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
#else
	ARG_UNUSED(type);
	ARG_UNUSED(value);
	ARG_UNUSED(description);

	return 0;
#endif /* CONFIG_NRF_CLOUD_ALERTS */
}
#endif /* CONFIG_NRF_CLOUD_MQTT */

#if defined(CONFIG_NRF_CLOUD_REST)
int nrf_cloud_rest_alert_send(struct nrf_cloud_rest_context *const rest_ctx,
			      const char *const device_id,
			      enum nrf_cloud_alert_type type,
			      float value,
			      const char *description)
{
#if defined(CONFIG_NRF_CLOUD_ALERTS)
	struct nrf_cloud_data data;
	int err;

	if (!alerts_enabled) {
		return 0;
	}
	(void)nrf_cloud_codec_init(NULL);
	err = alert_prepare(&data, type, value, description);
	if (!err) {
		LOG_DBG("Encoded alert: %s", (const char *)data.ptr);
	} else {
		LOG_ERR("Error encoding alert: %d", err);
		return err;
	}

	/* send to d2c topic */
	err = nrf_cloud_rest_send_device_message(rest_ctx, device_id, data.ptr, false, NULL);
	if (!err) {
		LOG_DBG("Send alert via REST");
	} else {
		LOG_ERR("Error sending alert via REST: %d", err);
	}

	if (data.ptr != NULL) {
		cJSON_free((void *)data.ptr);
	}
	return err;
#else
	ARG_UNUSED(rest_ctx);
	ARG_UNUSED(device_id);
	ARG_UNUSED(type);
	ARG_UNUSED(value);
	ARG_UNUSED(description);

	return 0;
#endif /* CONFIG_NRF_CLOUD_ALERTS */
}
#endif /* CONFIG_NRF_CLOUD_REST */

void nrf_cloud_alert_control_set(bool enable)
{
	if (!IS_ENABLED(CONFIG_NRF_CLOUD_ALERTS)) {
		return;
	}
	LOG_DBG("Changing alerts_enabled from:%d to:%d", alerts_enabled, enable);
	if (alerts_enabled != enable) {
		alerts_enabled = enable;
	}
}

bool nrf_cloud_alert_control_get(void)
{
	return alerts_enabled;
}
