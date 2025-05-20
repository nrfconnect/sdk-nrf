/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/kernel_structs.h>

FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason, const struct arch_esf *pEsf)
{
	TC_PRINT("SysFatalErrorHandler called - reason %d\n", reason);
	k_thread_abort(_current);
	CODE_UNREACHABLE;
}

/* In Zephyr 1.14 we could call k_oops and run tests as long as the flag
 * "ignore_faults" was present in the yaml.
 * This is no longer the case. Hence, we must mock k_oops and have this call the
 * error handler directly.
 */
#undef k_oops
#define k_oops() _SysFatalErrorHandler(0, NULL)
#include "macros_common.h"

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(macros_main);

/* This test is to check correct functionality of all macros. Since the Zephyr
 * system has to be invoked in order to make the k_oops and error handlers work,
 * this example is fairly complex.
 * Please refer to other tests for template use.
 */

#define MAIN_PRIORITY 7
#define PRIORITY 5

#define ERROR_HANDLER_CALLED 1
#define ERROR_HANDLER_NOT_CALLED 0
#define MAGIC_RET_VAL 999

static K_THREAD_STACK_DEFINE(macros_stack, 8192);
static struct k_thread macros_thread;
volatile int rv;

void thread_err_chk(void *p_err_val, void *p_rv_in)
{
	int key;

	key = irq_lock();
	ERR_CHK(*(int *)p_err_val);

	rv = *(int *)p_rv_in;
	irq_unlock(key);
}

void thread_err_chk_msg(void *p_err_val, void *p_rv_in)
{
	int key;

	key = irq_lock();
	ERR_CHK_MSG(*(int *)p_err_val, "TEST MESSAGE");

	rv = *(int *)p_rv_in;
	irq_unlock(key);
}

void create_thread(void (*thread_ptr)(void *, void *), int err_val, int rv_in)
{
	k_thread_create(&macros_thread, macros_stack, K_THREAD_STACK_SIZEOF(macros_stack),
			(k_thread_entry_t)thread_ptr, (void *)&err_val, (void *)&rv_in, NULL,
			K_PRIO_COOP(PRIORITY), 0, K_NO_WAIT);
}

ZTEST(suite_macros, test_macro_err_chk)
{
	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	TC_PRINT("test_macro_err_chk_0: ERR_CHK(0)\n");
	rv = ERROR_HANDLER_NOT_CALLED;

	int rv_in = ERROR_HANDLER_CALLED;
	int err_val = 0;

	create_thread(thread_err_chk, err_val, rv_in);
	zassert_equal(rv, rv_in, "Thread was killed\n");

	TC_PRINT("test_macro_err_chk_neg: ERR_CHK(-1)\n");
	rv = ERROR_HANDLER_NOT_CALLED;
	err_val = -1;
	rv_in = ERROR_HANDLER_CALLED;
	create_thread(thread_err_chk, err_val, rv_in);
	zassert_not_equal(rv, rv_in, "Thread was not killed\n");

	TC_PRINT("test_macro_err_chk_pos: ERR_CHK(1)\n");
	rv = ERROR_HANDLER_NOT_CALLED;
	rv_in = ERROR_HANDLER_CALLED;
	err_val = 1;
	create_thread(thread_err_chk, err_val, rv_in);
	zassert_not_equal(rv, rv_in, "Thread was not killed\n");
}

ZTEST(suite_macros, test_macro_err_chk_msg)
{
	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	TC_PRINT("test_macro_err_chk_0: ERR_CHK_MSG(0)\n");
	rv = ERROR_HANDLER_NOT_CALLED;

	int rv_in = ERROR_HANDLER_CALLED;
	int err_val = 0;

	create_thread(thread_err_chk_msg, err_val, rv_in);
	zassert_equal(rv, rv_in, "Thread was killed\n");

	TC_PRINT("test_macro_err_chk_neg: ERR_CHK_MSG(-1)\n");
	rv = ERROR_HANDLER_NOT_CALLED;
	err_val = -1;
	rv_in = ERROR_HANDLER_CALLED;
	create_thread(thread_err_chk_msg, err_val, rv_in);
	zassert_not_equal(rv, rv_in, "Thread was not killed\n");

	TC_PRINT("test_macro_err_chk_pos: ERR_CHK_MSG(1)\n");
	rv = ERROR_HANDLER_NOT_CALLED;
	rv_in = ERROR_HANDLER_CALLED;
	err_val = 1;
	create_thread(thread_err_chk_msg, err_val, rv_in);
	zassert_not_equal(rv, rv_in, "Thread was not killed\n");
}

ZTEST(suite_macros, test_macro_min)
{
	int ret;

	ret = MIN(4, 3);
	zassert_equal(ret, 3, "Wrong value returned from MIN macro");

	ret = MIN(7, 77);
	zassert_equal(ret, 7, "Wrong value returned from MIN macro");

	ret = MIN(-2, -1);
	zassert_equal(ret, -2, "Wrong value returned from MIN macro");

	ret = MIN(12, -5);
	zassert_equal(ret, -5, "Wrong value returned from MIN macro");
}

ZTEST_SUITE(suite_macros, NULL, NULL, NULL, NULL, NULL);
