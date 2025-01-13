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
	uint32_t wlen;
	union log_msg_generic *copy;
	int len;

	wlen = log_msg_generic_get_wlen(&msg->buf);
	copy = (union log_msg_generic *)mpsc_pbuf_alloc(&log_history_pbuf, wlen, K_NO_WAIT);

	if (!copy) {
		return;
	}

	copy->log.hdr = msg->log.hdr;
	len = cbprintf_package_copy((void *)msg->log.data, msg->log.hdr.desc.package_len,
				    copy->log.data, msg->log.hdr.desc.package_len, 0, NULL, 0);
	__ASSERT_NO_MSG(len == msg->log.hdr.desc.package_len);

	mpsc_pbuf_commit(&log_history_pbuf, &copy->buf);
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
