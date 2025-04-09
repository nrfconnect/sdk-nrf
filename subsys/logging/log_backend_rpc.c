/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements Remote Procedure Calls (RPC) for logging subsystem.
 *
 */

#include "log_rpc_group.h"
#include "log_backend_rpc_history.h"

#include <logging/log_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/retention/retention.h>

#include <string.h>

LOG_MODULE_REGISTER(log_rpc);

#define USEC_PER_TICK (1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Drop messages coming from nRF RPC to avoid the avalanche of logs:
 * 1. log added
 * 2. log sent over nRF RPC
 * 3. more logs generated by nRF RPC
 * 4. more logs sent over nRF RPC
 * ...
 */
static const char *const filtered_out_sources[] = {
	"nrf_rpc",
	"NRF_RPC",
};

static const uint32_t common_output_flags =
	LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

static bool panic_mode;
static uint32_t log_format = LOG_OUTPUT_TEXT;
static enum log_rpc_level stream_level = LOG_RPC_LEVEL_NONE;
static log_timestamp_t log_timestamp_delta;

#ifdef CONFIG_LOG_BACKEND_RPC_HISTORY
static enum log_rpc_level history_level = LOG_RPC_LEVEL_NONE;
static uint8_t history_threshold;
static bool history_threshold_active;
static void history_transfer_task(struct k_work *work);
static K_MUTEX_DEFINE(history_transfer_mtx);
static uint32_t history_transfer_id;
static union log_msg_generic *history_cur_msg;
static K_WORK_DEFINE(history_transfer_work, history_transfer_task);
static K_THREAD_STACK_DEFINE(history_transfer_workq_stack,
			     CONFIG_LOG_BACKEND_RPC_HISTORY_UPLOAD_THREAD_STACK_SIZE);
static struct k_work_q history_transfer_workq;
#endif

/*
 * Verify that Zephyr logging level can be used as the nRF RPC logging level without translation.
 */
BUILD_ASSERT(LOG_LEVEL_NONE == LOG_RPC_LEVEL_NONE, "Logging level value mismatch");
BUILD_ASSERT(LOG_LEVEL_ERR == LOG_RPC_LEVEL_ERR, "Logging level value mismatch");
BUILD_ASSERT(LOG_LEVEL_WRN == LOG_RPC_LEVEL_WRN, "Logging level value mismatch");
BUILD_ASSERT(LOG_LEVEL_INF == LOG_RPC_LEVEL_INF, "Logging level value mismatch");
BUILD_ASSERT(LOG_LEVEL_DBG == LOG_RPC_LEVEL_DBG, "Logging level value mismatch");

#ifdef CONFIG_LOG_BACKEND_RPC_CRASH_LOG

#define CRASH_LOG_DEV DEVICE_DT_GET(DT_NODELABEL(crash_log))

/* Header for the crash log data stored in the retained RAM partition. */
typedef struct crash_log_header {
	size_t size;
} crash_log_header_t;

static size_t crash_log_cur_size;
static size_t crash_log_max_size;

static void crash_log_clear(void)
{
	const struct device *const crash_log = CRASH_LOG_DEV;
	ssize_t rc;

	crash_log_cur_size = 0;
	crash_log_max_size = 0;

	rc = retention_clear(crash_log);

	if (rc) {
		return;
	}

	rc = retention_size(crash_log);

	if (rc <= (ssize_t)sizeof(crash_log_header_t)) {
		/* Either failed to obtain the retained partition size, or it is too small. */
		return;
	}

	crash_log_max_size = (size_t)rc - sizeof(crash_log_header_t);
}

static int output_to_retention(uint8_t *data, size_t length, void *output_ctx)
{
	const struct device *const crash_log = CRASH_LOG_DEV;

	crash_log_header_t header;
	size_t saved_length;

	ARG_UNUSED(output_ctx);

	if (crash_log_cur_size >= crash_log_max_size) {
		/* Crash log partition overflow. */
		goto out;
	}

	saved_length = MIN(length, crash_log_max_size - crash_log_cur_size);

	if (retention_write(crash_log, sizeof(header) + crash_log_cur_size, data, saved_length)) {
		goto out;
	}

	crash_log_cur_size += saved_length;
	header.size = crash_log_cur_size;

	(void)retention_write(crash_log, 0, (const uint8_t *)&header, sizeof(header));

out:
	return (int)length;
}

static void log_rpc_get_crash_log_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct device *const crash_log = CRASH_LOG_DEV;
	size_t offset;
	size_t length;
	crash_log_header_t header;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	offset = nrf_rpc_decode_uint(ctx);
	length = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_GET_CRASH_LOG,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	if (retention_is_valid(crash_log) != 1) {
		goto err;
	}

	if (retention_read(crash_log, 0, (uint8_t *)&header, sizeof(header))) {
		goto err;
	}

	if (offset >= header.size) {
		goto err;
	}

	length = MIN(length, header.size - offset);
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5 + length);

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		goto discard;
	}

	if (retention_read(crash_log, sizeof(header) + offset, rsp_ctx.zs[0].payload_mut, length)) {
		goto discard;
	}

	rsp_ctx.zs[0].payload_mut += length;

	if (!zcbor_bstr_end_encode(rsp_ctx.zs, NULL)) {
		goto discard;
	}

	goto send;

discard:
	NRF_RPC_CBOR_DISCARD(group, rsp_ctx);
err:
	/* Send empty string */
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	zcbor_bstr_encode_ptr(rsp_ctx.zs, NULL, 0);
send:
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_get_crash_log_handler, LOG_RPC_CMD_GET_CRASH_LOG,
			 log_rpc_get_crash_log_handler, NULL);
#else
static void crash_log_clear(void)
{
}

static int output_to_retention(uint8_t *data, size_t length, void *output_ctx)
{
	ARG_UNUSED(data);
	ARG_UNUSED(length);
	ARG_UNUSED(output_ctx);

	return (int)length;
}
#endif /* CONFIG_LOG_BACKEND_RPC_CRASH_LOG */

static void format_message(struct log_msg *msg, uint32_t flags, log_output_func_t output_func,
			   void *output_ctx)
{
	uint8_t output_buffer[CONFIG_LOG_BACKEND_RPC_OUTPUT_BUFFER_SIZE];
	struct log_output_control_block control_block = {.ctx = output_ctx};
	struct log_output output = {
		.func = output_func,
		.control_block = &control_block,
		.buf = output_buffer,
		.size = sizeof(output_buffer),
	};
	log_format_func_t log_formatter;

	log_formatter = log_format_func_t_get(log_format);
	log_formatter(&output, msg, flags);
}

struct output_to_buf_ctx {
	uint8_t *out;
	size_t out_len;
	size_t total_len;
};

static int output_to_buf(uint8_t *data, size_t length, void *ctx)
{
	struct output_to_buf_ctx *output_ctx = ctx;
	size_t saved_length;

	if (output_ctx->out != NULL) {
		saved_length = MIN(length, output_ctx->out_len);
		memcpy(output_ctx->out, data, saved_length);
		output_ctx->out += saved_length;
		output_ctx->out_len -= saved_length;
	}

	output_ctx->total_len += length;

	return (int)length;
}

static size_t format_message_to_buf(struct log_msg *msg, uint32_t flags, uint8_t *out,
				    size_t out_len)
{
	struct output_to_buf_ctx output_ctx = {
		.out = out,
		.out_len = out_len,
		.total_len = 0,
	};

	format_message(msg, flags, output_to_buf, &output_ctx);

	return output_ctx.total_len;
}

static void retain_message(struct log_msg *msg)
{
	const uint32_t flags = common_output_flags | LOG_OUTPUT_FLAG_CRLF_LFONLY;

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_RPC_CRASH_LOG)) {
		return;
	}

	format_message(msg, flags, output_to_retention, NULL);
}

static void stream_message(struct log_msg *msg)
{
	const uint32_t flags = common_output_flags | LOG_OUTPUT_FLAG_CRLF_NONE;

	struct nrf_rpc_cbor_ctx ctx;
	size_t length;
	size_t max_length;

	/* 1. Calculate the formatted message length to allocate a sufficient CBOR encode buffer */
	length = format_message_to_buf(msg, flags, NULL, 0);

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 6 + length);
	nrf_rpc_encode_uint(&ctx, log_msg_get_level(msg));

	/* 2. Format the message directly into the CBOR encode buffer. */
	if (zcbor_bstr_start_encode(ctx.zs)) {
		max_length = ctx.zs[0].payload_end - ctx.zs[0].payload_mut;
		length = format_message_to_buf(msg, flags, ctx.zs[0].payload_mut, max_length);
		ctx.zs[0].payload_mut += MIN(length, max_length);
		zcbor_bstr_end_encode(ctx.zs, NULL);
	}

	nrf_rpc_cbor_evt_no_err(&log_rpc_group, LOG_RPC_EVT_MSG, &ctx);
}

static const char *log_msg_source_name_get(struct log_msg *msg)
{
	void *source;
	uint32_t source_id;

	if (log_msg_get_domain(msg) != Z_LOG_LOCAL_DOMAIN_ID) {
		return NULL;
	}

	source = (void *)log_msg_get_source(msg);

	if (source == NULL) {
		return NULL;
	}

	source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? log_dynamic_source_id(source)
							     : log_const_source_id(source);

	return TYPE_SECTION_START(log_const)[source_id].name;
}

static bool starts_with(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg_generic)
{
	struct log_msg *msg = &msg_generic->log;
	const char *source_name = log_msg_source_name_get(msg);
	enum log_rpc_level level = (enum log_rpc_level)log_msg_get_level(msg);
	enum log_rpc_level max_level;

	if (source_name != NULL) {
		for (size_t i = 0; i < ARRAY_SIZE(filtered_out_sources); i++) {
			if (starts_with(source_name, filtered_out_sources[i])) {
				return;
			}
		}
	}

	if (panic_mode) {
		retain_message(msg);
		return;
	}

	max_level = stream_level;

	if (max_level != LOG_RPC_LEVEL_NONE && level <= max_level) {
		/*
		 * The "max_level != LOG_RPC_LEVEL_NONE" condition seems redundant but is in fact
		 * needed, because a log message can be generated with the level NONE, and such
		 * a message should also be discarded if the configured maximum level is NONE.
		 */
		stream_message(msg);
	}

#ifdef CONFIG_LOG_BACKEND_RPC_HISTORY
	max_level = history_level;

	if (max_level != LOG_RPC_LEVEL_NONE && level <= max_level) {
		log_rpc_history_push(msg_generic);

		if (history_threshold_active && log_rpc_history_get_usage() > history_threshold) {
			struct nrf_rpc_cbor_ctx ctx;

			NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 0);
			nrf_rpc_cbor_evt_no_err(&log_rpc_group,
						LOG_RPC_EVT_HISTORY_THRESHOLD_REACHED, &ctx);
			history_threshold_active = false;
		}
	}
#endif
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	crash_log_clear();
	panic_mode = true;
}

static void init(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

#ifdef CONFIG_LOG_BACKEND_RPC_HISTORY
	log_rpc_history_init();
	k_work_queue_init(&history_transfer_workq);
	k_work_queue_start(&history_transfer_workq, history_transfer_workq_stack,
			   K_THREAD_STACK_SIZEOF(history_transfer_workq_stack),
			   K_LOWEST_APPLICATION_THREAD_PRIO, NULL);
#endif
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(cnt);

	/*
	 * Due to an issue in Zephyr logging subsystem (see upstream PR 78145), this function
	 * may be called more often than the configured periodicity (1000ms by default).
	 * This might cause sending excessively many RPC events to the RPC client, which would
	 * waste the RCP transport's bandwidth.
	 *
	 * For this reason, do not report the dropped log messages until this issue is fixed.
	 */
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);

	log_format = log_type;

	return 0;
}

static const struct log_backend_api log_backend_rpc_api = {
	.process = process,
	.panic = panic,
	.init = init,
	.dropped = dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_rpc, log_backend_rpc_api, true);

static void log_rpc_set_stream_level_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	enum log_rpc_level level;

	level = (enum log_rpc_level)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_SET_STREAM_LEVEL,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	stream_level = level;

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_set_stream_level_handler,
			 LOG_RPC_CMD_SET_STREAM_LEVEL, log_rpc_set_stream_level_handler, NULL);

#ifdef CONFIG_LOG_BACKEND_RPC_HISTORY

static void log_rpc_set_history_level_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	enum log_rpc_level level;

	level = (enum log_rpc_level)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_SET_HISTORY_LEVEL,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	history_level = level;

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_set_history_level_handler,
			 LOG_RPC_CMD_SET_HISTORY_LEVEL, log_rpc_set_history_level_handler, NULL);

static void history_transfer_task(struct k_work *work)
{
	const uint32_t flags = common_output_flags | LOG_OUTPUT_FLAG_CRLF_NONE;

	struct nrf_rpc_cbor_ctx ctx;
	bool any_msg_consumed = false;
	struct log_msg *msg;
	size_t length;
	size_t max_length;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, CONFIG_LOG_BACKEND_RPC_HISTORY_UPLOAD_CHUNK_SIZE);

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);

	if (history_transfer_id == 0) {
		/* History transfer has been stopped */
		k_mutex_unlock(&history_transfer_mtx);
		return;
	}

	nrf_rpc_encode_uint(&ctx, history_transfer_id);

	while (true) {
		if (!history_cur_msg) {
			history_cur_msg = log_rpc_history_pop();
		}

		if (!history_cur_msg) {
			break;
		}

		msg = &history_cur_msg->log;
		length = 6 + format_message_to_buf(msg, flags, NULL, 0);
		max_length = ctx.zs[0].payload_end - ctx.zs[0].payload_mut;

		/* Check if there is enough buffer space to fit in the current message. */
		if (length > max_length) {
			break;
		}

		nrf_rpc_encode_uint(&ctx, log_msg_get_level(msg));

		if (zcbor_bstr_start_encode(ctx.zs)) {
			max_length = ctx.zs[0].payload_end - ctx.zs[0].payload_mut;
			length = format_message_to_buf(msg, flags, ctx.zs[0].payload_mut,
						       max_length);
			ctx.zs[0].payload_mut += MIN(length, max_length);
			zcbor_bstr_end_encode(ctx.zs, NULL);
		}

		log_rpc_history_free(history_cur_msg);
		history_cur_msg = NULL;
		any_msg_consumed = true;
	}

	/* Determine if the work should be resubmitted to continue the transfer. */
	if (any_msg_consumed) {
		k_work_submit_to_queue(&history_transfer_workq, work);
	} else {
		log_rpc_history_free(history_cur_msg);
		history_cur_msg = NULL;
		log_rpc_history_set_overwriting(true);
		history_threshold_active = history_threshold > 0;
	}

	k_mutex_unlock(&history_transfer_mtx);

	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_PUT_HISTORY_CHUNK, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void log_rpc_fetch_history_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t transfer_id;

	transfer_id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_FETCH_HISTORY,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);
	history_transfer_id = transfer_id;
	log_rpc_history_set_overwriting(false);
	k_work_submit_to_queue(&history_transfer_workq, &history_transfer_work);
	k_mutex_unlock(&history_transfer_mtx);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_fetch_history_handler, LOG_RPC_CMD_FETCH_HISTORY,
			 log_rpc_fetch_history_handler, NULL);

static void log_rpc_stop_fetch_history_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool pause;

	pause = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_STOP_FETCH_HISTORY,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);
	history_transfer_id = 0;
	log_rpc_history_set_overwriting(!pause);
	history_threshold_active = history_threshold > 0;
	k_mutex_unlock(&history_transfer_mtx);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_stop_fetch_history_handler,
			 LOG_RPC_CMD_STOP_FETCH_HISTORY, log_rpc_stop_fetch_history_handler, NULL);

static void log_rpc_get_history_usage_threshold_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_uint(group, history_threshold);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_get_history_usage_threshold_handler,
			 LOG_RPC_CMD_GET_HISTORY_USAGE_THRESHOLD,
			 log_rpc_get_history_usage_threshold_handler, NULL);

static void log_rpc_set_history_usage_threshold_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	uint8_t threshold = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group,
			    LOG_RPC_CMD_SET_HISTORY_USAGE_THRESHOLD, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	history_threshold = threshold;
	history_threshold_active = threshold > 0;

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_set_history_usage_threshold_handler,
			 LOG_RPC_CMD_SET_HISTORY_USAGE_THRESHOLD,
			 log_rpc_set_history_usage_threshold_handler, NULL);

#endif

#ifdef CONFIG_LOG_BACKEND_RPC_ECHO

static void put_log(enum log_rpc_level level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_generic(level, fmt, ap);
	va_end(ap);
}

static void log_rpc_echo_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	enum log_rpc_level level;
	const char *log_msg;
	size_t log_len;

	level = nrf_rpc_decode_uint(ctx);
	log_msg = nrf_rpc_decode_str_ptr_and_len(ctx, &log_len);

	if (log_msg) {
		put_log(level, "%.*s", log_len, log_msg);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_ECHO,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_echo_handler, LOG_RPC_CMD_ECHO,
			 log_rpc_echo_handler, NULL);

#endif

static log_timestamp_t log_rpc_timestamp(void)
{
	return sys_clock_tick_get() + log_timestamp_delta;
}

static void log_rpc_set_time_handler(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	log_timestamp_t log_time;
	log_timestamp_t local_time;
	log_timestamp_t old_delta;

	log_time = (log_timestamp_t)k_us_to_ticks_near64(nrf_rpc_decode_uint64(ctx));
	local_time = (log_timestamp_t)sys_clock_tick_get();

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_SET_TIME,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	k_sched_lock();
	old_delta = log_timestamp_delta;
	log_timestamp_delta = log_time - local_time;
	log_set_timestamp_func(log_rpc_timestamp, CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	k_sched_unlock();

	LOG_WRN("Log time shifted by %" PRId64 " us",
		((int64_t)log_timestamp_delta - (int64_t)old_delta) * USEC_PER_TICK);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_set_time_handler, LOG_RPC_CMD_SET_TIME,
			 log_rpc_set_time_handler, NULL);
