/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <qos.h>

/* Include header that exposes internal variables in QoS library. */
#include "vars_internal.h"

/* Test message type. */
#define TEST_MESSAGE_TYPE 0

/* Dummy payload. */
static uint8_t *var = "some text";
#define DUMMY_SIZE sizeof(var)

static uint8_t callback_count;

/* Function used to verify internal context variables. */
static void ctx_verify(struct ctx *expected)
{
	TEST_ASSERT_EQUAL(ctx.app_evt_handler,
			  expected->app_evt_handler);
	TEST_ASSERT_EQUAL(ctx.timeout_handler_work.work.handler,
			  expected->timeout_handler_work.work.handler);
	TEST_ASSERT_EQUAL(ctx.pending_list.head, expected->pending_list.head);
	TEST_ASSERT_EQUAL(ctx.pending_list.tail, expected->pending_list.tail);
	TEST_ASSERT_EQUAL(ctx.initialized, expected->initialized);
	TEST_ASSERT_EQUAL(ctx.message_id_next, expected->message_id_next);
}

static void dut_event_handler(const struct qos_evt *evt)
{
	switch (evt->type) {
	case QOS_EVT_MESSAGE_NEW:
	case QOS_EVT_MESSAGE_TIMER_EXPIRED:
		/* Fall through. */
	case QOS_EVT_MESSAGE_REMOVED_FROM_LIST:
		callback_count++;
		break;
	default:
		TEST_FAIL();
		break;
	}
}

static void dut_event_handler_expired(const struct qos_evt *evt)
{
	if (evt->type == QOS_EVT_MESSAGE_TIMER_EXPIRED) {
		callback_count++;
		return;
	}

	TEST_FAIL();
}

static void dut_event_handler_removed(const struct qos_evt *evt)
{
	if (evt->type == QOS_EVT_MESSAGE_REMOVED_FROM_LIST) {
		callback_count++;
		return;
	}

	TEST_FAIL();
}

static void init(void)
{
	struct ctx expected =  {
		.app_evt_handler = NULL,
		.pending_list.head = NULL,
		.pending_list.tail = NULL,
		.initialized = false,
		.timeout_handler_work.work.handler = NULL,
		.message_id_next = QOS_MESSAGE_ID_BASE
	};

	/* Verify internal context structure */
	ctx_verify(&expected);

	/* Test invalid handler argument */
	TEST_ASSERT_EQUAL(-EINVAL, qos_init(NULL));

	/* Successful initialization */
	TEST_ASSERT_FALSE(qos_init(dut_event_handler));

	/* Verify that internal variables has been initialized. */
	expected.app_evt_handler = &dut_event_handler;
	expected.initialized = true;
	expected.timeout_handler_work.work.handler = &timeout_handler_work_fn;

	ctx_verify(&expected);

	/* Test subsequent calls */
	TEST_ASSERT_EQUAL(-EALREADY, qos_init(dut_event_handler));
}

void setUp(void)
{
	ctx.message_id_next = QOS_MESSAGE_ID_BASE;
	init();
}

void tearDown(void)
{
	k_work_cancel_delayable(&ctx.timeout_handler_work);
	memset(&ctx, 0, sizeof(struct ctx));
	callback_count = 0;
}

void test_message_add(void)
{
	struct qos_metadata *node = NULL, *next_node = NULL;
	uint8_t count = 0;
	struct qos_data message = {
		.heap_allocated = true,
		.data.buf = var,
		.data.len = DUMMY_SIZE,
		.id = qos_message_id_get_next(),
		.type = TEST_MESSAGE_TYPE,
		.flags = QOS_FLAG_RELIABILITY_ACK_DISABLED
	};

	/* Test for invalid argument. */
	TEST_ASSERT_EQUAL(-EINVAL, qos_message_add(NULL));

	/* Add single message. */
	TEST_ASSERT_FALSE(qos_message_add(&message));

	/* Expect the QOS_EVT_MESSAGE_NEW and QOS_EVT_MESSAGE_REMOVED_FROM_LIST to be called once
	 * for the message flagged with QOS_FLAG_RELIABILITY_ACK_DISABLED.
	 */
	TEST_ASSERT_EQUAL(2, callback_count);

	/* Verify that internal list contains no entries. */
	TEST_ASSERT_TRUE(sys_slist_is_empty(&ctx.pending_list));

	/* Fill pending list */
	callback_count = 0;
	message.flags = QOS_FLAG_RELIABILITY_ACK_REQUIRED;
	for (int i = 0; i < CONFIG_QOS_PENDING_MESSAGES_MAX; i++) {
		qos_message_print(&message);
		TEST_ASSERT_FALSE(qos_message_add(&message));
		message.id = qos_message_id_get_next();
	}

	/* Verify that all messages has been notified in the event handler. */
	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);

	/* Verify return value if a message that is added to a full list. */
	TEST_ASSERT_EQUAL(-ENOMEM, qos_message_add(&message));

	/* Check number of list entries populated. */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx.pending_list, node, next_node, header) {
		count++;
	};

	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, count);

	/* When there are entries in the pending list the
	 * internal delayed work should be running.
	 */
	TEST_ASSERT_TRUE(k_work_delayable_is_pending(&ctx.timeout_handler_work));

	callback_count = 0;
}

void test_message_remove(void)
{
	uint8_t count = 0;
	struct qos_metadata *node = NULL, *next_node = NULL;
	struct qos_data message = {
		.heap_allocated = true,
		.data.buf = var,
		.data.len = DUMMY_SIZE,
		.id = QOS_MESSAGE_ID_BASE,
		.type = TEST_MESSAGE_TYPE,
		.flags = QOS_FLAG_RELIABILITY_ACK_REQUIRED
	};

	test_message_add();

	/* Remove a single message and check if list contains max - 1. */
	TEST_ASSERT_EQUAL(0, qos_message_remove(QOS_MESSAGE_ID_BASE));

	/* Timeout work should still be running when there are entries in the internal
	 * list.
	 */
	TEST_ASSERT_TRUE(k_work_delayable_is_pending(&ctx.timeout_handler_work));

	/* Remove a message that is not in the internal pending list. */
	TEST_ASSERT_EQUAL(-ENODATA, qos_message_remove(QOS_MESSAGE_ID_BASE));

	/* Check number of list entries populated. */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx.pending_list, node, next_node, header) {
		count++;
	};

	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX - 1, count);

	/* Expect QOS_EVT_MESSAGE_REMOVED_FROM_LIST to be called once for the removed message. */
	TEST_ASSERT_EQUAL(1, callback_count);

	/* Clear list */
	qos_message_remove_all();

	/* Expect QOS_EVT_MESSAGE_REMOVED_FROM_LIST to be called once for all removed messages. */
	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);

	/* Add and remove single message and verify that internal timeout work is not running. */
	TEST_ASSERT_FALSE(qos_message_add(&message));
	TEST_ASSERT_FALSE(qos_message_remove(QOS_MESSAGE_ID_BASE));
	TEST_ASSERT_FALSE(k_work_delayable_is_pending(&ctx.timeout_handler_work));
}

void test_message_remove_all(void)
{
	test_message_add();
	qos_message_remove_all();

	/* Verify that the internal list has been emptied and the internal delayed
	 * work is not running.
	 */
	TEST_ASSERT_TRUE(sys_slist_is_empty(&ctx.pending_list));
	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);
	TEST_ASSERT_FALSE(k_work_delayable_is_pending(&ctx.timeout_handler_work));
}

void test_message_notify_all(void)
{
	test_message_add();
	qos_message_notify_all();

	/* Verify that all items in the pending list has been notified and that the delayed
	 * work is still running.
	 */
	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);
	TEST_ASSERT_TRUE(k_work_delayable_is_pending(&ctx.timeout_handler_work));
}

void test_message_timer_reset(void)
{
	test_message_add();

	TEST_ASSERT_TRUE(k_work_delayable_is_pending(&ctx.timeout_handler_work));

	qos_timer_reset();

	TEST_ASSERT_FALSE(k_work_delayable_is_pending(&ctx.timeout_handler_work));
}

void test_message_id_get_next(void)
{
	for (int i = QOS_MESSAGE_ID_BASE; i < UINT16_MAX; i++) {
		TEST_ASSERT_EQUAL(i, qos_message_id_get_next());
	}

	/* The next call should return QOS_MESSAGE_ID_BASE. */
	TEST_ASSERT_EQUAL(QOS_MESSAGE_ID_BASE, qos_message_id_get_next());
}

void test_message_has_flag(void)
{
	struct qos_data dummy = { 0 };

	TEST_ASSERT_FALSE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_DISABLED));
	TEST_ASSERT_FALSE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_REQUIRED));

	dummy.flags = QOS_FLAG_RELIABILITY_ACK_DISABLED;

	TEST_ASSERT_TRUE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_DISABLED));
	TEST_ASSERT_FALSE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_REQUIRED));

	dummy.flags = QOS_FLAG_RELIABILITY_ACK_REQUIRED;

	TEST_ASSERT_TRUE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_REQUIRED));
	TEST_ASSERT_FALSE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_DISABLED));

	/* Set multiple flags and verify if API returns true for both. */
	dummy.flags = QOS_FLAG_RELIABILITY_ACK_DISABLED | QOS_FLAG_RELIABILITY_ACK_REQUIRED;

	TEST_ASSERT_TRUE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_REQUIRED));
	TEST_ASSERT_TRUE(qos_message_has_flag(&dummy, QOS_FLAG_RELIABILITY_ACK_DISABLED));
}

void test_message_timeout_work(void)
{
	struct qos_metadata *node = NULL, *next_node = NULL;
	struct qos_data message = {
		.heap_allocated = true,
		.data.buf = var,
		.data.len = DUMMY_SIZE,
		.id = qos_message_id_get_next(),
		.type = TEST_MESSAGE_TYPE,
		.flags = QOS_FLAG_RELIABILITY_ACK_REQUIRED
	};

	for (int i = 0; i < CONFIG_QOS_PENDING_MESSAGES_MAX; i++) {
		TEST_ASSERT_FALSE(qos_message_add(&message));
		message.id = qos_message_id_get_next();
	}

	k_work_cancel_delayable(&ctx.timeout_handler_work);

	callback_count = 0;
	ctx.app_evt_handler = &dut_event_handler_expired;

	/* Manually run the internal work. */
	timeout_handler_work_fn(NULL);
	k_work_cancel_delayable(&ctx.timeout_handler_work);

	/* Expect QOS_EVT_MESSAGE_TIMER_EXPIRED to be returned for every message in list. */
	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);

	callback_count = 0;
	ctx.app_evt_handler = &dut_event_handler_removed;

	/* Set every list item to the maximum allowed notified count. */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx.pending_list, node, next_node, header) {
		node->message.notified_count = CONFIG_QOS_MESSAGE_NOTIFIED_COUNT_MAX;
	};

	timeout_handler_work_fn(NULL);
	k_work_cancel_delayable(&ctx.timeout_handler_work);

	TEST_ASSERT_EQUAL(CONFIG_QOS_PENDING_MESSAGES_MAX, callback_count);

	/* Subsequent calls should not trigger callbacks because the list is empty. */
	callback_count = 0;

	timeout_handler_work_fn(NULL);

	TEST_ASSERT_EQUAL(0, callback_count);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
