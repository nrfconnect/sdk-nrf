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
#include "nrf_cloud_coap_transport.h"
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_coap.h>
#include <net/nrf_cloud_log.h>

LOG_MODULE_DECLARE(nrf_cloud_log, CONFIG_NRF_CLOUD_LOG_LOG_LEVEL);

#define RING_BUF_SIZE CONFIG_NRF_CLOUD_LOG_RING_BUF_SIZE

#define LOG_OUTPUT_RETRIES 5
#define LOG_OUTPUT_RETRY_DELAY_MS 50

/** Special value indicating the source of this log entry could not be determined */
#define UNKNOWN_LOG_SOURCE UINT32_MAX

static void logger_init(const struct log_backend *const backend);
static void logger_process(const struct log_backend *const backend, union log_msg_generic *msg);
static void logger_dropped(const struct log_backend *const backend, uint32_t cnt);
static void logger_panic(const struct log_backend *const backend);
static int logger_is_ready(const struct log_backend *const backend);
static int logger_format_set(const struct log_backend *const backend, uint32_t log_type);
static void logger_notify(const struct log_backend *const backend, enum log_backend_evt event,
		       union log_backend_evt_arg *arg);
static int logger_out(uint8_t *buf, size_t size, void *ctx);

static const struct log_backend_api logger_api = {
	.init		= logger_init,
	.format_set	= logger_format_set,
	.process	= logger_process,
	.panic		= logger_panic,
	.dropped	= logger_dropped,
	.is_ready	= logger_is_ready,
	.notify		= logger_notify
};

static struct nrf_cloud_log_stats {
	/** Total number of lines logged */
	uint32_t lines_rendered;
	/** Total number of bytes (before TLS) logged */
	uint32_t bytes_rendered;
	/** Total number of lines sent */
	uint32_t lines_sent;
	/** Total number of bytes (before TLS) sent */
	uint32_t bytes_sent;
	/** Total number of bytes (before TLS) sent */
	uint32_t lines_dropped;
} stats;

/* Information about a log message is stored in the log_context by the logger_process backend
 * function, then used by the logger_out function when encoding messages for transport.
 */
static struct nrf_cloud_log_context log_context;

static uint8_t log_buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE + 1];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DEFAULT;
static uint32_t log_output_flags = LOG_OUTPUT_FLAG_CRLF_NONE;
static int num_msgs;
static struct nrf_cloud_rest_context *rest_ctx;
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN];

/* Define array of log source names that could generate new log messages as a side effect
 * of sending other log messages to the cloud. We should filter out these log sources
 * to prevent such an unwanted outcome. Otherwise cloud logs never cease.
 */
static const char * const filtered_modules[] = {
#if defined(CONFIG_NRF_CLOUD_MQTT)
	"nrf_cloud_transport",
	"net_mqtt_sock_tls",
	"net_mqtt_enc",
	"net_mqtt_dec",
	"net_mqtt_rx",
	"net_mqtt",
#endif
#if defined(CONFIG_NRF_CLOUD_REST)
	"nrf_cloud_rest",
	"nrf_cloud_jwt",
	"rest_client",
	"net_http",
#endif
#if defined(CONFIG_NRF_CLOUD_COAP)
	"nrf_cloud_coap",
	"nrf_cloud_coap_transport",
	"coap_codec",
	"dtls",
#endif
	"net_tcp",
	"net_ipv4",
	"net_ipv6",
	"nrf_cloud",
	"nrf_cloud_log",
	"nrf_cloud_codec",
	"nrf_cloud_codec_internal"
};

/* Array of logging system source ids corresponding to the above names. */
static int filtered_ids[ARRAY_SIZE(filtered_modules)];

static K_SEM_DEFINE(ncl_active, 1, 1);

BUILD_ASSERT(CONFIG_NRF_CLOUD_LOG_BUF_SIZE < CONFIG_NRF_CLOUD_LOG_RING_BUF_SIZE,
	     "Ring buffer size must be larger than log buffer size");

/* CONFIG_LOG_FMT_SECTION_STRIP, if enabled, causes log strings to be removed from the
 * output image, making the flash used smaller. However, the strings cannot be removed
 * if text logging is enabled either on the UART or via nRF Cloud, you must set
 * CONFIG_LOG_FMT_SECTION_STRIP=n. If they are removed when text logging is enabled,
 * the firmware will crash at runtime.
 */
BUILD_ASSERT(!((IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_TEXT) ||
		IS_ENABLED(CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_TEXT))
		&& IS_ENABLED(CONFIG_LOG_FMT_SECTION_STRIP)),
		"CONFIG_LOG_FMT_SECTION_STRIP is not compatible with text logging.");

LOG_BACKEND_DEFINE(log_nrf_cloud_backend, logger_api, false);
/* Reduce reported log_buf size by 1 so we can null terminate */
LOG_OUTPUT_DEFINE(log_nrf_cloud_output, logger_out, log_buf, (sizeof(log_buf) - 1));
RING_BUF_DECLARE(log_nrf_cloud_rb, RING_BUF_SIZE);

static int send_ring_buffer(void);

static void logger_init(const struct log_backend *const backend)
{
	static bool initialized;
	uint32_t actual_level;
	int i;
	int sid;

	if ((backend != &log_nrf_cloud_backend) || initialized) {
		return;
	}
	if ((CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DEFAULT != LOG_OUTPUT_TEXT) &&
	    !IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		LOG_ERR("Only text mode logs supported with current cloud transport");
		return;
	}
	initialized = true;

	nrf_cloud_log_init();

	LOG_DBG("Filtering lower level log sources");
	for (i = 0; i < ARRAY_SIZE(filtered_modules); i++) {
		sid = log_source_id_get(filtered_modules[i]);
		filtered_ids[i] = sid;
		if (sid < 0) {
			LOG_DBG("%d. log:%s not present", i, filtered_modules[i]);
			continue;
		}
		/* If CONFIG_LOG_RUNTIME_FILTERING is enabled, the call below
		 * will turn off logs from the associated source id for our
		 * backend.  In that case, we will not need to manually filter
		 * each log message in logger_process() below.
		 */
		actual_level = log_filter_set(&log_nrf_cloud_backend,
					      Z_LOG_LOCAL_DOMAIN_ID,
					      sid, 0);
		if (actual_level != 0) {
			LOG_WRN("Unable to filter logs for module %d: %s", i, filtered_modules[i]);
		}
		LOG_DBG("%d. log:%s, srcid:%u, actual_level:%u",
			i, filtered_modules[i], sid, actual_level);
	}

	LOG_DBG("domain name:%s, num domains:%u, num sources:%u",
		log_domain_name_get(Z_LOG_LOCAL_DOMAIN_ID),
		log_domains_count(),
		log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID));
}

void logs_backend_enable(bool enable)
{
	if (enable) {
		logger_init(&log_nrf_cloud_backend);
		log_backend_enable(&log_nrf_cloud_backend, NULL, nrf_cloud_log_control_get());
	} else {
		log_backend_disable(&log_nrf_cloud_backend);
	}
}

void nrf_cloud_log_rest_context_set(struct nrf_cloud_rest_context *ctx, const char *dev_id)
{
#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	rest_ctx = ctx;
	strncpy(device_id, dev_id, NRF_CLOUD_CLIENT_ID_MAX_LEN);
#else
	ARG_UNUSED(ctx);
	ARG_UNUSED(dev_id);
#endif
}

static uint32_t get_source_id(void *source_data, uint8_t dom_id)
{
	if (IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) && (dom_id != Z_LOG_LOCAL_DOMAIN_ID)) {
		/* Remote domain already converted its source pointer to an ID */
		return (uint32_t)(uintptr_t)source_data;
	}
	if (source_data != NULL) {
		return IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
		       log_dynamic_source_id(source_data) :
		       log_const_source_id(source_data);
	}
	return UNKNOWN_LOG_SOURCE;
}

static int log_msg_filter(uint32_t src_id, int level)
{
	if (level > nrf_cloud_log_control_get()) {
		return -ENOMSG;
	}
	if ((level == 0) && !IS_ENABLED(NRF_CLOUD_LOG_INCLUDE_LEVEL_0)) {
		return -ENOMSG;
	}
#if !defined(CONFIG_LOG_RUNTIME_FILTERING)
	/* Prevent locally-generated logs or those from underlying modules
	 * from being self-logged, which results in an infinite series of
	 * log messages. Runtime filtering is used in logger_init() when so configured.
	 */
	for (int i = 0; i < ARRAY_SIZE(filtered_ids); i++) {
		if (filtered_ids[i] < 0) {
			continue;
		}
		if (src_id == filtered_ids[i]) {
			LOG_DBG("Filtered: %s", filtered_modules[i]);
			return -ENOMSG;
		}
	}
#endif
	return 0;
}

static void logger_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	log_format_func_t log_output_func;

	if (!nrf_cloud_log_is_enabled() ||
	    (backend != &log_nrf_cloud_backend) ||
	    (logger_is_ready(&log_nrf_cloud_backend) != 0)) {
		return;
	}

	uint8_t dom_id = log_msg_get_domain(&msg->log);
	uint32_t src_id = get_source_id((void *)log_msg_get_source(&msg->log), dom_id);
	int level = log_msg_get_level(&msg->log);

	if (log_msg_filter(src_id, level)) {
		return;
	}

	const char *src_name = src_id != UNKNOWN_LOG_SOURCE ?
			       log_source_name_get(dom_id, src_id) : NULL;
	int64_t ts = log_output_timestamp_to_us(log_msg_get_timestamp(&msg->log)) / 1000U;

	logs_init_context(rest_ctx, device_id, level, src_id, src_name, dom_id, ts, &log_context);

	log_output_func = log_format_func_t_get(log_format_current);
	log_output_func(&log_nrf_cloud_output, &msg->log, log_output_flags);
}

static void logger_dropped(const struct log_backend *const backend, uint32_t cnt)
{
	if (backend == &log_nrf_cloud_backend) {
		log_output_dropped_process(&log_nrf_cloud_output, cnt);
		stats.lines_dropped += cnt;
	}
}

static void logger_panic(const struct log_backend *const backend)
{
	if (backend == &log_nrf_cloud_backend) {
		log_output_flush(&log_nrf_cloud_output);
	}
}

static int logger_is_ready(const struct log_backend *const backend)
{
	static bool printed;

	ARG_UNUSED(backend);
	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
			return -EBUSY;
		}
	}
	if (IS_ENABLED(CONFIG_NRF_CLOUD_REST)) {
		if (log_context.rest_ctx == NULL) {
			if (rest_ctx) {
				log_context.rest_ctx = rest_ctx;
				LOG_DBG("rest_ctx set");
				printed = false;
			} else {
				if (!printed) {
					printed = true;
					LOG_WRN("rest_ctx is NULL");
				}
				return -EBUSY;
			}
		}
	}
	if (IS_ENABLED(CONFIG_NRF_CLOUD_COAP) && !nrf_cloud_coap_is_connected()) {
		return -EBUSY;
	}
	return 0; /* Logging is ready. */
}

static int logger_format_set(const struct log_backend *const backend, uint32_t log_type)
{
	if (backend != &log_nrf_cloud_backend) {
		return 0;
	}
	if ((log_type == LOG_OUTPUT_TEXT) || (log_type == LOG_OUTPUT_DICT)) {
		log_format_current = log_type;
		return 0;
	}
	return -ENOTSUP;
}

static void logger_notify(const struct log_backend *const backend, enum log_backend_evt event,
		       union log_backend_evt_arg *arg)
{
	if ((backend != &log_nrf_cloud_backend) ||
	    (event != LOG_BACKEND_EVT_PROCESS_THREAD_DONE) ||
	    (logger_is_ready(backend) != 0)) {
		return;
	}

	/* Flush our transmission buffer */
	send_ring_buffer();
	if (CONFIG_NRF_CLOUD_LOG_LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_DBG("Buffered lines:%u, bytes:%u; logged lines:%u, bytes:%u; "
			"sent lines:%u, bytes:%u; dropped lines:%u",
			log_buffered_cnt(), ring_buf_size_get(&log_nrf_cloud_rb),
			stats.lines_rendered, stats.bytes_rendered,
			stats.lines_sent, stats.bytes_sent,
			stats.lines_dropped);
	} else {
		LOG_INF("Sent lines:%u, bytes:%u", stats.lines_sent, stats.bytes_sent);
	}
}

static int send_ring_buffer(void)
{
	int err = 0;
	int ret = 0;
	struct nrf_cloud_tx_data output = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = (log_format_current == LOG_OUTPUT_TEXT) ?
			       NRF_CLOUD_TOPIC_BULK : NRF_CLOUD_TOPIC_BIN
	};
	uint32_t stored;
	uint8_t *base64_buf = NULL;

	/* The bulk topic requires the multiple JSON messages to be placed in
	 * a JSON array. Close the array then send it.
	 */
	if ((num_msgs != 0) && (log_format_current == LOG_OUTPUT_TEXT)) {
		ring_buf_put(&log_nrf_cloud_rb, "]", 1);
	}

	stored = ring_buf_size_get(&log_nrf_cloud_rb);
	output.data.len = ring_buf_get_claim(&log_nrf_cloud_rb,
					     (uint8_t **)&output.data.ptr,
					     stored);
	if (output.data.len != stored) {
		LOG_WRN("Capacity:%u, free:%u, stored:%u, claimed:%u",
			ring_buf_capacity_get(&log_nrf_cloud_rb),
			ring_buf_space_get(&log_nrf_cloud_rb),
			stored, output.data.len);
		stored = output.data.len;
	}
	if (!output.data.len) {
		goto cleanup;
	}

	uint8_t *p = (uint8_t *)output.data.ptr;

	p[output.data.len] = '\0';

	LOG_DBG("Ready to transmit %zd bytes...", output.data.len);
	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		err = nrf_cloud_send(&output);
	} else if (IS_ENABLED(CONFIG_NRF_CLOUD_REST)) {
		do {
			err = nrf_cloud_rest_send_device_message(log_context.rest_ctx,
								 log_context.device_id,
								 output.data.ptr, true, NULL);
			if (err) {
				LOG_ERR("Error sending message:%d", err);
				if (p[0] == '\0') {
					LOG_ERR("Empty buffer!");
				}
				if (err != -EBUSY) {
					LOG_ERR("Data: %s, len: %zd",
						(const char *)output.data.ptr, output.data.len);
				}
				k_sleep(K_MSEC(100));
			}
		} while (err == -EBUSY);
	} else if (IS_ENABLED(CONFIG_NRF_CLOUD_COAP)) {
		err = nrf_cloud_coap_json_message_send(output.data.ptr, true, true);
	} else {
		err = -ENODEV;
	}
	if (!err) {
		stats.lines_sent += num_msgs;
		stats.bytes_sent += output.data.len;
	}

cleanup:
	if (err) {
		LOG_ERR("Error %d ret %d processing ring buffer", err, ret);
	}
	if (base64_buf) {
		k_free(base64_buf);
	}
	ret = ring_buf_get_finish(&log_nrf_cloud_rb, stored);
	ring_buf_reset(&log_nrf_cloud_rb);
	num_msgs = 0;

	if (ret) {
		LOG_ERR("Error finishing ring buffer: %d", ret);
		err = ret;
	}
	return err;
}

static int logger_out(uint8_t *buf, size_t size, void *ctx)
{
	ARG_UNUSED(ctx);
	int err = 0;
	struct nrf_cloud_data data;
	size_t orig_size = size;
	uint32_t stored;
	int extra;
	static int retry_count;

	if (!size) {
		return 0;
	}

	if (k_sem_take(&ncl_active, K_NO_WAIT) < 0) {
		return 0;
	}

	if (log_format_current == LOG_OUTPUT_TEXT) {
		if ((buf >= log_buf) && (&buf[size] <= &log_buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE])) {
			/* Our log_buf has 1 extra byte, and we always dump the whole buffer
			 * at once, so this can be done safely.
			 */
			buf[size] = '\0';
		} else {
			printk("buf %p..%p is not inside our log_buf %p..%p\n",
				buf, &buf[size], log_buf,
				&log_buf[CONFIG_NRF_CLOUD_LOG_BUF_SIZE]);
			return orig_size;
		}

		extra = 3;
		if (log_context.src_name) {
			int len = strlen(log_context.src_name);

			/* We want to pass the log source as a separate field for cloud filtering.
			 * Because the Zephyr logging subsystem prefixes the log text with
			 * 'src_name: ', remove it before encoding the log.
			 */
			if ((strncmp(log_context.src_name, (const char *)buf, MIN(size, len)) == 0)
			    &&
			    ((len + 2) < size) && (buf[len] == ':') && (buf[len + 1] == ' ')) {
				buf += len + 2;
				size -= len + 2;
			}
		}
		err = nrf_cloud_log_json_encode(&log_context, buf, size, &data);
		if (err) {
			LOG_ERR("Error encoding log: %d", err);
			goto end;
		}
	} else {
		extra = sizeof(struct nrf_cloud_bin_hdr);
		data.ptr = buf;
		data.len = size;
	}

	stats.lines_rendered++;
	stats.bytes_rendered += data.len;

	do {
		if (!data.len) {
			LOG_WRN("No data in logger_out()");
			break;
		}
		/* If there is enough room for this rendering, store it and leave. */
		if (ring_buf_space_get(&log_nrf_cloud_rb) > (data.len + extra)) {

			if (num_msgs == 0) {
				/* Insert start of buffer marker */
				if (log_format_current == LOG_OUTPUT_TEXT) {
					/* Open JSON array */
					ring_buf_put(&log_nrf_cloud_rb, "[", 1);
				} else {
					struct nrf_cloud_bin_hdr hdr;

					hdr.magic = NRF_CLOUD_BINARY_MAGIC;
					hdr.format = NRF_CLOUD_DICT_LOG_FMT;
					hdr.ts = log_context.ts;
					hdr.sequence = log_context.sequence;
					ring_buf_put(&log_nrf_cloud_rb,
						     (const uint8_t *)&hdr, sizeof(hdr));
				}
			} else if (log_format_current == LOG_OUTPUT_TEXT) {
				ring_buf_put(&log_nrf_cloud_rb, ",", 1);
			}
			stored = ring_buf_put(&log_nrf_cloud_rb, data.ptr, data.len);
			if (stored != data.len) {
				LOG_WRN("Stored:%u, put:%u", stored, data.len);
			}
			num_msgs++;
			if (log_format_current == LOG_OUTPUT_TEXT) {
				cJSON_free((void *)data.ptr);
			}
			/* Stored rendered output in ring buffer, so we can exit now */
			break;
		}

		/* Low on space, so send everything. */
		if (logger_is_ready(&log_nrf_cloud_backend) == 0) {
			err = send_ring_buffer();
			if (!err) {
				retry_count = 0;
			}
		} else if (retry_count < LOG_OUTPUT_RETRIES) {
			k_sleep(K_MSEC(LOG_OUTPUT_RETRY_DELAY_MS));
			retry_count++;
		} else {
			if (log_format_current == LOG_OUTPUT_TEXT) {
				cJSON_free((void *)data.ptr);
			}
			err = -ETIMEDOUT;
		}
	} while (!err);

	if (err) {
		LOG_ERR("Error sending log: %d", err);
	}
end:
	/* Return original size of log buffer. Otherwise, logger_out will be called
	 * again with the remainder until the full size is sent.
	 */
	k_sem_give(&ncl_active);
	return orig_size;
}
