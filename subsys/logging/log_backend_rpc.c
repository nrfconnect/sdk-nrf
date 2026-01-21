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

#include <zephyr/debug/coredump.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/drivers/coredump.h>
#include <zephyr/random/random.h>

#include <string.h>

LOG_MODULE_REGISTER(log_rpc);

#define USEC_PER_TICK (1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define DUMP_META_VERSION 1
#define ASSERT_FILENAME_SIZE CONFIG_LOG_BACKED_RPC_CRASH_INFO_FILENAME_SIZE
#define DUMP_METADATA_SIZE(meta) \
	sizeof((meta).uuid) + 1 + \
	sizeof((meta).reason) + 1 + \
	sizeof((meta).lr) + 1 + \
	sizeof((meta).pc) + 1 + \
	sizeof((meta).xpsr) + 1 + \
	sizeof((meta).sp) + 1 + \
	sizeof((meta).assert_line) + 1 + \
	((meta).assert_line ? (strlen((meta).assert_filename) + 3) : 1)

#ifdef CONFIG_LOG_BACKEND_RPC_CRASH_LOG
struct arm_arch_block {
	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t r12;
		uint32_t lr;
		uint32_t pc;
		uint32_t xpsr;
		uint32_t sp;

		/* callee registers - optionally collected in V2 */
		uint32_t r4;
		uint32_t r5;
		uint32_t r6;
		uint32_t r7;
		uint32_t r8;
		uint32_t r9;
		uint32_t r10;
		uint32_t r11;
	} r;
} __packed;

struct dump_metadata {
	struct dump_metadata_header {
		uint8_t magic[8];
		uint32_t uuid;
		uint8_t version;
		uint8_t has_assert_info;
	} __packed header;
	uint32_t assert_line;
	char assert_filename[ASSERT_FILENAME_SIZE];
} __packed;

static struct dump_metadata next_dump_info;

static struct coredump_mem_region_node dump_region = {
	.start = (uintptr_t)&next_dump_info,
	.size = sizeof(next_dump_info.header)
};

static const uint8_t CRASH_INFO_MAGIC[] = { 'D', 'U', 'M', 'P', 'I', 'N', 'F', 'O' };
#endif

#ifdef CONFIG_LOG_TIMESTAMP_64BIT
typedef int64_t log_timedelta_t;
#else
typedef int32_t log_timedelta_t;
#endif

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

static int load_coredump_fragment(uint16_t offset, void *buffer, size_t length)
{
	struct coredump_cmd_copy_arg copy_params;

	copy_params.offset = offset;
	copy_params.buffer = (uint8_t *)buffer;
	copy_params.length = length;

	return coredump_cmd(COREDUMP_CMD_COPY_STORED_DUMP, &copy_params);
}

static int handle_arch_hdr(uint16_t *offset, struct nrf_rpc_crash_info *info)
{
	int rc;
	struct coredump_arch_hdr_t arch_hdr;
	struct arm_arch_block arch_blk;

	/* Fill out available registers */
	rc = load_coredump_fragment(*offset, &arch_hdr, sizeof(arch_hdr));
	if (rc < 0) {
		return rc;
	}

	if (arch_hdr.num_bytes != sizeof(arch_blk)) {
		return -EINVAL;
	}

	*offset += sizeof(arch_hdr);

	rc = load_coredump_fragment(*offset, &arch_blk, arch_hdr.num_bytes);
	if (rc < 0) {
		return rc;
	}

	*offset += arch_hdr.num_bytes;

	info->pc = arch_blk.r.pc;
	info->lr = arch_blk.r.lr;
	info->xpsr = arch_blk.r.xpsr;
	info->sp = arch_blk.r.sp;

	return 0;
}

static int handle_meta_hdr(uint16_t *offset, struct nrf_rpc_crash_info *info)
{
	int rc;
	struct coredump_threads_meta_hdr_t thread_meta_hdr;

	/* Intentionally skip over */
	rc = load_coredump_fragment(*offset, &thread_meta_hdr, sizeof(thread_meta_hdr));
	if (rc < 0) {
		return rc;
	}

	*offset += sizeof(thread_meta_hdr) + thread_meta_hdr.num_bytes;

	return 0;
}

static int handle_mem_hdr(uint16_t *offset, struct nrf_rpc_crash_info *info)
{
	int rc;
	int size;
	struct coredump_mem_hdr_t mem_hdr;
	struct dump_metadata metadata;

	rc = load_coredump_fragment(*offset, &mem_hdr, sizeof(mem_hdr));
	if (rc < 0) {
		return rc;
	}

	size = mem_hdr.end - mem_hdr.start;
	*offset += sizeof(mem_hdr);

	if (size >= sizeof(struct dump_metadata_header)) {
		rc = load_coredump_fragment(*offset, &metadata.header,
					    sizeof(struct dump_metadata_header));
		if (rc < 0) {
			return rc;
		}

		*offset += sizeof(struct dump_metadata_header);
		size -= sizeof(struct dump_metadata_header);

		if (memcmp((void *)metadata.header.magic, CRASH_INFO_MAGIC,
		    sizeof(CRASH_INFO_MAGIC)) == 0) {
			info->uuid = metadata.header.uuid;

			if (metadata.header.has_assert_info) {
				if (size + sizeof(struct dump_metadata_header) !=
				    sizeof(metadata)) {
					return -ENOMEM;
				}

				rc = load_coredump_fragment(*offset, &metadata.assert_line, size);
				if (rc < 0) {
					return rc;
				}

				info->assert_line = metadata.assert_line;
				strncpy(info->assert_filename, metadata.assert_filename,
					sizeof(info->assert_filename));
			}
		}
	}

	*offset += size;

	return 0;
}

static int load_crash_info(struct nrf_rpc_crash_info *info)
{
	int rc;
	uint16_t offset = 0;
	uint16_t max_offset;
	struct coredump_hdr_t dump_hdr;
	uint8_t header_id;

	memset(info, 0, sizeof(struct nrf_rpc_crash_info));

	rc = coredump_cmd(COREDUMP_CMD_VERIFY_STORED_DUMP, NULL);
	if (rc == 0) {
		return -ENOENT;
	} else if (rc != 1) {
		return rc;
	}

	max_offset = coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);

	rc = load_coredump_fragment(offset, &dump_hdr, sizeof(dump_hdr));
	if (rc < 0) {
		return rc;
	}

	info->reason = sys_le16_to_cpu(dump_hdr.reason);
	offset = sizeof(dump_hdr);

	while (offset < max_offset) {
		rc = load_coredump_fragment(offset, &header_id, sizeof(uint8_t));
		if (rc < 0) {
			return rc;
		}

		switch (header_id) {
		case COREDUMP_ARCH_HDR_ID:
			rc = handle_arch_hdr(&offset, info);
			break;
		case THREADS_META_HDR_ID:
			rc = handle_meta_hdr(&offset, info);
			break;
		case COREDUMP_MEM_HDR_ID:
			rc = handle_mem_hdr(&offset, info);
			break;
		default:
			return -ENOTSUP;
		}

		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static void log_rpc_get_crash_dump_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	size_t offset;
	size_t length;
	size_t total_length;
	int rc;
	struct coredump_cmd_copy_arg copy_params;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	offset = nrf_rpc_decode_uint(ctx);
	length = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_GET_CRASH_DUMP,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	/* Validate the dump integrity when the first chunk is requested */
	if (offset == 0) {
		rc = coredump_cmd(COREDUMP_CMD_VERIFY_STORED_DUMP, NULL);

		if (rc != 1) {
			goto err;
		}
	}

	rc = coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);

	if (rc <= 0) {
		goto err;
	}

	total_length = (size_t)rc;

	if (offset >= total_length) {
		goto err;
	}

	length = MIN(length, total_length - offset);
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5 + length);

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		goto discard;
	}

	copy_params.offset = offset;
	copy_params.buffer = rsp_ctx.zs[0].payload_mut;
	copy_params.length = length;
	rc = coredump_cmd(COREDUMP_CMD_COPY_STORED_DUMP, &copy_params);

	if (rc <= 0) {
		goto discard;
	}

	rsp_ctx.zs[0].payload_mut += rc;

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

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_get_crash_dump_handler, LOG_RPC_CMD_GET_CRASH_DUMP,
			 log_rpc_get_crash_dump_handler, NULL);

static void log_rpc_invalidate_crash_dump_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int rc;

	nrf_rpc_cbor_decoding_done(group, ctx);

	rc = coredump_cmd(COREDUMP_CMD_INVALIDATE_STORED_DUMP, NULL);

	nrf_rpc_rsp_send_int(group, rc);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_invalidate_crash_dump_handler,
			 LOG_RPC_CMD_INVALIDATE_CRASH_DUMP, log_rpc_invalidate_crash_dump_handler,
			 NULL);


static void log_rpc_crash_info(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	int rc;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	struct nrf_rpc_crash_info info = { 0 };

	nrf_rpc_cbor_decoding_done(group, ctx);

	rc = load_crash_info(&info);
	if (rc) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);

		nrf_rpc_encode_null(&rsp_ctx);
	} else {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, DUMP_METADATA_SIZE(info));

		nrf_rpc_encode_uint(&rsp_ctx, info.uuid);
		nrf_rpc_encode_uint(&rsp_ctx, info.reason);
		nrf_rpc_encode_uint(&rsp_ctx, info.pc);
		nrf_rpc_encode_uint(&rsp_ctx, info.lr);
		nrf_rpc_encode_uint(&rsp_ctx, info.sp);
		nrf_rpc_encode_uint(&rsp_ctx, info.xpsr);
		nrf_rpc_encode_uint(&rsp_ctx, info.assert_line);
		nrf_rpc_encode_str(&rsp_ctx, info.assert_filename, -1);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_crash_info, LOG_RPC_CMD_GET_CRASH_INFO,
			 log_rpc_crash_info, NULL);

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

static bool should_filter_out(struct log_msg *msg)
{
	/*
	 * Drop messages coming from nRF RPC to avoid the log feedback loop:
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

	const char *source_name = log_msg_source_name_get(msg);

	if (source_name != NULL) {
		for (size_t i = 0; i < ARRAY_SIZE(filtered_out_sources); i++) {
			if (starts_with(source_name, filtered_out_sources[i])) {
				return true;
			}
		}
	}

	return false;
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg_generic)
{
	struct log_msg *msg = &msg_generic->log;
	enum log_rpc_level level = (enum log_rpc_level)log_msg_get_level(msg);
	enum log_rpc_level max_level;

	if (panic_mode) {
		return;
	}

	max_level = stream_level;

	if (max_level != LOG_RPC_LEVEL_NONE && level <= max_level && !should_filter_out(msg)) {
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

static void log_rpc_get_history_usage_current_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx,
						  void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_uint(group, (uint32_t)log_rpc_history_get_usage_size());
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_get_history_usage_current_handler,
			 LOG_RPC_CMD_GET_HISTORY_USAGE_SIZE,
			 log_rpc_get_history_usage_current_handler, NULL);

static void log_rpc_get_history_usage_max_handler(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_uint(group, (uint32_t)log_rpc_history_get_max_size());
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_get_history_usage_max_handler,
			 LOG_RPC_CMD_GET_HISTORY_MAX_SIZE,
			 log_rpc_get_history_usage_max_handler, NULL);

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
		(int64_t)(log_timedelta_t)(log_timestamp_delta - old_delta) * USEC_PER_TICK);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_set_time_handler, LOG_RPC_CMD_SET_TIME,
			 log_rpc_set_time_handler, NULL);


#ifdef CONFIG_LOG_BACKEND_RPC_CRASH_LOG

#ifdef CONFIG_ASSERT
#ifndef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(const char *file, unsigned int line)
{
	size_t start = 0;
	size_t len = strlen(file);

	next_dump_info.assert_line = line;
	next_dump_info.header.has_assert_info = true;

	if (len + 1 > sizeof(next_dump_info.assert_filename)) {
		start = len - sizeof(next_dump_info.assert_filename) + 1;
	}

	strncpy(next_dump_info.assert_filename, &file[start],
		sizeof(next_dump_info.assert_filename));

	dump_region.size = sizeof(next_dump_info);

	k_panic();
}
#endif
#endif

static int init_dump_metadata(void)
{
	const struct device *const coredump_dev = DEVICE_DT_GET(DT_NODELABEL(coredump_device));

	memset(&next_dump_info, 0, sizeof(next_dump_info));
	memcpy(next_dump_info.header.magic, CRASH_INFO_MAGIC, sizeof(next_dump_info.header.magic));

	next_dump_info.header.version = DUMP_META_VERSION;
	next_dump_info.header.uuid = sys_rand32_get();

	coredump_device_register_memory(coredump_dev, &dump_region);

	return 0;
}

SYS_INIT(init_dump_metadata, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
