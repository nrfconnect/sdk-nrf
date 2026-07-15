/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/spsc_lockfree.h>
#include <zephyr/sys/util.h>

#include <rtfw/rtfw.h>

#include "rtfw_internal.h"
#if defined(CONFIG_RTFW_TEST)
#include "rtfw_test.h"
#endif

BUILD_ASSERT(IS_POWER_OF_TWO(CONFIG_RTFW_EVENT_QUEUE_SIZE),
	     "CONFIG_RTFW_EVENT_QUEUE_SIZE must be a power of two");
BUILD_ASSERT(__atomic_always_lock_free(sizeof(uint32_t), NULL),
	     "RTFW requires lock-free 32-bit atomics");
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "RTFW supports one application CPU");
BUILD_ASSERT(RTFW_EVENT_COMMAND_PROCESSED <= RTFW_EVENT_TYPE_FRAMEWORK_MAX);
BUILD_ASSERT(RTFW_EVENT_TYPE_FRAMEWORK_MAX < RTFW_EVENT_TYPE_USER_BASE);

struct status_slot {
	struct rtfw_command attempted;
	struct rtfw_command applied;
	int32_t result;
};

static struct rtfw_command command_slots[2];
static struct status_slot status_slots[2];
static struct rtfw_command last_applied;
static uint32_t published_token;
static uint32_t acknowledged_token;

static struct rtfw_config client;
static bool initialized;
static K_MUTEX_DEFINE(writer_lock);

SPSC_DEFINE(event_queue, struct rtfw_event, CONFIG_RTFW_EVENT_QUEUE_SIZE);
static atomic_t doorbell_pending;
static atomic_t dropped_events;
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
static atomic_t queue_depth;
static atomic_t max_queue_depth;
#endif
static atomic_t faults;

#if defined(CONFIG_RTFW_TEST)
static atomic_t delivery_runs;
static void (*status_copy_hook)(void);
static void (*delivery_release_hook)(void);
static void (*delivery_rearm_hook)(void);
#endif

K_THREAD_STACK_DEFINE(delivery_stack, CONFIG_RTFW_WORKQ_STACK_SIZE);
static struct k_work_q delivery_work_q;
static struct k_work delivery_work;

static int event_enqueue(const struct rtfw_event *event);

static inline uint32_t token_load_acquire(const uint32_t *token)
{
	return __atomic_load_n(token, __ATOMIC_ACQUIRE);
}

static inline void token_store_release(uint32_t *token, uint32_t value)
{
	__atomic_store_n(token, value, __ATOMIC_RELEASE);
}

#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
static void update_max_depth(uint32_t depth)
{
	atomic_val_t old = atomic_get(&max_queue_depth);

	if ((uint32_t)old < depth) {
		atomic_set(&max_queue_depth, (atomic_val_t)depth);
	}
}
#endif

static void delivery_resubmit(void)
{
	(void)k_work_submit_to_queue(&delivery_work_q, &delivery_work);
}

static void delivery_handler(struct k_work *work)
{
	struct rtfw_event event;
	struct rtfw_event *queued;
	rtfw_event_cb_t callback;
	void *user_data;
	uint32_t consumed = 0U;

	ARG_UNUSED(work);
#if defined(CONFIG_RTFW_TEST)
	atomic_inc(&delivery_runs);
#endif
	callback = client.event_handler;
	user_data = client.event_user_data;

	while (consumed < CONFIG_RTFW_DRAIN_BUDGET) {
		queued = spsc_consume(&event_queue);
		if (queued == NULL) {
			break;
		}

		event = *queued;
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
		atomic_dec(&queue_depth);
#endif
#if defined(CONFIG_RTFW_TEST)
		if (delivery_release_hook != NULL) {
			delivery_release_hook();
		}
#endif
		spsc_release(&event_queue);
		consumed++;

		if (callback != NULL) {
			callback(&event, user_data);
		}
	}

	if (spsc_peek(&event_queue) != NULL) {
		delivery_resubmit();
		return;
	}

	/*
	 * Re-arm after observing an empty queue, then close the producer race.
	 * A producer that arrives before the clear sees the old armed state; the
	 * recheck below therefore owns rescheduling in that case.
	 */
#if defined(CONFIG_RTFW_TEST)
	if (delivery_rearm_hook != NULL) {
		delivery_rearm_hook();
	}
#endif
	atomic_set(&doorbell_pending, 0);
	if (spsc_peek(&event_queue) != NULL &&
	    atomic_cas(&doorbell_pending, 0, 1)) {
		delivery_resubmit();
	}
}

int rtfw_init(const struct rtfw_config *config)
{
	if (config == NULL || config->command_handler == NULL ||
	    config->fastpath_handler == NULL ||
	    config->pend_source_irq == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&writer_lock, K_FOREVER);
	if (initialized) {
		k_mutex_unlock(&writer_lock);
		return -EALREADY;
	}

	memset(command_slots, 0, sizeof(command_slots));
	memset(status_slots, 0, sizeof(status_slots));
	memset(&last_applied, 0, sizeof(last_applied));
	published_token = 0U;
	acknowledged_token = 0U;
	spsc_reset(&event_queue);
	atomic_clear(&doorbell_pending);
	atomic_clear(&dropped_events);
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	atomic_clear(&queue_depth);
	atomic_clear(&max_queue_depth);
#endif
	atomic_clear(&faults);
	client = *config;
#if defined(CONFIG_RTFW_TEST)
	atomic_clear(&delivery_runs);
	status_copy_hook = NULL;
	delivery_release_hook = NULL;
	delivery_rearm_hook = NULL;
#endif

	k_work_queue_init(&delivery_work_q);
	k_work_init(&delivery_work, delivery_handler);
	k_work_queue_start(&delivery_work_q, delivery_stack,
			   K_THREAD_STACK_SIZEOF(delivery_stack),
			   CONFIG_RTFW_WORKQ_PRIORITY, NULL);
	k_thread_name_set(k_work_queue_thread_get(&delivery_work_q), "rtfw_delivery");
	rtfw_doorbell_init();

	initialized = true;
	k_mutex_unlock(&writer_lock);
	return 0;
}

int rtfw_submit(const struct rtfw_command *command)
{
	uint32_t token;
	uint32_t slot;

	if (!initialized) {
		return -EACCES;
	}
	if (command == NULL) {
		return -EINVAL;
	}
	if (command->data_len > CONFIG_RTFW_COMMAND_DATA_SIZE) {
		return -EMSGSIZE;
	}
	if (k_is_in_isr()) {
		return -EPERM;
	}

	k_mutex_lock(&writer_lock, K_FOREVER);
	token = token_load_acquire(&published_token) + 1U;
	slot = token & 1U;
	memset(&command_slots[slot], 0, sizeof(command_slots[slot]));
	command_slots[slot].id = command->id;
	command_slots[slot].data_len = command->data_len;
	memcpy(command_slots[slot].data, command->data, command->data_len);
	token_store_release(&published_token, token);
	client.pend_source_irq(client.fastpath_user_data);
	k_mutex_unlock(&writer_lock);

	return 0;
}

int rtfw_get_status(struct rtfw_status *status)
{
	uint32_t published;
	uint32_t acknowledged_before;
	uint32_t acknowledged_after;
	struct status_slot processed;

	if (!initialized) {
		return -EACCES;
	}
	if (status == NULL) {
		return -EINVAL;
	}
	if (k_is_in_isr()) {
		return -EPERM;
	}

	k_mutex_lock(&writer_lock, K_FOREVER);
	published = token_load_acquire(&published_token);
	status->requested = command_slots[published & 1U];

	/*
	 * A pending ZLI can preempt this thread even while writer_lock is held.
	 * Re-read the ACK around the slot copy so a coalesced command that reuses
	 * the same slot cannot produce a mixed applied snapshot.
	 */
	do {
		acknowledged_before = token_load_acquire(&acknowledged_token);
		processed.attempted =
			status_slots[acknowledged_before & 1U].attempted;
#if defined(CONFIG_RTFW_TEST)
		if (status_copy_hook != NULL) {
			status_copy_hook();
		}
#endif
		processed.applied =
			status_slots[acknowledged_before & 1U].applied;
		processed.result =
			status_slots[acknowledged_before & 1U].result;
		acknowledged_after = token_load_acquire(&acknowledged_token);
	} while (acknowledged_before != acknowledged_after);

	status->attempted = processed.attempted;
	status->applied = processed.applied;
	status->apply_result = processed.result;
	status->pending = (published != acknowledged_after);
	status->dropped_events = (uint32_t)atomic_get(&dropped_events);
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	status->max_queue_depth = (uint32_t)atomic_get(&max_queue_depth);
#else
	status->max_queue_depth = 0U;
#endif
	status->faults = (uint32_t)atomic_get(&faults);
	k_mutex_unlock(&writer_lock);

	return 0;
}

void rtfw_fastpath_run(void)
{
	struct rtfw_command command;
	struct rtfw_event event;
	uint32_t published;
	uint32_t acknowledged;
	uint32_t slot;
	int result;

	if (!initialized) {
		return;
	}

	published = token_load_acquire(&published_token);
	acknowledged = token_load_acquire(&acknowledged_token);
	if (published != acknowledged) {
		slot = published & 1U;
		command = command_slots[slot];
		result = client.command_handler(&command,
						client.fastpath_user_data);
		status_slots[slot].attempted = command;
		if (result == 0) {
			last_applied = command;
		} else {
			atomic_or(&faults, RTFW_FAULT_BACKEND_APPLY);
		}
		status_slots[slot].applied = last_applied;
		status_slots[slot].result = result;
		token_store_release(&acknowledged_token, published);

		event.type = RTFW_EVENT_COMMAND_PROCESSED;
		event.value = (uint32_t)result;
		event.timestamp = 0U;
		(void)event_enqueue(&event);
	}

	client.fastpath_handler(client.fastpath_user_data);
}

int rtfw_event_push(const struct rtfw_event *event)
{
	if (event == NULL ||
	    event->type < RTFW_EVENT_TYPE_USER_BASE) {
		return -EINVAL;
	}
	if (!initialized) {
		return -EACCES;
	}

	return event_enqueue(event);
}

static int event_enqueue(const struct rtfw_event *event)
{
	struct rtfw_event *entry;
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	uint32_t depth;
#endif

	entry = spsc_acquire(&event_queue);
	if (entry == NULL) {
		atomic_inc(&dropped_events);
		atomic_or(&faults, RTFW_FAULT_EVENT_OVERFLOW);
		return -ENOSPC;
	}

	*entry = *event;
	spsc_produce(&event_queue);
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	depth = (uint32_t)atomic_inc(&queue_depth) + 1U;
	update_max_depth(depth);
#endif

	if (atomic_cas(&doorbell_pending, 0, 1)) {
		rtfw_doorbell_notify();
	}

	return 0;
}

void rtfw_delivery_signal(void)
{
	if (initialized) {
		delivery_resubmit();
	}
}

#if defined(CONFIG_RTFW_TEST)
void rtfw_test_tokens_set(uint32_t published, uint32_t acknowledged)
{
	token_store_release(&published_token, published);
	token_store_release(&acknowledged_token, acknowledged);
}

void rtfw_test_mailbox_reset(void)
{
	memset(command_slots, 0, sizeof(command_slots));
	memset(status_slots, 0, sizeof(status_slots));
	memset(&last_applied, 0, sizeof(last_applied));
	published_token = 0U;
	acknowledged_token = 0U;
	atomic_clear(&faults);
	status_copy_hook = NULL;
}

void rtfw_test_event_queue_reset(void)
{
	rtfw_test_delivery_drain();
	spsc_reset(&event_queue);
	atomic_clear(&doorbell_pending);
	atomic_clear(&dropped_events);
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	atomic_clear(&queue_depth);
	atomic_clear(&max_queue_depth);
#endif
	atomic_clear(&faults);
	delivery_release_hook = NULL;
	delivery_rearm_hook = NULL;
}

void rtfw_test_delivery_signal(void)
{
	rtfw_delivery_signal();
}

void rtfw_test_delivery_drain(void)
{
	(void)k_work_queue_drain(&delivery_work_q, true);
	k_work_queue_unplug(&delivery_work_q);
}

void rtfw_test_status_copy_hook_set(rtfw_test_hook_t hook)
{
	status_copy_hook = hook;
}

void rtfw_test_delivery_release_hook_set(rtfw_test_hook_t hook)
{
	delivery_release_hook = hook;
}

void rtfw_test_delivery_rearm_hook_set(rtfw_test_hook_t hook)
{
	delivery_rearm_hook = hook;
}

uint32_t rtfw_test_delivery_runs_get(void)
{
	return (uint32_t)atomic_get(&delivery_runs);
}
#endif
