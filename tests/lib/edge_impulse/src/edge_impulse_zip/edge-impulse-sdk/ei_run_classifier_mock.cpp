/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <ei_run_classifier.h>

static size_t prediction_idx;

void ei_run_classifier_mock_init(void)
{
	prediction_idx = 0;
}

/* Input data must be ascending sequence of floats. Difference between
 * subsequent elements of input sequence equals 1. The first element
 * has value defined by ei_test_params.h (depends on current prediction idx).
 */
static void verify_data_read(signal_t *signal, const size_t prediction_idx,
			     const size_t chunk_size)
{
	size_t data_size = signal->total_length;

	zassert_true(data_size % chunk_size == 0,
		     "Improper data and chunk size combination");

	static float data_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
	float *data_ptr = data_buf;

	memset(data_buf, 0, sizeof(data_buf));

	for (size_t off = 0; off < data_size; off += chunk_size) {
		int err = signal->get_data(off, chunk_size, data_ptr + off);

		zassert_ok(err, "get_data returned an error");
	}

	float value = EI_MOCK_GEN_FIRST_INPUT(prediction_idx);;

	for (size_t off = 0; off < data_size; off++) {
		zassert_within(data_buf[off], value, FLOAT_CMP_EPSILON,
			       "Input data error");
		value++;
	}
}

EI_IMPULSE_ERROR run_classifier(signal_t *signal,
				ei_impulse_result_t *result,
				bool debug)
{
	ARG_UNUSED(debug);

	/* Test getting data. */
	verify_data_read(signal, prediction_idx, 1);
	verify_data_read(signal, prediction_idx,
			 EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
	verify_data_read(signal, prediction_idx,
			 EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);

	/* Busy wait for predefined amount of time to simulate calculations. */
	k_busy_wait(EI_MOCK_BUSY_WAIT_TIME);

	/* Timing results. */
	result->timing.dsp = EI_MOCK_GEN_DSP_TIME(prediction_idx);
	result->timing.classification = EI_MOCK_GEN_CLASSIFICATION_TIME(prediction_idx);
	result->timing.anomaly = EI_MOCK_GEN_ANOMALY_TIME(prediction_idx);

	/* Classification results. */
	result->anomaly = EI_MOCK_GEN_ANOMALY(prediction_idx);

	size_t res_idx = EI_MOCK_GEN_LABEL_IDX(prediction_idx);
	const float value_selected = EI_MOCK_GEN_VALUE(prediction_idx);
	const float value_others = EI_MOCK_GEN_VALUE_OTHERS(prediction_idx);

	zassert_true(value_selected < 1.0, "Wrong value of selected label.");
	zassert_true(value_selected > value_others, "Wrong values");

	for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
		result->classification[i].label = ei_classifier_inferencing_categories[i];
		result->classification[i].value =
			(i == res_idx) ? (value_selected) : (value_others);
	}

	zassert_false(strcmp(EI_MOCK_GEN_LABEL(prediction_idx),
		      ei_classifier_inferencing_categories[res_idx]),
		      "Wrong label");

	prediction_idx++;

	return EI_IMPULSE_OK;
}
