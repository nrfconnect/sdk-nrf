/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TRANS_RPMSG_H_
#define TRANS_RPMSG_H_

#include <zephyr.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>
#include <openamp/open_amp.h>

#include "rp_ll.h"

/**
 * @file
 * @defgroup rp_transport Remote procedures transport
 * @{
 * @brief Remote procedures transport implementation using rpmsg
 *
 * API is compatible with rp_trans API. For API documentation
 * @see rp_trans_tmpl.h
 */

#ifdef __cplusplus
extern "C" {
#endif


struct rp_trans_endpoint {
	struct rp_ll_endpoint ep;
	/* Union allows reuse of memory taken by fields that are never used in
	 * the same time.
	 */
	union {
		/* Group of fields that is used only for initialization. */
		struct {
			k_thread_stack_t *stack;
			size_t stack_size;
			int prio;
		};
		/* Group of fields that is used only after initialization. */
		struct {
			struct k_sem sem;
			struct k_thread thread;
			struct k_fifo fifo;
			bool running;
		};
	};
};

#define RP_TRANS_ENDPOINT_PREPARE(_name, _stack_size, _prio)                   \
	K_THREAD_STACK_DEFINE(RP_CONCAT(_rp_trans_stack_, _name),              \
			(_stack_size));                                        \
	static const size_t RP_CONCAT(_rp_trans_stack_size_, _name) =          \
			(_stack_size);                                         \
	static const int RP_CONCAT(_rp_trans_prio_, _name) = (_prio)

#define RP_TRANS_ENDPOINT_INITIALIZER(_name)                                   \
	{                                                                      \
		.stack = RP_CONCAT(_rp_trans_stack_, _name),                   \
		.stack_size = RP_CONCAT(_rp_trans_stack_size_, _name),         \
		.prio = RP_CONCAT(_rp_trans_prio_, _name),                     \
	}

typedef void (*rp_trans_receive_handler)(struct rp_trans_endpoint *endpoint,
	const u8_t *buf, size_t length);

int rp_trans_init(rp_trans_receive_handler callback);

void rp_trans_uninit(void);

int rp_trans_endpoint_init(struct rp_trans_endpoint *endpoint,
	int endpoint_number);

void rp_trans_endpoint_uninit(struct rp_trans_endpoint *endpoint);

#define rp_trans_alloc_tx_buf(endpoint, buf, length) \
	ARG_UNUSED(endpoint); \
	u32_t _rp_trans_buf_vla \
		[(*(length) + sizeof(u32_t) - 1) / sizeof(u32_t)]; \
	*(buf) = (u8_t *)(&_rp_trans_buf_vla[0])

#define rp_trans_free_tx_buf(endpoint, buf)

int rp_trans_send(struct rp_trans_endpoint *endpoint, const u8_t *buf,
	size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_RPMSG_H_ */
