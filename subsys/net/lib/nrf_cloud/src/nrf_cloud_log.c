/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_log_internal.h"
#include <net/nrf_cloud_rest.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#include <net/nrf_cloud_log.h>

#define REST_WAIT_MS 1000

LOG_MODULE_REGISTER(nrf_cloud_log, CONFIG_NRF_CLOUD_LOG_LOG_LEVEL);

static bool enabled;
static int nrf_cloud_log_level = CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL;
static int64_t starting_timestamp;

static atomic_t log_sequence;

static K_SEM_DEFINE(ncl_active, 1, 1);

void nrf_cloud_log_init(void)
{
	static bool initialized;
	int err = 0;

	if (initialized) {
		return;
	}

	(void)nrf_cloud_codec_init(NULL);

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		err = date_time_now(&starting_timestamp);
	}
	if (!err) {
		initialized = true;
	}
}

void logs_init_context(void *rest_ctx, const char *dev_id, int log_level,
		       uint32_t src_id, const char *src_name,
		       uint8_t dom_id, int64_t ts,
		       struct nrf_cloud_log_context *context)
{
	context->rest_ctx = rest_ctx;
	context->level = log_level;
	context->src_id = src_id;
	context->src_name = src_name;
	context->dom_id = dom_id;
	context->sequence = atomic_inc(&log_sequence);

	if (dev_id) {
		strncpy(context->device_id, dev_id, NRF_CLOUD_CLIENT_ID_MAX_LEN);
	} else {
		context->device_id[0] = '\0';
	}

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		context->ts = ts + starting_timestamp;
	} else {
		context->ts = 0;
	}
}

#if defined(CONFIG_NRF_CLOUD_LOG_DIRECT)
#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
/* If cloud logging backend is enabled, send it through the main logging system. */
static void log_inject(int log_level, const char *fmt, va_list ap)
{
	__ASSERT_NO_MSG(fmt != NULL);

	/* Idempotent so is OK to call every time. */
	nrf_cloud_log_init();

	/* Cloud logging is enabled, so send it through the main logging system. */
	z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, NULL, log_level,
				  NULL, 0, Z_LOG_MSG_CBPRINTF_FLAGS(0), fmt, ap);
}

#else /* !CONFIG_NRF_CLOUD_LOG_BACKEND */
static int log_format(struct nrf_cloud_log_context *context,
		      char *buf, struct nrf_cloud_tx_data *output,
		      void *ctx, const char *dev_id, int log_level, const char *fmt, va_list ap)
{
	__ASSERT_NO_MSG(context != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(fmt != NULL);
	int err;

	/* Idempotent so is OK to call every time. */
	nrf_cloud_log_init();

	logs_init_context(ctx, dev_id, log_level, 0, "nrf_cloud_log",
			  Z_LOG_LOCAL_DOMAIN_ID, k_uptime_get(), context);

	vsnprintfcb(buf, CONFIG_NRF_CLOUD_LOG_BUF_SIZE - 1, fmt, ap);
	if (strlen(buf) == 0) {
		LOG_WRN("Log msg is empty");
		return -EINVAL;
	}
	err = nrf_cloud_log_json_encode(context, buf, strlen(buf), &output->data);
	if (err) {
		LOG_ERR("Error encoding log:%d", err);
	}
	return err;
}
#endif /* CONFIG_NRF_CLOUD_LOG_BACKEND */

/**
 * @brief These functions provide a simple logging interface as an alternative
 * to using the full logger, but will still work if the full logger is enabled.
 */
#if defined(CONFIG_NRF_CLOUD_MQTT)
int nrf_cloud_log_send(int log_level, const char *fmt, ...)
{
	va_list ap;
	int err;

	if ((log_level > nrf_cloud_log_level) || !enabled) {
		return 0;
	}

#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	/* If cloud logging backend is enabled, send it through the main logging system. */
	va_start(ap, fmt);
	log_inject(log_level, fmt, ap);
	va_end(ap);
	err = 0;
#else
	struct nrf_cloud_log_context context = {0};
	char buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE];
	struct nrf_cloud_tx_data output = {
		.obj = NULL,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE
	};

	/* Send it directly to the cloud. */
	va_start(ap, fmt);
	err = log_format(&context, buf, &output, NULL, NULL, log_level, fmt, ap);
	va_end(ap);
	if (err) {
		return err;
	}
	err = nrf_cloud_send(&output);
	nrf_cloud_free((void *)output.data.ptr);
	if (err) {
		LOG_ERR("Error sending log:%d", err);
	}
#endif /* CONFIG_NRF_CLOUD_LOG_BACKEND */

	return err;
}
#endif /* CONFIG_NRF_CLOUD_MQTT */

#if defined(CONFIG_NRF_CLOUD_REST)
int nrf_cloud_rest_log_send(struct nrf_cloud_rest_context *ctx, const char *dev_id,
			     int log_level, const char *fmt, ...)
{
	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(dev_id != NULL);
	va_list ap;
	int err;

	if ((log_level > nrf_cloud_log_level) || !enabled) {
		return 0;
	}

#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	/* Cloud logging backend is enabled, so send it through the main logging system. */
	va_start(ap, fmt);
	log_inject(log_level, fmt, ap);
	va_end(ap);
	err = 0;
#else
	struct nrf_cloud_log_context context = {0};
	char buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE];
	struct nrf_cloud_tx_data output = {
		.obj = NULL,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE
	};

	/* Send it directly to the cloud. */
	va_start(ap, fmt);
	err = log_format(&context, buf, &output, NULL, NULL, log_level, fmt, ap);
	va_end(ap);
	if (err) {
		return err;
	}

	err = k_sem_take(&ncl_active, K_MSEC(REST_WAIT_MS));
	if (err < 0) {
		LOG_INF("Failed to take semaphore");
		return err; /* Caller should try again; busy or other error */
	}
	err = nrf_cloud_rest_send_device_message(ctx, dev_id,
						 output.data.ptr, false, NULL);
	k_sem_give(&ncl_active);
	nrf_cloud_free((void *)output.data.ptr);

	if (err) {
		LOG_ERR("Error sending log:%d", err);
	}
#endif /* CONFIG_NRF_CLOUD_LOG_BACKEND */

	return err;
}
#endif /* CONFIG_NRF_CLOUD_REST */

#if defined(CONFIG_NRF_CLOUD_COAP)
int nrf_cloud_log_send(int log_level, const char *fmt, ...)
{
	va_list ap;
	int err;

	if ((log_level > nrf_cloud_log_level) || !enabled) {
		return 0;
	}

#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	/* Cloud logging backend is enabled, so send it through the main logging system. */
	va_start(ap, fmt);
	log_inject(log_level, fmt, ap);
	va_end(ap);
	err = 0;
#else
	struct nrf_cloud_log_context context = {0};
	char buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE];
	struct nrf_cloud_tx_data output = {
		.obj = NULL,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE
	};

	/* Send it directly to the cloud. */
	va_start(ap, fmt);
	err = log_format(&context, buf, &output, NULL, NULL, log_level, fmt, ap);
	va_end(ap);
	if (err) {
		return err;
	}

	err = k_sem_take(&ncl_active, K_MSEC(REST_WAIT_MS));
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
#endif /* CONFIG_NRF_CLOUD_COAP */
#endif /* CONFIG_NRF_CLOUD_LOG_DIRECT */

void nrf_cloud_log_control_set(int level)
{
	LOG_DBG("set level to %d", level);
	nrf_cloud_log_level_set(level);
	nrf_cloud_log_enable(level != LOG_LEVEL_NONE);
}

int nrf_cloud_log_control_get(void)
{
	return nrf_cloud_log_level;
}

void nrf_cloud_log_level_set(int level)
{
	if (nrf_cloud_log_level != level) {
		LOG_DBG("Changing log level from:%d to:%d", nrf_cloud_log_level, level);
		nrf_cloud_log_level = level;
	} else {
		LOG_DBG("No change in log level from %d", level);
	}
}

void nrf_cloud_log_enable(bool enable)
{
	if (enable != enabled) {
#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
		logs_backend_enable(enable);
#endif
		enabled = enable;
		LOG_DBG("enabled = %d", enabled);
	}
}

bool nrf_cloud_log_is_enabled(void)
{
	return enabled;
}

bool nrf_cloud_is_text_logging_enabled(void)
{
	return IS_ENABLED(CONFIG_NRF_CLOUD_LOG_TEXT_LOGGING_ENABLED);
}

bool nrf_cloud_is_dict_logging_enabled(void)
{
	return IS_ENABLED(CONFIG_NRF_CLOUD_LOG_DICTIONARY_LOGGING_ENABLED);
}
