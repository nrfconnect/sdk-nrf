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
#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_log_internal.h"
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#include <net/nrf_cloud_log.h>

LOG_MODULE_REGISTER(nrf_cloud_log, CONFIG_NRF_CLOUD_LOG_LOG_LEVEL);

static bool enabled;
static int nrf_cloud_log_level = CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL;
static int64_t starting_timestamp;

static atomic_t log_sequence;

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

void nrf_cloud_log_init_context_internal(void *rest_ctx, const char *dev_id, int log_level,
					 uint32_t src_id, const char *src_name, uint8_t dom_id,
					 int64_t ts, struct nrf_cloud_log_context *context)
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
void nrf_cloud_log_inject_internal(int log_level, const char *fmt, va_list ap)
{
	__ASSERT_NO_MSG(fmt != NULL);

	/* Idempotent so is OK to call every time. */
	nrf_cloud_log_init();

	const void *source;

	/* There is no logging API call to retrieve a pointer to the current source.
	 * Instead, access it using the local variable name declared by the
	 * LOG_MODULE_REGISTER macro.
	 */
#if defined(CONFIG_LOG_RUNTIME_FILTERING)
	source = __log_current_dynamic_data;
#else
	source = __log_current_const_data;
#endif

	/* Cloud logging is enabled, so send it through the main logging system. */
	z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, source, log_level, NULL, 0,
				  Z_LOG_MSG_CBPRINTF_FLAGS(0), fmt, ap);
}

#else  /* !CONFIG_NRF_CLOUD_LOG_BACKEND */
int nrf_cloud_log_format_internal(struct nrf_cloud_log_context *context, char *buf,
				  struct nrf_cloud_tx_data *output, void *ctx, const char *dev_id,
				  int log_level, const char *fmt, va_list ap)
{
	__ASSERT_NO_MSG(context != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(fmt != NULL);
	int err;

	/* Idempotent so is OK to call every time. */
	nrf_cloud_log_init();

	nrf_cloud_log_init_context_internal(ctx, dev_id, log_level, 0, "nrf_cloud_log",
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
		LOG_INF("Changing cloud log level from:%d to:%d", nrf_cloud_log_level, level);
		nrf_cloud_log_level = level;
	} else {
		LOG_DBG("No change in log level from %d", level);
	}
}

void nrf_cloud_log_enable(bool enable)
{
	if (enable != enabled) {
#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
		nrf_cloud_log_backend_enable_internal(enable);
#endif
		enabled = enable;
		LOG_INF("Changing cloud logging enabled to:%d", enabled);
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
