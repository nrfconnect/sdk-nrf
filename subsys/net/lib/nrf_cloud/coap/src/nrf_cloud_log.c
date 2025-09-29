/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/base64.h>
#include <date_time.h>
#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_log_internal.h"
#include <net/nrf_cloud_coap.h>
#include <net/nrf_cloud_log.h>

LOG_MODULE_REGISTER(nrf_cloud_log_coap, CONFIG_NRF_CLOUD_LOG_LOG_LEVEL);

#define COAP_WAIT_MS 1000

static K_SEM_DEFINE(ncl_active, 1, 1);

int nrf_cloud_log_send(int log_level, const char *fmt, ...)
{
	va_list ap;
	int err;

	if ((log_level > nrf_cloud_log_control_get()) || !nrf_cloud_log_is_enabled()) {
		return 0;
	}

#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	/* nRF Cloud logging backend is enabled, so send it through the main logging system. */
	va_start(ap, fmt);
	nrf_cloud_log_inject_internal(log_level, fmt, ap);
	va_end(ap);
	err = 0;
#else
	struct nrf_cloud_log_context context = {0};
	char buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE];
	struct nrf_cloud_tx_data output = {
		.obj = NULL, .qos = MQTT_QOS_0_AT_MOST_ONCE, .topic_type = NRF_CLOUD_TOPIC_MESSAGE};

	/* Send it directly to the cloud. */
	va_start(ap, fmt);
	err = nrf_cloud_log_format_internal(&context, buf, &output, NULL, NULL, log_level, fmt, ap);
	va_end(ap);
	if (err) {
		return err;
	}

	err = k_sem_take(&ncl_active, K_MSEC(COAP_WAIT_MS));
	if (err < 0) {
		LOG_INF("Failed to take semaphore");
		return err; /* Caller should try again; busy or other error */
	}
	err = nrf_cloud_coap_json_message_send(output.data.ptr, false, true);
	k_sem_give(&ncl_active);
	nrf_cloud_free((void *)output.data.ptr);

	if (err) {
		LOG_ERR("Error sending log:%d", err);
	}
#endif /* CONFIG_NRF_CLOUD_LOG_BACKEND */
	return err;
}
