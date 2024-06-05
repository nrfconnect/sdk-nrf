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

#include <nrf_rpc_cbor.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/retention/retention.h>

#include <string.h>

#define CRASH_LOG_DEV DEVICE_DT_GET(DT_NODELABEL(crash_log))

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

static bool panic_mode;
static uint32_t log_format = LOG_OUTPUT_TEXT;
static uint8_t current_level;
static uint8_t log_output_buffer[CONFIG_LOG_BACKEND_RPC_BUFFER_SIZE];

#ifdef CONFIG_LOG_BACKEND_RPC_CRASH_LOG
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

static void retain(const uint8_t *data, size_t length)
{
	const struct device *const crash_log = CRASH_LOG_DEV;
	crash_log_header_t header;
	int rc;

	if (crash_log_cur_size >= crash_log_max_size) {
		/* Crash log partition overflow. */
		return;
	}

	length = MIN(length, crash_log_max_size - crash_log_cur_size);

	rc = retention_write(crash_log, sizeof(header) + crash_log_cur_size, data, length);

	if (rc) {
		return;
	}

	crash_log_cur_size += length;
	header.size = crash_log_cur_size;

	(void)retention_write(crash_log, 0, (const uint8_t *)&header, sizeof(header));
}

static void log_rpc_get_crash_log_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct device *const crash_log = CRASH_LOG_DEV;
	size_t offset;
	size_t length;
	bool decoded_ok;
	crash_log_header_t header;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint_decode(ctx->zs, &offset, sizeof(offset));
	decoded_ok = decoded_ok && zcbor_uint_decode(ctx->zs, &length, sizeof(length));
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto err;
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

static void retain(const uint8_t *data, size_t length)
{
	ARG_UNUSED(data);
	ARG_UNUSED(length);
}
#endif /* CONFIG_LOG_BACKEND_RPC_CRASH_LOG */

static void send(const uint8_t *data, size_t length)
{
	struct nrf_rpc_cbor_ctx ctx;
	const struct zcbor_string data_string = {
		.value = data,
		.len = length,
	};

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 4 + length);

	if (!zcbor_uint_encode(ctx.zs, &current_level, sizeof(current_level)) ||
	    !zcbor_bstr_encode(ctx.zs, &data_string)) {
		NRF_RPC_CBOR_DISCARD(&log_rpc_group, ctx);
		return;
	}

	nrf_rpc_cbor_evt_no_err(&log_rpc_group, LOG_RPC_EVT_MSG, &ctx);
}

static int flush(uint8_t *data, size_t length, void *context)
{
	ARG_UNUSED(context);

	if (panic_mode) {
		retain(data, length);
	} else {
		send(data, length);
	}

	/*
	 * Log output does not handle error codes gracefully, so pretend the buffer has been
	 * flushed to move on and not enter an infinite loop.
	 */

	return (int)length;
}

LOG_OUTPUT_DEFINE(log_output_rpc, flush, log_output_buffer, sizeof(log_output_buffer));

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

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	const log_format_func_t log_formatter = log_format_func_t_get(log_format);
	const char *source_name = log_msg_source_name_get(&msg->log);
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

	/*
	 * Panic mode: logs are appended to the buffer in retained RAM, so include LF characters.
	 * Otherwise:  if nRF RPC group is ready, logs are sent in separate messages, so skip LFs.
	 */
	if (panic_mode) {
		flags |= LOG_OUTPUT_FLAG_CRLF_LFONLY;
	} else if (NRF_RPC_GROUP_STATUS(log_rpc_group)) {
		flags |= LOG_OUTPUT_FLAG_CRLF_NONE;
	} else {
		return;
	}

	if (source_name != NULL) {
		for (size_t i = 0; i < ARRAY_SIZE(filtered_out_sources); i++) {
			if (starts_with(source_name, filtered_out_sources[i])) {
				return;
			}
		}
	}

	current_level = log_msg_get_level(&msg->log);
	log_formatter(&log_output_rpc, &msg->log, flags);
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
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_rpc, cnt);
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
