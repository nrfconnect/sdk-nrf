/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "log_backend_rpc_history.h"

#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/util.h>

#define HISTORY_WLEN (CONFIG_LOG_BACKEND_RPC_HISTORY_SIZE / sizeof(uint32_t))

static uint32_t __aligned(Z_LOG_MSG_ALIGNMENT) log_history_raw[HISTORY_WLEN];
static struct mpsc_pbuf_buffer log_history_pbuf;

void log_rpc_history_init(void)
{
	const struct mpsc_pbuf_buffer_config log_history_config = {
		.buf = log_history_raw,
		.size = ARRAY_SIZE(log_history_raw),
		.get_wlen = log_msg_generic_get_wlen,
		.flags = MPSC_PBUF_MODE_OVERWRITE,
	};

	mpsc_pbuf_init(&log_history_pbuf, &log_history_config);
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
