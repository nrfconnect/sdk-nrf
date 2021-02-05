/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <secure_services.h>
#include <kernel.h>

static struct k_delayed_work interrupting_work;
static volatile bool work_done;

static void work_func(struct k_work *work)
{
	work_done = true;

	/* Call the secure service here as well, to test the added complexity of
	 * calling secure services from two threads.
	 */
	spm_busy_wait(20000);
}

void test_spm_service_thread_swap1(void)
{
	int err;

	work_done = false;

	/* Set up interrupting_work to fire while spm_busy_wait() is executing.
	 * This tests that it is safe to switch threads while a secure service
	 * is running.
	 */
	k_delayed_work_init(&interrupting_work, work_func);
	err = k_delayed_work_submit(&interrupting_work, K_MSEC(10));
	zassert_equal(0, err, "k_delayed_work failed: %d\n", err);

	/* Call into the secure service which will be interrupted. If the
	 * scheduler is locked before entering secure services, it will prevent
	 * work_func from being run until after the below function returns, but
	 * this is valid behavior.
	 */
	spm_busy_wait(20000);
	zassert_true(work_done, "work_func should have interrupted wait.");
}

void test_main(void)
{
	ztest_test_suite(test_spm_service_thread_swap,
			ztest_unit_test(test_spm_service_thread_swap1)
			);
	ztest_run_test_suite(test_spm_service_thread_swap);
}
