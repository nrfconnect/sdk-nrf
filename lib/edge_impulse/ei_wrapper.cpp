/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <ei_run_classifier.h>
#include <ei_wrapper.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ei_wrapper, CONFIG_EI_WRAPPER_LOG_LEVEL);

#define INPUT_FRAME_SIZE	EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME
#define INPUT_WINDOW_SIZE	EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define HAS_ANOMALY		EI_CLASSIFIER_HAS_ANOMALY
#define RESULT_LABEL_COUNT	EI_CLASSIFIER_LABEL_COUNT

BUILD_ASSERT(CONFIG_EI_WRAPPER_THREAD_STACK_SIZE > 0);

#define DATA_BUFFER_SIZE	CONFIG_EI_WRAPPER_DATA_BUF_SIZE
#define THREAD_STACK_SIZE	CONFIG_EI_WRAPPER_THREAD_STACK_SIZE
#define THREAD_PRIORITY 	CONFIG_EI_WRAPPER_THREAD_PRIORITY
#define DEBUG_MODE		IS_ENABLED(CONFIG_EI_WRAPPER_DEBUG_MODE)

enum state {
	STATE_DISABLED,
	STATE_PROCESSING,
	STATE_READY,
};

struct data_buffer {
	float buf[DATA_BUFFER_SIZE];
	size_t process_idx;
	size_t append_idx;
	size_t wait_data_size;
	struct k_spinlock lock;
	enum state state;
};

static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;
static k_tid_t ei_thread_id;

static K_SEM_DEFINE(ei_sem, 0, 1);

static struct data_buffer ei_input;
static ei_impulse_result_t ei_result;
static ei_wrapper_result_ready_cb user_cb;


BUILD_ASSERT(DATA_BUFFER_SIZE > INPUT_WINDOW_SIZE);
BUILD_ASSERT(INPUT_WINDOW_SIZE % INPUT_FRAME_SIZE == 0);


static size_t buf_get_collected_data_count(const struct data_buffer *b)
{
	if (b->append_idx >= b->process_idx) {
		return b->append_idx - b->process_idx;
	}

	return (ARRAY_SIZE(b->buf) - b->process_idx) + b->append_idx;
}

static size_t buf_calc_free_space(const struct data_buffer *b)
{
	if (b->wait_data_size > 0) {
		return b->wait_data_size + ARRAY_SIZE(b->buf) -
		       INPUT_WINDOW_SIZE - 1;
	}

	return ARRAY_SIZE(b->buf) - buf_get_collected_data_count(b) - 1;
}

static void buf_processing_end(struct data_buffer *b)
{
	k_spinlock_key_t key = k_spin_lock(&b->lock);

	__ASSERT_NO_MSG(b->state == STATE_PROCESSING);
	b->state = STATE_READY;

	k_spin_unlock(&b->lock, key);
}

static int buf_cleanup(struct data_buffer *b)
{
	int err = 0;

	k_spinlock_key_t key = k_spin_lock(&b->lock);

	if (b->state == STATE_PROCESSING) {
		err = -EBUSY;
	} else {
		b->process_idx = 0;
		b->append_idx = 0;
		b->wait_data_size = 0;
		b->state = STATE_READY;
	}

	k_spin_unlock(&b->lock, key);

	return err;
}

static int buf_append(struct data_buffer *b, const float *data, size_t len,
		      bool *process_buf)
{
	*process_buf = false;

	k_spinlock_key_t key = k_spin_lock(&b->lock);

	if (buf_calc_free_space(b) < len) {
		k_spin_unlock(&b->lock, key);
		return -ENOMEM;
	}

	size_t cur_idx = b->append_idx;
	size_t new_idx = b->append_idx + len;
	bool looped = false;

	if (b->wait_data_size > 0) {
		if (b->wait_data_size > len) {
			b->wait_data_size -= len;
		} else {
			b->wait_data_size = 0;
			*process_buf = true;
		}
	}

	if (new_idx >= ARRAY_SIZE(b->buf)) {
		new_idx -= ARRAY_SIZE(b->buf);
		looped = true;
	}

	b->append_idx = new_idx;

	k_spin_unlock(&b->lock, key);

	if (looped) {
		size_t copy_cnt = ARRAY_SIZE(b->buf) - cur_idx;

		memcpy(&b->buf[cur_idx], data, copy_cnt * sizeof(b->buf[0]));
		memcpy(&b->buf[0], data + copy_cnt,
		      (len - copy_cnt) * sizeof(b->buf[0]));
	} else {
		memcpy(&b->buf[cur_idx], data, len * sizeof(b->buf[0]));
	}

	return 0;
}

static void buf_get(const struct data_buffer *b, float *b_res, size_t offset,
		    size_t len)
{
	__ASSERT_NO_MSG((offset + len) <= INPUT_WINDOW_SIZE);

	/* Processing index cannot change while processing is done. */
	__ASSERT_NO_MSG(b->state == STATE_PROCESSING);

	size_t read_start = b->process_idx + offset;
	size_t read_end = read_start + len;

	if ((read_end > ARRAY_SIZE(b->buf)) && (read_start < ARRAY_SIZE(b->buf))) {
		size_t copy_cnt = ARRAY_SIZE(b->buf) - read_start;

		memcpy(b_res, &b->buf[read_start],
		       copy_cnt * sizeof(b->buf[0]));
		memcpy(b_res + copy_cnt, &b->buf[0],
		       (len - copy_cnt) * sizeof(b->buf[0]));
	} else {
		if (read_start >= ARRAY_SIZE(b->buf)) {
			read_start -= ARRAY_SIZE(b->buf);
		}
		memcpy(b_res, &b->buf[read_start], len * sizeof(b->buf[0]));
	}
}

static int buf_processing_move(struct data_buffer *b, size_t move,
			       bool *process_buf)
{
	*process_buf = false;

	k_spinlock_key_t key = k_spin_lock(&b->lock);

	if (b->state == STATE_READY) {
		b->state = STATE_PROCESSING;
	} else {
		__ASSERT_NO_MSG(b->state != STATE_DISABLED);
		k_spin_unlock(&b->lock, key);
		return -EBUSY;
	}

	size_t max_move = buf_get_collected_data_count(b);

	b->process_idx += move;
	if (b->process_idx >= ARRAY_SIZE(b->buf)) {
		b->process_idx -= ARRAY_SIZE(b->buf);
	}

	size_t processing_end_move = move + INPUT_WINDOW_SIZE;

	if (processing_end_move > max_move) {
		b->wait_data_size = processing_end_move - max_move;
	} else {
		*process_buf = true;
	}

	k_spin_unlock(&b->lock, key);

	return 0;
}

bool ei_wrapper_classifier_has_anomaly(void)
{
	return (HAS_ANOMALY) ? (true) : (false);
}

size_t ei_wrapper_get_frame_size(void)
{
	return INPUT_FRAME_SIZE;
}

size_t ei_wrapper_get_window_size(void)
{
	return INPUT_WINDOW_SIZE;
}

int ei_wrapper_add_data(const float *data, size_t data_size)
{
	if (data_size % INPUT_FRAME_SIZE) {
		return -EINVAL;
	}

	bool process_buf;
	int err = buf_append(&ei_input, data, data_size, &process_buf);

	if (!err && process_buf) {
		k_sem_give(&ei_sem);
	}

	return err;
}

int ei_wrapper_clear_data(void)
{
	return buf_cleanup(&ei_input);
}

int ei_wrapper_start_prediction(size_t window_shift, size_t frame_shift)
{
	size_t sample_shift = window_shift * ei_wrapper_get_window_size() +
			      frame_shift * ei_wrapper_get_frame_size();

	bool process_buf;
	int err = buf_processing_move(&ei_input, sample_shift, &process_buf);

	if (!err && process_buf) {
		k_sem_give(&ei_sem);
	}

	return err;
}

static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr)
{
	buf_get(&ei_input, out_ptr, offset, length);

	return 0;
}

static void processing_finished(int err)
{
	__ASSERT_NO_MSG(user_cb);

	buf_processing_end(&ei_input);
	user_cb(err);
}

static void edge_impulse_thread_fn(void)
{
	signal_t features_signal;

	while (true) {
		k_sem_take(&ei_sem, K_FOREVER);

		features_signal.get_data = &raw_feature_get_data;
		features_signal.total_length = INPUT_WINDOW_SIZE;

		/* Invoke the impulse. */
		EI_IMPULSE_ERROR err = run_classifier(&features_signal,
						      &ei_result, DEBUG_MODE);

		if (err) {
			LOG_ERR("run_classifier err=%d", err);
		}

		processing_finished(err);
	}
}

static bool can_read_result(void)
{
	/* User is allowed to access results only from the result ready callback. */
	return (k_current_get() == ei_thread_id);
}

int ei_wrapper_get_classification_results(const char **label, float *value,
					  float *anomaly)
{
	if (!can_read_result()) {
		LOG_WRN("Result can be read only from callback context");
		return -EACCES;
	}

	float max_val = ei_result.classification[0].value;
	size_t max_idx = 0;

	for (size_t idx = 1; idx < RESULT_LABEL_COUNT; idx++) {
		if (max_val < ei_result.classification[idx].value) {
			max_idx = idx;
			max_val = ei_result.classification[idx].value;
		}
	}

	*label = ei_result.classification[max_idx].label;
	*value = ei_result.classification[max_idx].value;
	if (HAS_ANOMALY) {
		*anomaly = ei_result.anomaly;
	} else {
		*anomaly = 0.0;
	}

	return 0;
}

int ei_wrapper_get_timing(int *dsp_time, int *classification_time,
			  int *anomaly_time)
{
	if (!can_read_result()) {
		LOG_WRN("Result can be read only from callback context");
		return -EACCES;
	}

	*dsp_time = ei_result.timing.dsp;
	*classification_time = ei_result.timing.classification;

	if (HAS_ANOMALY) {
		*anomaly_time = ei_result.timing.anomaly;
	} else {
		*anomaly_time = -1;
	}

	return 0;
}

int ei_wrapper_init(ei_wrapper_result_ready_cb cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (user_cb) {
		return -EALREADY;
	}

	user_cb = cb;

	int err = buf_cleanup(&ei_input);

	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);

	ei_thread_id = k_thread_create(&thread, thread_stack, THREAD_STACK_SIZE,
				       (k_thread_entry_t)edge_impulse_thread_fn,
				       NULL, NULL, NULL,
				       THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread, "edge_impulse_thread");

	return 0;
}
