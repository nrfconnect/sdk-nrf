/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zephyr.h>
#include <ei_test_params.h>
#include <ei_wrapper.h>

#define EI_TEST_SEM_TIMEOUT  K_MSEC(200 + (10 * EI_MOCK_BUSY_WAIT_TIME / 1000))
#define EI_TEST_ISR_WINDOW_SHIFT		2
#define EI_TEST_THREAD_WINDOW_SHIFT		2
#define EI_TEST_WINDOW_SHIFT_CB			1

static atomic_t rerun_in_cb;

static size_t prediction_idx;
/* Semaphore is used to wait until ei_wrapper returns prediction results. */
static K_SEM_DEFINE(test_sem, 0, 1)


static int add_input_data(const size_t pred_idx, const size_t frame_surplus)
{
	static float data_buf[EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME];

	int err = 0;
	float value = EI_MOCK_GEN_FIRST_INPUT(pred_idx);
	size_t data_surplus =
		      frame_surplus * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

	for (size_t i = 0;
	     i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE + data_surplus;
	     i += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
		for (size_t j = 0; j < ARRAY_SIZE(data_buf); j++) {
			data_buf[j] = value;
			value++;
		}

		err = ei_wrapper_add_data(data_buf, EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
		if (err) {
			break;
		}
	}

	return err;
}

static void verify_result(const size_t pred_idx)
{
	int err;
	const char *label;
	float value;
	float anomaly;

	int dsp_time;
	int classification_time;
	int anomaly_time;

	err = ei_wrapper_get_classification_results(&label, &value, &anomaly);
	zassert_ok(err, "ei_wrapper_get_classification_results returned an error");
	err = ei_wrapper_get_timing(&dsp_time, &classification_time, &anomaly_time);
	zassert_ok(err, "ei_wrapper_get_timing returned an error");

	zassert_not_null(label, "Returned label is NULL");
	zassert_false(strcmp(label, EI_MOCK_GEN_LABEL(pred_idx)), "Wrong label");
	zassert_within(value, EI_MOCK_GEN_VALUE(pred_idx), FLOAT_CMP_EPSILON, "Wrong value");
	zassert_within(anomaly, EI_MOCK_GEN_ANOMALY(pred_idx), FLOAT_CMP_EPSILON,
		       "Wrong anomaly value");

	zassert_equal(dsp_time, EI_MOCK_GEN_DSP_TIME(pred_idx), "Wrong DSP time");
	zassert_equal(classification_time, EI_MOCK_GEN_CLASSIFICATION_TIME(pred_idx),
		      "Wrong classification time");
	zassert_equal(anomaly_time, EI_MOCK_GEN_ANOMALY_TIME(pred_idx), "Wrong anomaly time");
}

static void run_basic_setup(const size_t pred_idx,
			    const size_t add_data_cnt,
			    const size_t window_shift,
			    const size_t frame_shift)
{
	int err;

	for (size_t i = 0; i < add_data_cnt; i++) {
		err = add_input_data(pred_idx, 0);
		zassert_ok(err, "Cannot add input data");
	}

	err = ei_wrapper_start_prediction(window_shift, frame_shift);
	zassert_ok(err, "Cannot start prediction");

	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_ok(err, "Cannot take semaphore");
}

static void result_ready_cb(int err)
{
	zassert_ok(err, "Callback returned error");

	verify_result(prediction_idx);
	prediction_idx++;

	if (atomic_clear(&rerun_in_cb)) {
		err = ei_wrapper_start_prediction(EI_TEST_WINDOW_SHIFT_CB, 0);
		zassert_ok(err, "Cannot start prediction");
	} else {
		k_sem_give(&test_sem);
	}
}

static void test_init(void)
{
	zassert_equal(ei_wrapper_get_frame_size(), EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME,
		     "Wrong frame size");
	zassert_equal(ei_wrapper_get_window_size(), EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
		      "Wrong window size");
	zassert_true(ei_wrapper_get_window_size() % ei_wrapper_get_frame_size() == 0,
		     "Wrong window and frame size combination");
	zassert_true(ei_wrapper_classifier_has_anomaly(), "Mocked library supports anomaly");

	int err = ei_wrapper_init(result_ready_cb);

	zassert_ok(err, "Initialization failed");

	err = ei_wrapper_init(result_ready_cb);
	zassert_true(err, "Double initialization should not be allowed");
}

static void test_basic(void)
{
	run_basic_setup(prediction_idx, 1, 0, 0);
}

static void test_run_from_cb(void)
{
	int err;

	err = add_input_data(prediction_idx, 0);
	zassert_ok(err, "Cannot add input data");

	for (size_t i = 0; i < EI_TEST_WINDOW_SHIFT_CB; i++) {
		err = add_input_data(prediction_idx + 1, 0);
		zassert_ok(err, "Cannot add input data");
	}

	atomic_set(&rerun_in_cb, true);
	err = ei_wrapper_start_prediction(0, 0);
	zassert_ok(err, "Cannot start prediction");

	/* Processing is called twice. */
	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_ok(err, "Cannot take semaphore");
}

static void test_result_read_fail(void)
{
	int err;
	const char *label;
	float value;
	float anomaly;

	int dsp_time;
	int classification_time;
	int anomaly_time;

	/* Results cannot be read outside of ei_wrapper callback context. */
	err = ei_wrapper_get_classification_results(&label, &value, &anomaly);
	zassert_true(err, "No error for ei_wrapper_get_classification_results");
	err = ei_wrapper_get_timing(&dsp_time, &classification_time, &anomaly_time);
	zassert_true(err, "No error for ei_wrapper_get_timing");
}

static void test_data_add_fail(void)
{
	zassert_false(((EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 1) %
		       EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) == 0,
		      "Wrong value of EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME");

	float data_buf[EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 1] = {0.0};
	int err = ei_wrapper_add_data(data_buf, EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 1);

	zassert_true(err, "Expected error adding data with improper size");
}

static void test_double_start(void)
{
	int err;

	err = add_input_data(prediction_idx, 0);
	zassert_ok(err, "Cannot add input data");

	int key = irq_lock();

	err = ei_wrapper_start_prediction(0, 0);
	zassert_ok(err, "Cannot start prediction");
	err = ei_wrapper_start_prediction(0, 0);
	zassert_true(err, "Prediction started twice");

	irq_unlock(key);

	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_ok(err, "Cannot take semaphore");
}


static void test_overflow(void)
{
	int err = 0;
	size_t counter = 0;
	size_t counter_max = (CONFIG_EI_WRAPPER_DATA_BUF_SIZE / EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);

	while (true) {
		err = add_input_data(prediction_idx, 0);
		if (counter < counter_max)
			zassert_ok(err, "Cannot add input data");
		else {
			zassert_true(err, "Expected add input data error");
			break;
		}
		counter++;
	}
}

static void test_cancel(void)
{
	int err;

	err = ei_wrapper_start_prediction(0, 1);
	zassert_ok(err, "Cannot start prediction");
	err = add_input_data(prediction_idx, 0);
	zassert_ok(err, "Cannot add input data");

	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_true(err, "Unexpected prediction end");

	bool cancelled;

	err = ei_wrapper_clear_data(&cancelled);
	zassert_true(cancelled, "Prediciton was cancelled");
	zassert_ok(err, "Cannot clear data");
}

static void test_loop(void)
{
	const static size_t loop_cnt = 100;

	for (size_t i = 0; i < loop_cnt; i++) {
		size_t window_shift = (i == 0) ? (0) : (1);

		run_basic_setup(prediction_idx, 1, window_shift, 0);
	}

	for (size_t i = 0; i < loop_cnt; i++) {
		size_t frame_shift = ei_wrapper_get_window_size() / ei_wrapper_get_frame_size();

		run_basic_setup(prediction_idx, 1, 0, frame_shift);
	}

	for (size_t i = 0; i < loop_cnt; i++) {
		size_t window_shift = 1;
		size_t frame_shift = ei_wrapper_get_window_size() / ei_wrapper_get_frame_size();

		run_basic_setup(prediction_idx, 2, window_shift, frame_shift);
	}
}

static void test_sliding_window(void)
{
	const static size_t frame_surplus = 100;
	const static size_t loop_cnt = frame_surplus + 1;
	int err;

	err = add_input_data(prediction_idx, frame_surplus);
	zassert_ok(err, "Cannot add input data");

	for (size_t i = 0; i < loop_cnt; i++) {
		size_t frame_shift = (i == 0) ? (0) : (1);
		/* Make sure that input data will be proper. */
		zassert_within((EI_MOCK_GEN_FIRST_INPUT(prediction_idx) +
				EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME),
			       EI_MOCK_GEN_FIRST_INPUT(prediction_idx + 1),
			       FLOAT_CMP_EPSILON,
			       "Wrong input values for sliding window test");

		err = ei_wrapper_start_prediction(0, frame_shift);
		zassert_ok(err, "Cannot start prediction");
		err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
		zassert_ok(err, "Cannot take semaphore");
	}
}

static void test_data_after_start(void)
{
	static const size_t loop_cnt = 10;
	static const size_t window_shift = 2;
	int err;

	err = add_input_data(prediction_idx, 0);
	zassert_ok(err, "Cannot add input data");
	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_true(err, "Expected semaphore timeout");

	for (size_t i = 0; i < loop_cnt; i++) {
		err = ei_wrapper_start_prediction(window_shift, 0);
		zassert_ok(err, "Cannot start prediction");
		err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
		zassert_true(err, "Expected semaphore timeout");

		for (size_t j = 0; j < window_shift - 1; j++) {
			err = add_input_data(prediction_idx, 0);
			zassert_ok(err, "Cannot add input data");
			err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
			zassert_true(err, "Expected semaphore timeout");
		}

		err = add_input_data(prediction_idx, 0);
		zassert_ok(err, "Cannot add input data");
		err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
		zassert_ok(err, "Cannot take semaphore");
	}
}

static void test_thread_fn(void)
{
	static const size_t sleep_ms = 10;
	int err;

	for (size_t i = 0; i <= EI_TEST_THREAD_WINDOW_SHIFT; i++) {
		err = add_input_data(prediction_idx, 0);
		zassert_ok(err, "Cannot add input data");

		k_sleep(K_MSEC(sleep_ms));
	}
}

static void test_data_thread(void)
{
	static const size_t thread_stack_size = 1000;
	static struct k_thread thread;
	int err;

	static K_THREAD_STACK_DEFINE(thread_stack, thread_stack_size);

	k_thread_create(&thread, thread_stack, thread_stack_size,
			(k_thread_entry_t)test_thread_fn,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&thread, "test_thread");

	err = ei_wrapper_start_prediction(EI_TEST_THREAD_WINDOW_SHIFT, 0);
	zassert_ok(err, "Cannot start prediction");
	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_ok(err, "Cannot take semaphore");
}

void timer_fn(struct k_timer *timer)
{
	static size_t calls = 0;
	int err;

	err = add_input_data(prediction_idx, 0);
	zassert_ok(err, "Cannot add input data");

	calls++;

	if (calls > EI_TEST_ISR_WINDOW_SHIFT) {
		k_timer_stop(timer);
	}
}

static void test_data_isr(void)
{
	static const size_t period_ms = 10;
	static K_TIMER_DEFINE(test_timer, timer_fn, NULL);
	int err;

	err = ei_wrapper_start_prediction(EI_TEST_ISR_WINDOW_SHIFT, 0);
	zassert_ok(err, "Cannot start prediction");

	k_timer_start(&test_timer, K_MSEC(period_ms), K_MSEC(period_ms));

	err = k_sem_take(&test_sem, EI_TEST_SEM_TIMEOUT);
	zassert_ok(err, "Cannot take semaphore");
}

static void setup_fn(void)
{
	bool cancelled;
	int err = ei_wrapper_clear_data(&cancelled);

	zassert_false(cancelled, "Prediction was not cancelled");
	zassert_ok(err, "Cannot clear data");
}

static void teardown_fn(void)
{

}

void test_main(void)
{
	ztest_test_suite(test_ei_wrapper,
		ztest_unit_test(test_init),
		ztest_unit_test_setup_teardown(test_basic, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_run_from_cb, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_result_read_fail, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_data_add_fail, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_double_start, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_overflow, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_cancel, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_loop, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_sliding_window, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_data_after_start, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_data_thread, setup_fn, teardown_fn),
		ztest_unit_test_setup_teardown(test_data_isr, setup_fn, teardown_fn)
	);

	ztest_run_test_suite(test_ei_wrapper);
}
