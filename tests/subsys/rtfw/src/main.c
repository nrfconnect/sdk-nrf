/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <rtfw/rtfw.h>
#include "rtfw_test.h"

#define TEST_EVENT_TYPE RTFW_EVENT_TYPE_USER_BASE

static struct rtfw_command last_handled;
static struct rtfw_command fake_applied;
static uint32_t apply_count;
static uint32_t fastpath_count;
static uint32_t kick_count;
static uint32_t delivered_count;
static struct rtfw_event last_delivered;
static uint32_t delivered_types[CONFIG_RTFW_EVENT_QUEUE_SIZE];
#if !defined(CONFIG_RTFW_TEST_PREINIT) && \
	!defined(CONFIG_RTFW_TEST_NO_EVENT_HANDLER)
static int rearm_push_result;
static int release_push_result;
static bool push_client_event;
static int fastpath_push_result;
static int isr_submit_result;
static int isr_status_result;
#endif
static K_SEM_DEFINE(delivered, 0, CONFIG_RTFW_EVENT_QUEUE_SIZE);

static int fake_command_handler(const struct rtfw_command *command,
				void *user_data)
{
	ARG_UNUSED(user_data);
	last_handled = *command;
	apply_count++;
	if (command->id == UINT32_MAX) {
		return -EINVAL;
	}

	fake_applied = *command;
	return 0;
}

static void fake_fastpath_handler(void *user_data)
{
	const struct rtfw_event event = {
		.type = TEST_EVENT_TYPE,
	};

	ARG_UNUSED(user_data);
	fastpath_count++;
#if !defined(CONFIG_RTFW_TEST_PREINIT) && \
	!defined(CONFIG_RTFW_TEST_NO_EVENT_HANDLER)
	if (push_client_event) {
		fastpath_push_result = rtfw_event_push(&event);
	}
#else
	(void)event;
#endif
}

static void fake_pend_source_irq(void *user_data)
{
	ARG_UNUSED(user_data);
	kick_count++;
}

static void event_callback(const struct rtfw_event *event, void *user_data)
{
	ARG_UNUSED(user_data);
	last_delivered = *event;
	if (delivered_count < ARRAY_SIZE(delivered_types)) {
		delivered_types[delivered_count] = event->type;
	}
	delivered_count++;
	k_sem_give(&delivered);
}

static const struct rtfw_config fake_config = {
	.command_handler = fake_command_handler,
	.fastpath_handler = fake_fastpath_handler,
	.pend_source_irq = fake_pend_source_irq,
	.event_handler = event_callback,
};

#if !defined(CONFIG_RTFW_TEST_NO_EVENT_HANDLER)
static struct rtfw_command command_create(uint32_t id, uint32_t value)
{
	struct rtfw_command command = {
		.id = id,
		.data_len = sizeof(value),
	};

	memcpy(command.data, &value, sizeof(value));
	return command;
}
#endif

#if !defined(CONFIG_RTFW_TEST_PREINIT) && \
	!defined(CONFIG_RTFW_TEST_NO_EVENT_HANDLER)
static uint32_t command_value(const struct rtfw_command *command)
{
	uint32_t value;

	memcpy(&value, command->data, sizeof(value));
	return value;
}
#endif

#if defined(CONFIG_RTFW_TEST_PREINIT)

ZTEST(rtfw_preinit, test_api_rejects_calls_before_initialization)
{
	struct rtfw_command command = command_create(1U, 2U);
	struct rtfw_event event = {.type = TEST_EVENT_TYPE};
	struct rtfw_status status;
	struct rtfw_config invalid = fake_config;

	zassert_equal(rtfw_submit(&command), -EINVAL);
	zassert_equal(rtfw_get_status(&status), -EINVAL);
	zassert_equal(rtfw_event_push(&event), -EACCES);
	rtfw_fastpath_run();
	zassert_equal(fastpath_count, 0U);

	zassert_equal(rtfw_init(NULL), -EINVAL);
	invalid.command_handler = NULL;
	zassert_equal(rtfw_init(&invalid), -EINVAL);
	invalid = fake_config;
	invalid.fastpath_handler = NULL;
	zassert_equal(rtfw_init(&invalid), -EINVAL);
	invalid = fake_config;
	invalid.pend_source_irq = NULL;
	zassert_equal(rtfw_init(&invalid), -EINVAL);
}

ZTEST_SUITE(rtfw_preinit, NULL, NULL, NULL, NULL, NULL);

#elif defined(CONFIG_RTFW_TEST_NO_EVENT_HANDLER)

static void *suite_setup(void)
{
	struct rtfw_config config = fake_config;

	config.event_handler = NULL;
	zassert_ok(rtfw_init(&config));
	return NULL;
}

ZTEST(rtfw_no_handler, test_delivery_discards_events_without_handler)
{
	const struct rtfw_event event = {
		.type = TEST_EVENT_TYPE,
	};

	for (uint32_t i = 0U; i < CONFIG_RTFW_EVENT_QUEUE_SIZE; i++) {
		zassert_ok(rtfw_event_push(&event));
	}
	rtfw_test_delivery_signal();
	rtfw_test_delivery_drain();

	for (uint32_t i = 0U; i < CONFIG_RTFW_EVENT_QUEUE_SIZE; i++) {
		zassert_ok(rtfw_event_push(&event));
	}
}

ZTEST_SUITE(rtfw_no_handler, NULL, suite_setup, NULL, NULL, NULL);

#else

static void thread_only_apis_from_isr(const void *unused)
{
	struct rtfw_command command = command_create(4U, 5U);
	struct rtfw_status status;

	ARG_UNUSED(unused);
	isr_submit_result = rtfw_submit(&command);
	isr_status_result = rtfw_get_status(&status);
}

static void *suite_setup(void)
{
	zassert_ok(rtfw_init(&fake_config));
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	rtfw_test_mailbox_reset();
	rtfw_test_event_queue_reset();
	rtfw_test_doorbell_reset();
	memset(&last_handled, 0, sizeof(last_handled));
	memset(&fake_applied, 0, sizeof(fake_applied));
	memset(&last_delivered, 0, sizeof(last_delivered));
	memset(delivered_types, 0, sizeof(delivered_types));
	apply_count = 0U;
	fastpath_count = 0U;
	kick_count = 0U;
	delivered_count = 0U;
	rearm_push_result = -EINPROGRESS;
	release_push_result = -EINPROGRESS;
	push_client_event = false;
	fastpath_push_result = -EINPROGRESS;
	isr_submit_result = -EINPROGRESS;
	isr_status_result = -EINPROGRESS;
	while (k_sem_take(&delivered, K_NO_WAIT) == 0) {
	}
}

ZTEST(rtfw, test_public_command_contract)
{
	struct rtfw_command zero = {.id = 1U};
	struct rtfw_command maximum = {.id = 2U};
	struct rtfw_command too_large = {.id = 3U};
	struct rtfw_event reserved_event = {
		.type = RTFW_EVENT_COMMAND_PROCESSED,
	};
	struct rtfw_status status;

	maximum.data_len = CONFIG_RTFW_COMMAND_DATA_SIZE;
	memset(maximum.data, 0xa5, sizeof(maximum.data));
	too_large.data_len = CONFIG_RTFW_COMMAND_DATA_SIZE + 1U;

	zassert_equal(rtfw_init(&fake_config), -EALREADY);
	zassert_equal(rtfw_submit(NULL), -EINVAL);
	zassert_equal(rtfw_get_status(NULL), -EINVAL);
	zassert_equal(rtfw_event_push(NULL), -EINVAL);
	zassert_equal(rtfw_event_push(&reserved_event), -EINVAL);
	zassert_ok(rtfw_submit(&zero));
	zassert_ok(rtfw_submit(&maximum));
	zassert_equal(rtfw_submit(&too_large), -EINVAL);
	zassert_equal(kick_count, 2U);

	rtfw_fastpath_run();
	zassert_ok(rtfw_get_status(&status));
	zassert_equal(status.attempted.id, maximum.id);
	zassert_equal(status.attempted.data_len,
		      CONFIG_RTFW_COMMAND_DATA_SIZE);
	zassert_mem_equal(status.attempted.data, maximum.data,
			  CONFIG_RTFW_COMMAND_DATA_SIZE);
}

ZTEST(rtfw, test_thread_only_apis_reject_isr_context)
{
	irq_offload(thread_only_apis_from_isr, NULL);

	zassert_equal(isr_submit_result, -EPERM);
	zassert_equal(isr_status_result, -EPERM);
	zassert_equal(kick_count, 0U);
}

ZTEST(rtfw, test_latest_wins_and_wrap)
{
	struct rtfw_command first = command_create(10U, 100U);
	struct rtfw_command latest = command_create(11U, 200U);
	struct rtfw_command wrapped = command_create(12U, 300U);
	struct rtfw_status status;

	/* Latest-wins publication coalesces two kicks into one backend apply. */
	zassert_ok(rtfw_submit(&first));
	zassert_ok(rtfw_submit(&latest));
	zassert_equal(kick_count, 2U);
	zassert_ok(rtfw_get_status(&status));
	zassert_true(status.pending);
	zassert_equal(status.requested.id, latest.id);
	zassert_equal(command_value(&status.requested), 200U);

	rtfw_fastpath_run();
	zassert_equal(apply_count, 1U);
	zassert_equal(last_handled.id, latest.id);
	zassert_equal(fastpath_count, 1U);
	rtfw_fastpath_run();
	zassert_equal(apply_count, 1U);
	zassert_equal(fastpath_count, 2U);
	zassert_ok(rtfw_get_status(&status));
	zassert_false(status.pending);
	zassert_equal(status.attempted.id, latest.id);
	zassert_equal(status.applied.id, latest.id);
	zassert_equal(command_value(&status.applied), 200U);

	/* UINT32_MAX -> 0 remains a real pending token transition. */
	rtfw_test_event_queue_reset();
	rtfw_test_tokens_set(UINT32_MAX, UINT32_MAX);
	zassert_ok(rtfw_submit(&wrapped));
	zassert_ok(rtfw_get_status(&status));
	zassert_true(status.pending);
	rtfw_fastpath_run();
	zassert_ok(rtfw_get_status(&status));
	zassert_false(status.pending);
	zassert_equal(status.applied.id, wrapped.id);
}

ZTEST(rtfw, test_failed_apply_preserves_applied_state)
{
	struct rtfw_command valid = command_create(20U, 400U);
	struct rtfw_command rejected = command_create(UINT32_MAX, 500U);
	struct rtfw_status status;

	zassert_ok(rtfw_submit(&valid));
	rtfw_fastpath_run();
	zassert_equal(fake_applied.id, valid.id);
	rtfw_test_event_queue_reset();
	rtfw_test_doorbell_reset();

	zassert_ok(rtfw_submit(&rejected));
	rtfw_fastpath_run();
	zassert_ok(rtfw_get_status(&status));
	zassert_false(status.pending);
	zassert_equal(status.requested.id, rejected.id);
	zassert_equal(status.attempted.id, rejected.id);
	zassert_equal(status.applied.id, valid.id);
	zassert_equal(status.apply_result, -EINVAL);
	zassert_true((status.faults & RTFW_FAULT_BACKEND_APPLY) != 0U);
	zassert_equal(fake_applied.id, valid.id);

	rtfw_test_delivery_signal();
	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	rtfw_test_delivery_drain();
	zassert_equal(last_delivered.type, RTFW_EVENT_COMMAND_PROCESSED);
	zassert_equal(last_delivered.value, (uint32_t)-EINVAL);
}

ZTEST(rtfw, test_framework_event_precedes_client_fastpath_event)
{
	struct rtfw_command command = command_create(21U, 401U);

	push_client_event = true;
	zassert_ok(rtfw_submit(&command));
	rtfw_fastpath_run();
	zassert_ok(fastpath_push_result);

	rtfw_test_delivery_signal();
	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	rtfw_test_delivery_drain();

	zassert_equal(delivered_count, 2U);
	zassert_equal(delivered_types[0], RTFW_EVENT_COMMAND_PROCESSED);
	zassert_equal(delivered_types[1], TEST_EVENT_TYPE);
}

ZTEST(rtfw, test_framework_event_reports_queue_overflow)
{
	struct rtfw_status status;

	for (uint32_t i = 0U; i < CONFIG_RTFW_EVENT_QUEUE_SIZE; i++) {
		struct rtfw_command command = command_create(22U, i);

		zassert_ok(rtfw_submit(&command));
		rtfw_fastpath_run();
	}

	zassert_ok(rtfw_submit(&(struct rtfw_command){.id = 23U}));
	rtfw_fastpath_run();
	zassert_ok(rtfw_get_status(&status));
	zassert_equal(status.dropped_events, 1U);
	zassert_true((status.faults & RTFW_FAULT_EVENT_OVERFLOW) != 0U);
}

static void status_copy_preempt(void)
{
	rtfw_test_status_copy_hook_set(NULL);
	rtfw_fastpath_run();
}

ZTEST(rtfw, test_status_retries_when_ack_changes)
{
	struct rtfw_command first = command_create(30U, 600U);
	struct rtfw_command coalesced = command_create(31U, 700U);
	struct rtfw_command rejected = command_create(UINT32_MAX, 800U);
	struct rtfw_status status;

	zassert_ok(rtfw_submit(&first));
	rtfw_fastpath_run();
	zassert_ok(rtfw_submit(&coalesced));
	zassert_ok(rtfw_submit(&rejected));
	rtfw_test_status_copy_hook_set(status_copy_preempt);

	zassert_ok(rtfw_get_status(&status));
	zassert_false(status.pending);
	zassert_equal(status.requested.id, rejected.id);
	zassert_equal(status.attempted.id, rejected.id);
	zassert_equal(status.applied.id, first.id);
	zassert_equal(command_value(&status.applied), 600U);
	zassert_equal(status.apply_result, -EINVAL);
	zassert_equal(apply_count, 2U);
	zassert_equal(last_handled.id, rejected.id);
}

ZTEST(rtfw, test_queue_capacity_and_bounded_delivery)
{
	struct rtfw_status status;
	uint32_t runs_before;

	/* The public SPSC fills exactly to capacity and reports overflow. */
	for (uint32_t i = 0U; i < CONFIG_RTFW_EVENT_QUEUE_SIZE; i++) {
		struct rtfw_event event = {
			.type = TEST_EVENT_TYPE,
			.value = i,
			.timestamp = i,
		};

		zassert_ok(rtfw_event_push(&event));
	}
	zassert_equal(rtfw_event_push(
			      &(struct rtfw_event){.type = TEST_EVENT_TYPE}),
		      -ENOSPC);
	zassert_equal(rtfw_test_doorbell_count_get(), 1U);
	zassert_ok(rtfw_get_status(&status));
	zassert_equal(status.dropped_events, 1U);
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	zassert_equal(status.max_queue_depth, CONFIG_RTFW_EVENT_QUEUE_SIZE);
#else
	zassert_equal(status.max_queue_depth, 0U);
#endif
	zassert_true((status.faults & RTFW_FAULT_EVENT_OVERFLOW) != 0U);

	/* One work invocation never exceeds the configured drain budget. */
	runs_before = rtfw_test_delivery_runs_get();
	rtfw_test_delivery_signal();
	for (uint32_t i = 0U; i < CONFIG_RTFW_EVENT_QUEUE_SIZE; i++) {
		zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	}
	rtfw_test_delivery_drain();
	zassert_equal(delivered_count, CONFIG_RTFW_EVENT_QUEUE_SIZE);
	zassert_true(rtfw_test_delivery_runs_get() - runs_before >=
		     DIV_ROUND_UP(CONFIG_RTFW_EVENT_QUEUE_SIZE,
				  CONFIG_RTFW_DRAIN_BUDGET));
}

static void delivery_release_produce(void)
{
	const struct rtfw_event event = {
		.type = TEST_EVENT_TYPE,
		.value = 2U,
	};

	rtfw_test_delivery_release_hook_set(NULL);
	release_push_result = rtfw_event_push(&event);
}

ZTEST(rtfw, test_queue_depth_handles_push_during_release)
{
	const struct rtfw_event first = {
		.type = TEST_EVENT_TYPE,
		.value = 1U,
	};
	struct rtfw_status status;

	zassert_ok(rtfw_event_push(&first));
	rtfw_test_delivery_release_hook_set(delivery_release_produce);
	rtfw_test_delivery_signal();

	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	rtfw_test_delivery_drain();
	zassert_ok(release_push_result);
	zassert_equal(delivered_count, 2U);
	zassert_ok(rtfw_get_status(&status));
#if defined(CONFIG_RTFW_QUEUE_USAGE_STATS)
	zassert_equal(status.max_queue_depth, 1U);
#else
	zassert_equal(status.max_queue_depth, 0U);
#endif
}

static void delivery_rearm_produce(void)
{
	const struct rtfw_event event = {
		.type = TEST_EVENT_TYPE,
		.value = 2U,
	};

	rtfw_test_delivery_rearm_hook_set(NULL);
	rearm_push_result = rtfw_event_push(&event);
}

ZTEST(rtfw, test_delivery_rearms_when_producer_observes_armed_state)
{
	const struct rtfw_event first = {
		.type = TEST_EVENT_TYPE,
		.value = 1U,
	};
	const struct rtfw_event after_rearm = {
		.type = TEST_EVENT_TYPE,
		.value = 3U,
	};

	zassert_ok(rtfw_event_push(&first));
	rtfw_test_delivery_rearm_hook_set(delivery_rearm_produce);
	rtfw_test_delivery_signal();

	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	zassert_ok(k_sem_take(&delivered, K_SECONDS(1)));
	rtfw_test_delivery_drain();
	zassert_ok(rearm_push_result);
	zassert_equal(delivered_count, 2U);
	zassert_equal(last_delivered.value, 2U);
	zassert_equal(rtfw_test_doorbell_count_get(), 1U);
	zassert_ok(rtfw_event_push(&after_rearm));
	zassert_equal(rtfw_test_doorbell_count_get(), 2U);
}

ZTEST_SUITE(rtfw, NULL, suite_setup, before_each, NULL, NULL);

#endif /* CONFIG_RTFW_TEST_PREINIT */
