/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_OPENTHREAD
#include <zephyr/net/openthread.h>
#endif

/*
 * Pointer to a function that "kicks" a watched thread.
 *
 * Currently, only work queue threads can be watched.
 * "Kicking" means submitting a task to the thread, which marks the thread as alive (not hung).
 */
typedef void (*watched_thread_kick_fn)(struct k_work *work);

static void kick_system_workq(struct k_work *work)
{
	(void)k_work_submit(work);
}

#ifdef CONFIG_OPENTHREAD
static void kick_openthread(struct k_work *work)
{
	k_tid_t tid = openthread_thread_id_get();
	struct k_work_q *queue = CONTAINER_OF(tid, struct k_work_q, thread);

	(void)k_work_submit_to_queue(queue, work);
}
#endif

#ifdef CONFIG_BT_LONG_WQ
extern int bt_long_wq_submit(struct k_work *work);

static void kick_bluetooth_long(struct k_work *work)
{
	(void)bt_long_wq_submit(work);
}
#endif

const static watched_thread_kick_fn watched_thread_kick[] = {
	kick_system_workq,
#ifdef CONFIG_OPENTHREAD
	kick_openthread,
#endif
#ifdef CONFIG_BT_LONG_WQ
	kick_bluetooth_long,
#endif
};

enum {
	NUM_WATCHED_THREADS = ARRAY_SIZE(watched_thread_kick),
};

static atomic_t hung_threads;
static atomic_t hung_thread_candidates;
static void watchdog_timer_handler(struct k_timer *timer);
static K_TIMER_DEFINE(watchdog_timer, watchdog_timer_handler, NULL);
static struct k_work watched_thread_work[NUM_WATCHED_THREADS];

/*
 * Function called by a watched thread to prove it's not hung.
 */
static void watchdog_feed(struct k_work *work)
{
	const size_t index = ARRAY_INDEX(watched_thread_work, work);

	atomic_clear_bit(&hung_thread_candidates, index);
}

static void watchdog_timer_handler(struct k_timer *timer)
{
	const atomic_val_t ALL_THREADS = GENMASK(NUM_WATCHED_THREADS - 1, 0);

	atomic_set(&hung_threads, atomic_set(&hung_thread_candidates, ALL_THREADS));

	for (size_t i = 0; i < NUM_WATCHED_THREADS; i++) {
		watched_thread_kick[i](&watched_thread_work[i]);
	}
}

static int watchdog_init(void)
{
	for (size_t i = 0; i < NUM_WATCHED_THREADS; i++) {
		k_work_init(&watched_thread_work[i], watchdog_feed);
	}

	k_timer_start(&watchdog_timer, K_NO_WAIT, K_SECONDS(CONFIG_NRF_RPC_UTILS_WATCHDOG_PERIOD));

	return 0;
}

SYS_INIT(watchdog_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void system_health_get_handler(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_uint(group, atomic_get(&hung_threads));
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, system_health_get, RPC_UTIL_SYSTEM_HEALTH_GET,
			 system_health_get_handler, NULL);
