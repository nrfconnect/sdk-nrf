/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <drivers/clock_control.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#include <drivers/clock_control/nrf_clock_control.h>

struct device_subsys_data {
	clock_control_subsys_t subsys;
	u32_t startup_us;
};

struct device_data {
	const char *name;
	const struct device_subsys_data *subsys_data;
	u32_t subsys_cnt;
};

static const struct device_data devices[] = {
	{
		.name = DT_LABEL(DT_INST(0, nordic_nrf_clock)),
		.subsys_data =  (const struct device_subsys_data[]){
			{
				.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF,
				.startup_us =
					IS_ENABLED(CONFIG_SOC_SERIES_NRF91X) ?
						3000 : 400
			},
			{
				.subsys = CLOCK_CONTROL_NRF_SUBSYS_LF,
				.startup_us = (CLOCK_CONTROL_NRF_K32SRC ==
					NRF_CLOCK_LFCLK_RC) ? 1000 : 300000
			}
		},
		.subsys_cnt = CLOCK_CONTROL_NRF_TYPE_COUNT
	}
};


typedef void (*test_func_t)(const char *dev_name,
			    clock_control_subsys_t subsys,
			    u32_t startup_us);

typedef bool (*test_capability_check_t)(const char *dev_name,
					clock_control_subsys_t subsys);

/* LFCLK is started by default by mpsl_init(). */
#define MPSL_CLOCK_CONTROL_STATUS_OFF(subsys)                       \
	(((subsys) == CLOCK_CONTROL_NRF_SUBSYS_LF) ?                \
		CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF)

/* Stopping of LFCLK is not supported. */
#define MPSL_CLOCK_CONTROL_OFF(subsys) \
	(((subsys) == CLOCK_CONTROL_NRF_SUBSYS_LF) ? -ENOTSUP : 0)

static void setup_instance(const char *dev_name, clock_control_subsys_t subsys)
{
	struct device *dev = device_get_binding(dev_name);
	int err;

	k_busy_wait(1000);
	do {
		err = clock_control_off(dev, subsys);
	} while (err == 0);
	LOG_INF("setup done");
}

static void tear_down_instance(const char *dev_name,
				clock_control_subsys_t subsys)
{
	struct device *dev = device_get_binding(dev_name);

	clock_control_on(dev, subsys);
}

static void test_with_single_instance(const char *dev_name,
				      clock_control_subsys_t subsys,
				      u32_t startup_time,
				      test_func_t func,
				      test_capability_check_t capability_check)
{
	if ((capability_check == NULL) || capability_check(dev_name, subsys)) {
		setup_instance(dev_name, subsys);
		func(dev_name, subsys, startup_time);
		tear_down_instance(dev_name, subsys);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
}

static void test_all_instances(test_func_t func,
				test_capability_check_t capability_check)
{
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		for (size_t j = 0; j < devices[i].subsys_cnt; j++) {
			test_with_single_instance(devices[i].name,
					devices[i].subsys_data[j].subsys,
					devices[i].subsys_data[j].startup_us,
					func, capability_check);
		}
	}
}

/*
 * Basic test for checking correctness of getting clock status.
 */
static void test_on_off_status_instance(const char *dev_name,
					clock_control_subsys_t subsys,
					u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;

	zassert_true(dev != NULL, "%s: Unknown device", dev_name);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);


	err = clock_control_on(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);
}

static void test_on_off_status(void)
{
	test_all_instances(test_on_off_status_instance, NULL);
}

/*
 * Test validates that if number of enabling requests matches disabling requests
 * then clock is disabled.
 */
static void test_multiple_users_instance(const char *dev_name,
					 clock_control_subsys_t subsys,
					 u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int users = 5;
	int err;

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	for (int i = 0; i < users; i++) {
		err = clock_control_on(dev, subsys);
		zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
	}

	for (int i = 0; i < users; i++) {
		err = clock_control_off(dev, subsys);
		zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
				"%s: Unexpected err (%d)", dev_name, err);
	}

	status = clock_control_get_status(dev, subsys);
	zassert_true(status == MPSL_CLOCK_CONTROL_STATUS_OFF(subsys),
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	if (subsys == CLOCK_CONTROL_NRF_SUBSYS_LF) {
		/* Stopping of LFCLK is not supported. */
		zassert_equal(-ENOTSUP, err,
				"%s: Unexpected err (%d)", dev_name, err);
	} else {
		zassert_equal(-EALREADY, err,
				"%s: Unexpected err (%d)", dev_name, err);
	}
}

static void test_multiple_users(void)
{
	test_all_instances(test_multiple_users_instance, NULL);
}

static bool async_capable(const char *dev_name, clock_control_subsys_t subsys)
{
	struct device *dev = device_get_binding(dev_name);

	if (clock_control_async_on(dev, subsys, NULL) != 0) {
		return false;
	}

	clock_control_off(dev, subsys);

	return true;
}

/*
 * Test checks that callbacks are called after clock is started.
 */
static void clock_on_callback(struct device *dev,
			      clock_control_subsys_t subsys,
			      void *user_data)
{
	bool *executed = (bool *)user_data;

	*executed = true;
}

/*
 * Test checks that when asynchronous clock enabling is called twice.
 * The clock should keep on until it is closed by all users.
 */
static void test_async_on_off_instance(const char *dev_name,
				       clock_control_subsys_t subsys,
				       u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	bool executed1 = false;
	bool executed2 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};
	struct clock_control_async_data data2 = {
		.cb = clock_on_callback,
		.user_data = &executed2
	};

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, subsys, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	err = clock_control_async_on(dev, subsys, &data2);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* wait for clock started. */
	k_busy_wait(startup_us);

	zassert_true(executed1, "%s: Expected flag to be true", dev_name);
	zassert_true(executed2, "%s: Expected flag to be true", dev_name);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	/* the clock should keep on until no more reference. */
	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);
}

static void test_async_on_off(void)
{
	test_all_instances(test_async_on_off_instance, async_capable);
}

/*
 * Test callback used to count the number of executed async_on requests.
 */
static void clock_on_counting_callback(struct device *dev,
				       clock_control_subsys_t subsys,
				       void *user_data)
{
	int *count = (int *)user_data;

	++(*count);
}

/*
 * Test validates that the number of callbacks matches the executed async_on
 * requests. The clock should keep on until it is closed by all users.
 */
static void test_async_on_off_multiple_users_instance(const char *dev_name,
						clock_control_subsys_t subsys,
						u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int users = 5;
	int err;
	int count = 0;
	struct clock_control_async_data data[users];

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	for (int i = 0; i < users; i++) {
		data[i].cb = clock_on_counting_callback;
		data[i].user_data = &count;
		err = clock_control_async_on(dev, subsys, &data[i]);
		zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
	}

	/* wait for clock started. */
	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	zassert_equal(users, count,
			"%s: Unexpected count (%d)", dev_name, count);

	for (int i = 0; i < (users - 1); i++) {
		err = clock_control_off(dev, subsys);
		zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
				"%s: Unexpected err (%d)", dev_name, err);
	}

	/* the clock should keep on until no more reference. */
	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);
}

/*
 * Test validates that when the number of async_on request is over the
 * supporting counter, HFCLK-clock shall return -ENOTSUP.
 */
static void test_async_on_too_many_users_instance(const char *dev_name,
						clock_control_subsys_t subsys,
						u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	int count = 0;
	static struct clock_control_async_data data[256];
	int users =  sizeof(data) / sizeof(struct clock_control_async_data);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	for (int i = 0; i < (users - 1); i++) {
		data[i].cb = clock_on_counting_callback;
		data[i].user_data = &count;
		err = clock_control_async_on(dev, subsys, &data[i]);
		zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
	}

	/* wait for clock started. */
	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, subsys, &data[(users - 1)]);
	if (subsys == CLOCK_CONTROL_NRF_SUBSYS_LF) {
		zassert_equal(0, err,
				"%s: Unexpected err (%d)", dev_name, err);
	} else {
		/* This request is over the supporting counter */
		zassert_equal(-ENOTSUP, err,
				"%s: Unexpected err (%d)", dev_name, err);
	}

	zassert_equal((users - 1), count,
			"%s: Unexpected count (%d)", dev_name, count);

	for (int i = 0; i < (users - 1); i++) {
		err = clock_control_off(dev, subsys);
		zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
				"%s: Unexpected err (%d)", dev_name, err);
	}

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	if (subsys == CLOCK_CONTROL_NRF_SUBSYS_LF) {
		/* Stopping of LFCLK is not supported. */
		zassert_equal(-ENOTSUP, err,
				"%s: Unexpected err (%d)", dev_name, err);
	} else {
		zassert_equal(-EALREADY, err,
				"%s: Unexpected err (%d)", dev_name, err);
	}
}

static void test_async_on_off_multiple_users(void)
{
	test_all_instances(test_async_on_off_multiple_users_instance,
			   async_capable);
	test_all_instances(test_async_on_too_many_users_instance,
			   async_capable);
}

/*
 * Test checks that when asynchronous clock enabling is scheduled but clock
 * is disabled before being started then callback is never called.
 */
static void test_async_on_stopped_instance(const char *dev_name,
					   clock_control_subsys_t subsys,
					   u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	int key;
	bool executed1 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	/* lock to prevent clock interrupt for fast starting clocks.*/
	key = irq_lock();
	err = clock_control_async_on(dev, subsys, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	irq_unlock(key);

	k_busy_wait(10000);

	if (subsys == CLOCK_CONTROL_NRF_SUBSYS_LF) {
		/* For LFCLK, the callback will be immediately called from the
		 * context of the function call.
		 */
		zassert_true(executed1,
				"%s: Expected flag to be true", dev_name);
	} else {
		zassert_false(executed1,
				"%s: Expected flag to be false", dev_name);
	}
}

static void test_async_on_stopped(void)
{
	test_all_instances(test_async_on_stopped_instance, async_capable);
}

/*
 * Test checks that when asynchronous clock enabling is called when clock is
 * running then callback is immediate.
 */
static void test_immediate_cb_when_clock_on_instance(const char *dev_name,
						clock_control_subsys_t subsys,
						u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	bool executed1 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_on(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* wait for clock started. */
	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, subsys, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	zassert_true(executed1, "%s: Expected flag to be true", dev_name);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	/* the clock should keep on until no more reference. */
	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_OFF(subsys), err,
			"%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(MPSL_CLOCK_CONTROL_STATUS_OFF(subsys), status,
			"%s: Unexpected status (%d)", dev_name, status);
}

static void test_immediate_cb_when_clock_on(void)
{
	test_all_instances(test_immediate_cb_when_clock_on_instance,
			   async_capable);
}

void test_main(void)
{
	ztest_test_suite(test_clock_control,
		ztest_unit_test(test_on_off_status),
		ztest_unit_test(test_multiple_users),
		ztest_unit_test(test_async_on_off),
		ztest_unit_test(test_async_on_off_multiple_users),
		ztest_unit_test(test_async_on_stopped),
		ztest_unit_test(test_immediate_cb_when_clock_on)
			 );
	ztest_run_test_suite(test_clock_control);
}
