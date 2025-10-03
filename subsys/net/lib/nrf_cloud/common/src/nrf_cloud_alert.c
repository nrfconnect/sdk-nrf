/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <net/nrf_cloud_alert.h>
#include "nrf_cloud_codec_internal.h"
#include <date_time.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_alert_common, CONFIG_NRF_CLOUD_ALERT_LOG_LEVEL);

static bool alerts_enabled = IS_ENABLED(CONFIG_NRF_CLOUD_ALERT);

static atomic_t alert_sequence;

int alert_prepare(struct nrf_cloud_data *output, enum nrf_cloud_alert_type type, float value,
		  const char *description)
{
	struct nrf_cloud_alert_info alert = {
		.ts_ms = 0, .type = type, .value = value, .description = description};

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		/* If date_time_now() fails, alert.ts_ms remains unchanged. */
		date_time_now(&alert.ts_ms);
	}

	if (!alert.ts_ms || IS_ENABLED(CONFIG_NRF_CLOUD_ALERT_SEQ_ALWAYS)) {
		alert.sequence = ++alert_sequence;
	}

	LOG_DBG("Alert! type: %d, value: %f, description: %s, ts: %lld, seq: %d", alert.type,
		(double)alert.value, alert.description ? alert.description : "", alert.ts_ms,
		alert.sequence);

	return nrf_cloud_alert_encode(&alert, output);
}

void nrf_cloud_alert_control_set(bool enable)
{
	LOG_INF("Changing alerts enabled from:%d to:%d", alerts_enabled, enable);
	if (alerts_enabled != enable) {
		alerts_enabled = enable;
	}
}

bool nrf_cloud_alert_control_get(void)
{
	return alerts_enabled;
}
