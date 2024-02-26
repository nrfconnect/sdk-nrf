/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __ZEPHYR_KERNEL_H
#define __ZEPHYR_KERNEL_H

/*
 * Compatibility header for using Zephyr API in TF-M.
 *
 * The macros and functions here can be used by code that is common for both
 * Zephyr and TF-M RTOS.
 *
 * The functionality will be forwarded to TF-M equivalent of the Zephyr API.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/printk.h>

#include <zephyr/kernel_includes.h>
#include <errno.h>
#include <stdbool.h>

#define k_panic() tfm_core_panic()

/*
 * This function is only used by cryptomaster to determine if it can
 * use concurrency primitives. In the TF-M build we want to always use
 * synchronization primitives so we return 0.
 */
static inline bool k_is_pre_kernel(void)
{
	return 0;
}

#define K_FOREVER 0

/*
 * K_MUTEX is only used to enforce single-user access in a
 * multi-threaded environment and TF-M is a single-threaded
 * environment so we can safely compile-out these mutexes.
 */
#define K_MUTEX_DEFINE(name) uint32_t name

static inline int k_mutex_lock(uint32_t *mutex, uint32_t timeout)
{
	return 0;
}

static inline int k_mutex_unlock(uint32_t *mutex)
{
	return 0;
}

struct k_event {
	uint32_t volatile val;
};

/*
 * The K_EVENT API is used by the CRACEN driver to signal from IRQs to
 * threads that certain CRACEN IRQs have triggered.
 */
#define K_EVENT_DEFINE(name) struct k_event name;

static inline void k_event_init(struct k_event *event)
{
	event->val = 0;
}

static inline uint32_t k_event_wait(struct k_event *event, uint32_t events, bool reset,
				    uint32_t timeout)
{
	if (reset) {
		/* reset is not supported yet */
		k_panic();
	}

	if (timeout != K_FOREVER) {
		/* timeout is not supported yet */
		k_panic();
	}

	/* Wait while we have none of the subscribed events */
	while ((event->val & events) == 0) {
		__WFE();
	}

	return event->val & events;
}

static inline void k_event_clear(struct k_event *event, uint32_t events)
{
	event->val &= ~events;
}

static inline uint32_t k_event_set(struct k_event *event, uint32_t events)
{
	uint32_t prev = event->val;

	event->val |= events;

	return prev;
}

#endif /* __ZEPHYR_KERNEL_H */
