/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * Taken from zephyr/tests/kernel/context/kernel.context
 */

/*
 * @brief Timeout tests
 *
 * Test the k_sleep() API, as well as the k_thread_create() ones.
 */
struct timeout_order {
	void *link_in_fifo;
	int32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order timeouts[] = {
	{0, 1000, 2, 0}, {0, 1500, 4, 1}, {0, 500, 0, 2},  {0, 750, 1, 3},
	{0, 1750, 5, 4}, {0, 2000, 6, 5}, {0, 1250, 3, 6},
};

#define THREAD_PRIORITY	    4
#define THREAD_STACKSIZE2   (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_TIMEOUT_THREADS ARRAY_SIZE(timeouts)
static K_THREAD_STACK_ARRAY_DEFINE(timeout_stacks, NUM_TIMEOUT_THREADS, THREAD_STACKSIZE2);
static struct k_thread timeout_threads[NUM_TIMEOUT_THREADS];
static struct k_sem reply_timeout;
static struct k_timer timer;
struct k_fifo timeout_order_fifo;

/* a thread busy waits */
static void busy_wait_thread(void *mseconds, void *arg2, void *arg3)
{
	uint32_t usecs;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	usecs = POINTER_TO_INT(mseconds) * 1000;

	k_busy_wait(usecs);

	int key = arch_irq_lock();

	k_busy_wait(usecs);
	arch_irq_unlock(key);

	/*
	 * Ideally the test should verify that the correct number of ticks
	 * have elapsed. However, when running under QEMU, the tick interrupt
	 * may be processed on a very irregular basis, meaning that far
	 * fewer than the expected number of ticks may occur for a given
	 * number of clock cycles vs. what would ordinarily be expected.
	 *
	 * Consequently, the best we can do for now to test busy waiting is
	 * to invoke the API and verify that it returns. (If it takes way
	 * too long, or never returns, the main test thread may be able to
	 * time out and report an error.)
	 */

	k_sem_give(&reply_timeout);
}

/* a thread sleeps and times out, then reports through a fifo */
static void thread_sleep(void *delta, void *arg2, void *arg3)
{
	int64_t timestamp;
	int timeout = POINTER_TO_INT(delta);

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	timestamp = k_uptime_get();
	k_msleep(timeout);
	timestamp = k_uptime_get() - timestamp;

	int slop = MAX(k_ticks_to_ms_floor64(2), 1);

	if (timestamp < timeout || timestamp > timeout + slop) {
		TC_ERROR("timestamp out of range, got %d\n", (int)timestamp);
		return;
	}

	k_sem_give(&reply_timeout);
}

/* a thread is started with a delay, then it reports that it ran via a fifo */
static void delayed_thread(void *num, void *arg2, void *arg3)
{
	struct timeout_order *timeout = &timeouts[POINTER_TO_INT(num)];

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	TC_PRINT(" thread (q order: %d, t/o: %d) is running\n", timeout->q_order, timeout->timeout);

	k_fifo_put(&timeout_order_fifo, timeout);
}

/**
 *
 * @brief Initialize kernel objects
 *
 * This routine initializes the kernel objects used in this module's tests.
 *
 */
void kernel_init_objects(void)
{
	k_sem_init(&reply_timeout, 0, UINT_MAX);
	k_timer_init(&timer, NULL, NULL);
	k_fifo_init(&timeout_order_fifo);
}

/**
 * @brief Test timeouts
 *
 * @ingroup kernel_context_tests
 *
 * @see k_busy_wait(), k_sleep()
 */
void test_busy_wait(void)
{
	int32_t timeout;
	int rv;

	timeout = 20; /* in ms */

	k_thread_create(&timeout_threads[0], timeout_stacks[0], THREAD_STACKSIZE2, busy_wait_thread,
			INT_TO_POINTER(timeout), NULL, NULL, K_PRIO_COOP(THREAD_PRIORITY), 0,
			K_NO_WAIT);

	rv = k_sem_take(&reply_timeout, K_MSEC(timeout * 2 * 2));

	zassert_false(rv, " *** thread timed out waiting for "
			  "k_busy_wait()");
}

/**
 * @brief Test timeouts
 *
 * @ingroup kernel_context_tests
 *
 * @see k_sleep()
 */
void test_k_sleep(void)
{
	struct timeout_order *data;
	int32_t timeout;
	int rv;
	int i;

	timeout = 50;

	k_thread_create(&timeout_threads[0], timeout_stacks[0], THREAD_STACKSIZE2, thread_sleep,
			INT_TO_POINTER(timeout), NULL, NULL, K_PRIO_COOP(THREAD_PRIORITY), 0,
			K_NO_WAIT);

	rv = k_sem_take(&reply_timeout, K_MSEC(timeout * 2));
	zassert_equal(rv, 0,
		      " *** thread timed out waiting for thread on "
		      "k_sleep().");

	/* test k_thread_create() without cancellation */
	TC_PRINT("Testing k_thread_create() without cancellation\n");

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_thread_create(&timeout_threads[i], timeout_stacks[i], THREAD_STACKSIZE2,
				delayed_thread, INT_TO_POINTER(i), NULL, NULL, K_PRIO_COOP(5), 0,
				K_MSEC(timeouts[i].timeout));
	}
	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));
		zassert_not_null(data, " *** timeout while waiting for"
				       " delayed thread");

		zassert_equal(data->timeout_order, i,
			      " *** wrong delayed thread ran (got %d, "
			      "expected %d)\n",
			      data->timeout_order, i);

		TC_PRINT(" got thread (q order: %d, t/o: %d) as expected\n", data->q_order,
			 data->timeout);
	}

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));

	zassert_false(data, " *** got something unexpected in the fifo");

	/* test k_thread_create() with cancellation */
	TC_PRINT("Testing k_thread_create() with cancellations\n");

	int cancellations[] = {0, 3, 4, 6};
	int num_cancellations = ARRAY_SIZE(cancellations);
	int next_cancellation = 0;

	k_tid_t delayed_threads[NUM_TIMEOUT_THREADS];

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_tid_t id;

		id = k_thread_create(&timeout_threads[i], timeout_stacks[i], THREAD_STACKSIZE2,
				     delayed_thread, INT_TO_POINTER(i), NULL, NULL, K_PRIO_COOP(5),
				     0, K_MSEC(timeouts[i].timeout));

		delayed_threads[i] = id;
	}

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		int j;

		if (i == cancellations[next_cancellation]) {
			TC_PRINT(" cancelling "
				 "[q order: %d, t/o: %d, t/o order: %d]\n",
				 timeouts[i].q_order, timeouts[i].timeout, i);

			for (j = 0; j < NUM_TIMEOUT_THREADS; j++) {
				if (timeouts[j].timeout_order == i) {
					break;
				}
			}

			if (j < NUM_TIMEOUT_THREADS) {
				k_thread_abort(delayed_threads[j]);
				++next_cancellation;
				continue;
			}
		}

		data = k_fifo_get(&timeout_order_fifo, K_MSEC(2750));

		zassert_not_null(data, " *** timeout while waiting for"
				       " delayed thread");

		zassert_equal(data->timeout_order, i,
			      " *** wrong delayed thread ran (got %d, "
			      "expected %d)\n",
			      data->timeout_order, i);

		TC_PRINT(" got (q order: %d, t/o: %d, t/o order %d) "
			 "as expected\n",
			 data->q_order, data->timeout, data->timeout_order);
	}

	zassert_equal(num_cancellations, next_cancellation,
		      " *** wrong number of cancellations (expected %d, "
		      "got %d\n",
		      num_cancellations, next_cancellation);

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));
	zassert_false(data, " *** got something unexpected in the fifo");
}
