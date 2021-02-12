/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Edge Impulse wrapper header.
 */

#ifndef _EI_WRAPPER_H_
#define _EI_WRAPPER_H_


/**
 * @defgroup ei_wrapper Edge Impulse wrapper
 * @brief Wrapper that uses Edge Impulse lib to run machine learning on device.
 *
 * @{
 */

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef ei_wrapper_result_ready_cb
 * @brief Callback executed by the wrapper when the result is ready.
 *
 * @param[in] err Zero (if operation was successful) or negative error code.
 */
typedef void (*ei_wrapper_result_ready_cb)(int err);


/** Check if classifier calculates anomaly value.
 *
 * @retval true If the classifier calculates the anomaly value.
 *              Otherwise, false is returned.
 */
bool ei_wrapper_classifier_has_anomaly(void);


/** Get the size of the input frame.
 *
 * @return Size of the input frame, expressed as a number of floating-point
 *         values.
 */
size_t ei_wrapper_get_frame_size(void);


/** Get the size of the input window.
 *
 * @return Size of the input window, expressed as a number of floating-point
 *         values.
 */
size_t ei_wrapper_get_window_size(void);


/** Add input data for the library.
 *
 * Size of the added data must be divisible by input frame size.
 *
 * @param[in] data       Pointer to the buffer with input data.
 * @param[in] data_size  Size of the data (number of floating-point values).
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_add_data(const float *data, size_t data_size);


/** Clear all buffered data.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_clear_data(void);


/** Start a prediction using the Edge Impulse library.
 *
 * If there is not enough data in the input buffer, the prediction start is
 * delayed until the missing data is added.
 *
 * @param[in] window_shift  Number of windows the input window is shifted before
 *                          prediction.
 * @param[in] frame_shift   Number of frames the input window is shifted before
 *                          prediction.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_start_prediction(size_t window_shift, size_t frame_shift);


/** Get classification results.
 *
 * This function can be executed only from the wrapper's callback context.
 * Otherwise it returns a (negative) error code.
 *
 * If calculating anomaly value is not supported, anomaly is set to value
 * of 0.0.
 *
 * @param[out] label   Pointer to the variable that is used to store the pointer
 *                     to the classification label.
 * @param[out] value   Pointer to the variable that is used to store the
 *                     classification value.
 * @param[out] anomaly Pointer to the variable that is used to store
 *                     the anomaly.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_get_classification_results(const char **label, float *value,
					  float *anomaly);


/** Get execution times for operations performed by the library.
 *
 * This function can be executed only from the wrapper's callback context.
 * Otherwise, it returns a (negative) error code.
 *
 * The library uses Zephyr's uptime for calculations. Because of that execution
 * times can be affected by other operations performed by the CPU.
 *
 * If calculating the anomaly value is not supported, anomaly_time is set to
 * the value of -1.
 *
 * @param[out] dsp_time            Pointer to the variable that is used to store
 *                                 the dsp time.
 * @param[out] classification_time Pointer to the variable that is used to store
 *                                 the classification time.
 * @param[out] anomaly_time        Pointer to the variable that is used to store
 *                                 the anomaly time.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_get_timing(int *dsp_time, int *classification_time,
			  int *anomaly_time);


/** Initialize the Edge Impulse wrapper.
 *
 * @param[in] cb Callback used to receive results.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ei_wrapper_init(ei_wrapper_result_ready_cb cb);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EI_WRAPPER_H_ */
