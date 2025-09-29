/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <net/nrf_cloud_coap.h>
#include <net/nrf_cloud_alert.h>
#include "nrf_cloud_alert_internal.h"
#include "nrf_cloud_codec_internal.h"
#include <cJSON.h>

LOG_MODULE_REGISTER(nrf_cloud_alert_coap, CONFIG_NRF_CLOUD_ALERT_LOG_LEVEL);

int nrf_cloud_alert_send(enum nrf_cloud_alert_type type, float value, const char *description)
{
	struct nrf_cloud_data data;
	int err;

	if (!nrf_cloud_alert_control_get()) {
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
	err = nrf_cloud_coap_json_message_send(data.ptr, false, true);
	if (!err) {
		LOG_DBG("Send alert via CoAP");
	} else {
		LOG_ERR("Error sending alert via CoAP: %d", err);
	}

	if (data.ptr != NULL) {
		cJSON_free((void *)data.ptr);
	}

	return err;
}
