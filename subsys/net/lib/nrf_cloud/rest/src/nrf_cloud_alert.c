/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "nrf_cloud_codec_internal.h"
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_alert.h>
#include "nrf_cloud_alert_internal.h"
#include <cJSON.h>

LOG_MODULE_REGISTER(nrf_cloud_alert_rest, CONFIG_NRF_CLOUD_ALERT_LOG_LEVEL);

int nrf_cloud_rest_alert_send(struct nrf_cloud_rest_context *const rest_ctx,
			      const char *const device_id, enum nrf_cloud_alert_type type,
			      float value, const char *description)
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
}
