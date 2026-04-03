/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "log_backend_rpc_history.h"

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/section_tags.h>

/* DTS nodelabel for optional fixed RAM region (board overlay); change here and in overlay. */
#define LOG_RPC_HISTORY_FIXED_REGION_NODE DT_NODELABEL(log_rpc_history_region)
#define LOG_RPC_HISTORY_FIXED_REGION_DEFINED DT_NODE_EXISTS(LOG_RPC_HISTORY_FIXED_REGION_NODE)

#if LOG_RPC_HISTORY_FIXED_REGION_DEFINED
#define LOG_HISTORY_REGION_NODE LOG_RPC_HISTORY_FIXED_REGION_NODE

/* Buffer bytes: CONFIG_LOG_BACKEND_RPC_HISTORY_SIZE — board overlay region must fit buffer, pbuf,
 * and metadata.
 */
#define HISTORY_WLEN (CONFIG_LOG_BACKEND_RPC_HISTORY_SIZE / sizeof(uint32_t))

#define _LOG_HISTORY_SECTION \
	Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME_TOKEN(LOG_HISTORY_REGION_NODE))

static uint32_t __aligned(Z_LOG_MSG_ALIGNMENT)
log_history_raw[HISTORY_WLEN] _LOG_HISTORY_SECTION;
static struct mpsc_pbuf_buffer log_history_pbuf _LOG_HISTORY_SECTION;
static uint32_t log_retention_warm_magic _LOG_HISTORY_SECTION;
static uint32_t log_retention_pbuf_checksum _LOG_HISTORY_SECTION;
/* Fixed region only: set after mpsc_pbuf valid (cold or warm-resume). Skipped if init aborts. */
static bool log_retention_ready;

#define LOG_RETENTION_WARM_MAGIC 0x6c6f6772U /* "logr" */

static uint32_t calculate_pbuf_checksum(const struct mpsc_pbuf_buffer *pbuf)
{
	size_t len = sizeof(*pbuf);
	const uint8_t *p = (const uint8_t *)pbuf;
	uint32_t sum = 0U;

	/* Use memcpy for each word: direct uint32_t loads can fault on strict UNALIGN_TRP if
	 * struct layout leaves the cursor at a non-4-byte boundary for some offsets.
	 */
	for (size_t i = 0; i + 4 <= len; i += 4) {
		uint32_t w;

		memcpy(&w, p + i, sizeof(w));
		sum += w;
	}
	if (len % 4 != 0) {
		uint32_t last = 0U;

		memcpy(&last, p + (len & ~3U), len % 4);
		sum += last;
	}
	return sum;
}
#else

#define HISTORY_WLEN (CONFIG_LOG_BACKEND_RPC_HISTORY_SIZE / sizeof(uint32_t))

static uint32_t __aligned(Z_LOG_MSG_ALIGNMENT) log_history_raw[HISTORY_WLEN];
static struct mpsc_pbuf_buffer log_history_pbuf;
#endif

void log_rpc_history_init(void)
{
	const struct mpsc_pbuf_buffer_config log_history_config = {
		.buf = log_history_raw,
		.size = ARRAY_SIZE(log_history_raw),
		.get_wlen = log_msg_generic_get_wlen,
		.flags = MPSC_PBUF_MODE_OVERWRITE,
	};

#if LOG_RPC_HISTORY_FIXED_REGION_DEFINED
	if (log_retention_warm_magic == LOG_RETENTION_WARM_MAGIC) {
		/* Self-consistent RAM is not enough after OTA: callbacks and buf must match
		 * this image's symbols/layout, not only the last saved checksum.
		 */
		uint32_t stored = log_retention_pbuf_checksum;
		bool checksum_ok = (stored != 0U &&
				    calculate_pbuf_checksum(&log_history_pbuf) ==
				    stored);
		bool control_ok = (log_history_pbuf.buf == log_history_raw) &&
				  (log_history_pbuf.size ==
				   (uint32_t)ARRAY_SIZE(log_history_raw)) &&
				  (log_history_pbuf.get_wlen == log_msg_generic_get_wlen) &&
				  (log_history_pbuf.notify_drop == NULL);

		if (!checksum_ok || !control_ok) {
			log_retention_warm_magic = 0U;
		} else {
			/* Warm reset: ring indices and buf/callbacks unchanged; re-init lock and
			 * sem only.
			 */
			memset(&log_history_pbuf.lock, 0, sizeof(log_history_pbuf.lock));
			(void)k_sem_init(&log_history_pbuf.sem, 0, 1);
			log_retention_ready = true;
			return;
		}
	}
#endif

#if LOG_RPC_HISTORY_FIXED_REGION_DEFINED
	/* Linker must place pbuf in writable DTS RAM, not flash / outside the region. */
	{
		uintptr_t pbuf_addr = (uintptr_t)&log_history_pbuf;
		uintptr_t region_start = DT_REG_ADDR(LOG_HISTORY_REGION_NODE);
		uintptr_t region_end = region_start + DT_REG_SIZE(LOG_HISTORY_REGION_NODE);

		__ASSERT(pbuf_addr >= region_start && pbuf_addr < region_end,
			 "log_history_pbuf not in log_rpc_history_region (check overlay & map)");
	}
#endif

	mpsc_pbuf_init(&log_history_pbuf, &log_history_config);

#if LOG_RPC_HISTORY_FIXED_REGION_DEFINED
	log_retention_ready = true;
	log_retention_warm_magic = LOG_RETENTION_WARM_MAGIC;
	log_retention_pbuf_checksum = calculate_pbuf_checksum(&log_history_pbuf);
#endif
}

void log_rpc_history_push(const union log_msg_generic *msg)
{
	size_t wlen;
	union mpsc_pbuf_generic *dst;
	uint8_t *dst_data;
	uint8_t *src_data;

	wlen = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg);
	if (wlen * sizeof(uint32_t) <= sizeof(struct mpsc_pbuf_hdr)) {
		return;
	}

	dst = mpsc_pbuf_alloc(&log_history_pbuf, wlen, K_NO_WAIT);
	if (!dst) {
		return;
	}

	/* First word contains internal mpsc packet flags and when copying
	 * those flags must be omitted.
	 */
	dst_data = (uint8_t *)dst + sizeof(struct mpsc_pbuf_hdr);
	src_data = (uint8_t *)msg + sizeof(struct mpsc_pbuf_hdr);

	dst->hdr.data = msg->buf.hdr.data;
	memcpy(dst_data, src_data, wlen * sizeof(uint32_t) - sizeof(struct mpsc_pbuf_hdr));

	mpsc_pbuf_commit(&log_history_pbuf, dst);
}

void log_rpc_history_set_overwriting(bool overwriting)
{
	k_sched_lock();

	if (overwriting) {
		log_history_pbuf.flags |= MPSC_PBUF_MODE_OVERWRITE;
	} else {
		log_history_pbuf.flags &= (~MPSC_PBUF_MODE_OVERWRITE);
	}

	k_sched_unlock();
}

union log_msg_generic *log_rpc_history_pop(void)
{
	return (union log_msg_generic *)mpsc_pbuf_claim(&log_history_pbuf);
}

void log_rpc_history_free(const union log_msg_generic *msg)
{
	if (!msg) {
		return;
	}

	mpsc_pbuf_free(&log_history_pbuf, &msg->buf);
}

uint8_t log_rpc_history_get_usage(void)
{
	size_t total_size;
	size_t current_size;

	mpsc_pbuf_get_utilization(&log_history_pbuf, &total_size, &current_size);

	return current_size * 100 / total_size;
}

size_t log_rpc_history_get_usage_size(void)
{
	size_t total_size;
	size_t current_size;

	mpsc_pbuf_get_utilization(&log_history_pbuf, &total_size, &current_size);

	return current_size;
}

size_t log_rpc_history_get_max_size(void)
{
	return CONFIG_LOG_BACKEND_RPC_HISTORY_SIZE;
}

void log_rpc_history_save_checksum(void)
{
#if LOG_RPC_HISTORY_FIXED_REGION_DEFINED
	if (!log_retention_ready) {
		return;
	}
	log_retention_pbuf_checksum = calculate_pbuf_checksum(&log_history_pbuf);
#endif
}
