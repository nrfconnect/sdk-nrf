/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define NRF_RPC_LOG_MODULE NRF_RPC_OS
#include <nrf_rpc_log.h>

#include "nrf_rpc_os.h"
#include <zephyr/sys/math_extras.h>

/* Maximum number of remote thread that this implementation allows. */
#define MAX_REMOTE_THREADS 255

/* Initial value contains ones (context free) on the
 * CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE most significant bits.
 */
#define CONTEXT_MASK_INIT_VALUE						       \
	(~(((atomic_val_t)1 << (8 * sizeof(atomic_val_t) -		       \
				CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE)) - 1))

struct pool_start_msg {
	const uint8_t *data;
	size_t len;
};

static nrf_rpc_os_work_t thread_pool_callback;

static struct pool_start_msg pool_start_msg_buf[2];
static struct k_msgq pool_start_msg;

static struct k_sem context_reserved;
static atomic_t context_mask;

static K_THREAD_STACK_ARRAY_DEFINE(pool_stacks,
	CONFIG_NRF_RPC_THREAD_POOL_SIZE,
	CONFIG_NRF_RPC_THREAD_STACK_SIZE);

static struct k_thread pool_threads[CONFIG_NRF_RPC_THREAD_POOL_SIZE];

BUILD_ASSERT(CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE > 0,
	     "CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE must be greaten than zero");
BUILD_ASSERT(CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE <= 8 * sizeof(atomic_val_t),
	     "CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE too big");
BUILD_ASSERT(sizeof(uint32_t) == sizeof(atomic_val_t),
	     "Only atomic_val_t is implemented that is the same as uint32_t");

static void thread_pool_entry(void *p1, void *p2, void *p3)
{
	struct pool_start_msg msg;

	do {
		k_msgq_get(&pool_start_msg, &msg, K_FOREVER);
		thread_pool_callback(msg.data, msg.len);
	} while (1);
}

int nrf_rpc_os_init(nrf_rpc_os_work_t callback)
{
	int err;
	int i;

	__ASSERT_NO_MSG(callback != NULL);

	thread_pool_callback = callback;

	err = k_sem_init(&context_reserved, CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE,
			 CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE);
	if (err < 0) {
		return err;
	}

	atomic_set(&context_mask, CONTEXT_MASK_INIT_VALUE);

	k_msgq_init(&pool_start_msg, (char *)pool_start_msg_buf,
		    sizeof(struct pool_start_msg),
		    ARRAY_SIZE(pool_start_msg_buf));

	for (i = 0; i < CONFIG_NRF_RPC_THREAD_POOL_SIZE; i++) {
		k_thread_create(&pool_threads[i], pool_stacks[i],
			K_THREAD_STACK_SIZEOF(pool_stacks[i]),
			thread_pool_entry,
			NULL, NULL, NULL,
			CONFIG_NRF_RPC_THREAD_PRIORITY, 0, K_NO_WAIT);
	}

	return 0;
}

void nrf_rpc_os_thread_pool_send(const uint8_t *data, size_t len)
{
	struct pool_start_msg msg;

	msg.data = data;
	msg.len = len;
	k_msgq_put(&pool_start_msg, &msg, K_FOREVER);
}

void nrf_rpc_os_msg_set(struct nrf_rpc_os_msg *msg, const uint8_t *data,
			size_t len)
{
	k_sched_lock();
	msg->data = data;
	msg->len = len;
	k_sem_give(&msg->sem);
	k_sched_unlock();
}

void nrf_rpc_os_msg_get(struct nrf_rpc_os_msg *msg, const uint8_t **data,
			size_t *len)
{
	k_sem_take(&msg->sem, K_FOREVER);
	k_sched_lock();
	*data = msg->data;
	*len = msg->len;
	k_sched_unlock();
}

uint32_t nrf_rpc_os_ctx_pool_reserve(void)
{
	uint32_t number;
	atomic_val_t old_mask;
	atomic_val_t new_mask;

	k_sem_take(&context_reserved, K_FOREVER);

	do {
		old_mask = atomic_get(&context_mask);
		number = u32_count_leading_zeros(old_mask);
		new_mask = old_mask & ~(0x80000000u >> number);
	} while (!atomic_cas(&context_mask, old_mask, new_mask));

	return number;
}

void nrf_rpc_os_ctx_pool_release(uint32_t number)
{
	__ASSERT_NO_MSG(number < CONFIG_NRF_RPC_CMD_CTX_POOL_SIZE);

	atomic_or(&context_mask, 0x80000000u >> number);
	k_sem_give(&context_reserved);
}
