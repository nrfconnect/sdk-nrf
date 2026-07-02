/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Zephyr-side control plane of the app-domain RT framework.
 *
 * These functions implement the non-blocking seq/ack handshake with the RT
 * component: write the parameters, issue a memory barrier, then bump
 * command_seq. The RT ISR observes the change and applies it at a tick
 * boundary, copying command_seq into ack_seq.
 *
 * When the data plane is enabled, this file also owns the EGU interrupt
 * (normal Zephyr priority) that drains the RT -> Zephyr event ring into the
 * system workqueue.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include "rt_comm.h"
#include "rt_shared.h"
#include "rt_framework.h"

#if defined(CONFIG_RT_FW_DATA_PLANE)
#include <zephyr/irq.h>
#include <soc.h>
#include <hal/nrf_egu.h>
#include "rt_platform.h"
#endif

LOG_MODULE_REGISTER(rt_framework, LOG_LEVEL_INF);

/* Default toggle period: 500 ms -> visible blink and easy scope capture. */
#define RT_FW_DEFAULT_PERIOD_US 500000U

/* Implemented in rt_fastpath.c (RT component). */
void rt_fastpath_init(uint32_t initial_period_ticks);
uint32_t rt_fastpath_us_to_ticks(uint32_t us);
uint32_t rt_fastpath_ticks_to_us(uint32_t ticks);

/*
 * Zephyr-private "desired state" shadow. The public API mutates this and then
 * publishes a complete, consistent command snapshot to the RT component. Keeping
 * the desired state here (instead of reading it back from the shared block)
 * means a single API call always publishes a coherent {enable, period} pair.
 *
 * rt_lock serializes the Zephyr-side writers so the seqlock writer sequence and
 * the desired shadow are never corrupted by concurrent callers. The lock does
 * NOT (and cannot) exclude the ZLI. The seqlock alone makes the command
 * publish-safe against the RT ISR.
 */
static K_MUTEX_DEFINE(rt_lock);

static struct {
	uint32_t enable;
	uint32_t period_us;
	uint32_t period_ticks;
} rt_desired = {
	.enable = 0U,
	.period_us = RT_FW_DEFAULT_PERIOD_US,
};

/*
 * Publish rt_desired to the RT component using the seqlock protocol: make
 * command_seq odd (write in progress), write the snapshot, then make it even
 * again. The RT ISR only consumes a command observed with an even command_seq
 * that is stable across the read. Caller must hold rt_lock.
 */
static void rt_publish(void)
{
	uint32_t seq = rt_ctrl.command_seq;

	rt_ctrl.command_seq = seq + 1U; /* odd: write in progress */
	rt_shared_release();

	rt_ctrl.enable = rt_desired.enable;
	rt_ctrl.period_ticks = rt_desired.period_ticks;
	rt_shared_release();

	rt_ctrl.command_seq = seq + 2U; /* even: snapshot published */
}

#if defined(CONFIG_RT_FW_DATA_PLANE)

#define RT_FW_EGU_IRQ_PRIO CONFIG_RT_FW_EGU_IRQ_PRIO

static rt_fw_event_cb_t event_cb;
static struct k_work event_work;

static void event_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	while (rt_evq.tail != rt_evq.head) {
		uint32_t tail = rt_evq.tail;
		struct rt_event evt;

		/* Acquire paired with the producer's release (entry written,
		 * release, then head advanced). Observing the updated head must
		 * happen-before reading the entry contents, otherwise a stale
		 * entry could be read on a weakly-ordered memory system.
		 */
		rt_shared_acquire();

		/* Drop stale cache lines for the entry if it is DMA-/cross-
		 * master-visible. No-op on the default same-core path. Safe
		 * because the entry is written only by the producer.
		 */
		rt_shared_dma_invalidate(&rt_evq.entries[tail], sizeof(rt_evq.entries[tail]));

		evt = rt_evq.entries[tail];

		/* Release: the entry read must complete before the new tail
		 * becomes visible to the producer, otherwise the RT ISR could
		 * overwrite the slot while it is still being read.
		 */
		rt_shared_release();

		rt_evq.tail = (tail + 1U) & (RT_EVENT_QUEUE_SIZE - 1U);

		if (event_cb != NULL) {
			event_cb(evt.type, evt.value, evt.timestamp);
		}
	}
}

static void rt_egu_isr(const void *arg)
{
	ARG_UNUSED(arg);

	nrf_egu_event_clear(RT_EGU, NRF_EGU_EVENT_TRIGGERED0);
	k_work_submit(&event_work);
}

static void rt_dataplane_init(void)
{
	rt_evq.head = 0;
	rt_evq.tail = 0;

	k_work_init(&event_work, event_work_handler);

	nrf_egu_event_clear(RT_EGU, NRF_EGU_EVENT_TRIGGERED0);
	nrf_egu_int_enable(RT_EGU, NRF_EGU_INT_TRIGGERED0);

	IRQ_CONNECT(RT_EGU_IRQN, RT_FW_EGU_IRQ_PRIO, rt_egu_isr, NULL, 0);
	irq_enable(RT_EGU_IRQN);
}

int rt_fw_register_event_cb(rt_fw_event_cb_t cb)
{
	event_cb = cb;
	return 0;
}

#endif /* CONFIG_RT_FW_DATA_PLANE */

int rt_fw_init(void)
{
	rt_desired.enable = 0U;
	rt_desired.period_us = RT_FW_DEFAULT_PERIOD_US;
	rt_desired.period_ticks = rt_fastpath_us_to_ticks(RT_FW_DEFAULT_PERIOD_US);

	rt_ctrl.command_seq = 0;
	rt_ctrl.ack_seq = 0;
	rt_ctrl.enable = 0;
	rt_ctrl.period_ticks = rt_desired.period_ticks;
	rt_ctrl.tick_count = 0;
	rt_ctrl.dropped_events = 0;
	rt_ctrl.last_error = 0;
	rt_shared_release();

#if defined(CONFIG_RT_FW_DATA_PLANE)
	rt_dataplane_init();
#endif

	rt_fastpath_init(rt_desired.period_ticks);

	LOG_INF("RT framework initialized (timer ZLI + RT GPIO, period %u us)",
		rt_desired.period_us);
	return 0;
}

int rt_fw_enable(bool enable)
{
	k_mutex_lock(&rt_lock, K_FOREVER);
	rt_desired.enable = enable ? 1U : 0U;
	rt_publish();
	k_mutex_unlock(&rt_lock);
	return 0;
}

int rt_fw_set_period_us(uint32_t period_us)
{
	if (period_us < RT_FW_MIN_PERIOD_US || period_us > RT_FW_MAX_PERIOD_US) {
		/* Reject out-of-range periods rather than silently clamping: the
		 * period is also the control-plane polling interval, so an
		 * unbounded value would make stop/reconfigure unresponsive.
		 */
		return -EINVAL;
	}

	k_mutex_lock(&rt_lock, K_FOREVER);
	rt_desired.period_us = period_us;
	rt_desired.period_ticks = rt_fastpath_us_to_ticks(period_us);
	rt_publish();
	k_mutex_unlock(&rt_lock);
	return 0;
}

int rt_fw_get_status(struct rt_fw_status *status)
{
	if (status == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&rt_lock, K_FOREVER);

	/* Requested state: what Zephyr last asked for. */
	status->req_period_us = rt_desired.period_us;
	status->req_period_ticks = rt_desired.period_ticks;
	status->req_enabled = (rt_desired.enable != 0U);

	/* Applied state: what the RT path has actually programmed (RT-owned). */
	rt_shared_acquire();
	status->enabled = (rt_ctrl.applied_enable != 0U);
	status->period_ticks = rt_ctrl.applied_period_ticks;
	status->period_us = rt_fastpath_ticks_to_us(rt_ctrl.applied_period_ticks);

	/* A command is still pending until the RT ISR acks the latest seq. */
	status->command_seq = rt_ctrl.command_seq;
	status->ack_seq = rt_ctrl.ack_seq;
	status->pending = (rt_ctrl.command_seq != rt_ctrl.ack_seq);

	status->tick_count = rt_ctrl.tick_count;
	status->dropped_events = rt_ctrl.dropped_events;
	status->last_error = rt_ctrl.last_error;

	k_mutex_unlock(&rt_lock);
	return 0;
}
