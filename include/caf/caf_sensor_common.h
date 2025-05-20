/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CAF_SENSOR_COMMON_H_
#define _CAF_SENSOR_COMMON_H_

/**
 * @file
 * @defgroup caf_sensor_common CAF Sensor Common
 * @{
 * @brief CAF Sensor Common.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>


/* This value has to be equal to fractional part of the sensor_value. */
#define FLOAT_TO_SENSOR_VAL_CONST 1000000

#define FLOAT_TO_SENSOR_VALUE(float_val)							\
	{											\
		.val1 = (int32_t)(float_val),							\
		.val2 = (int32_t)(((float_val) - (int32_t)(float_val)) *			\
						FLOAT_TO_SENSOR_VAL_CONST),			\
	}

/**
 * @brief Description of single channel
 *
 * The description of the channel to do the measurement on.
 */
struct caf_sampled_channel {
	/** @brief Channel identifier */
	enum sensor_channel chan;
	/** @brief Number of data samples in selected channel */
	uint8_t data_cnt;
};

/**
 * @brief Helper function for checking if one sensor_value is greater than the other.
 *
 * @param sensor_val1 First compered sensor value.
 * @param sensor_val2 Second compered sensor value.
 * @return True if sensor_val1 is greater than sensor_val2, false otherwise.
 */
static inline bool sensor_value_greater_then(struct sensor_value sensor_val1,
					     struct sensor_value sensor_val2)
{
	return ((sensor_val1.val1 > sensor_val2.val1) ||
		((sensor_val1.val1 == sensor_val2.val1) && (sensor_val1.val2 > sensor_val2.val2)));
}

/**
 * @brief Helper function for calculating absolute value of difference of two sensor_values.
 *
 * @param sensor_val1 First sensor value.
 * @param sensor_val2 Second sensor value.
 * @return Absolute value of difference between sensor_val1 and sensor_val2.
 */
static inline struct sensor_value sensor_value_abs_difference(struct sensor_value sensor_val1,
							      struct sensor_value sensor_val2)
{
	struct sensor_value result;

	result.val1 = abs(sensor_val1.val1 - sensor_val2.val1);
	result.val2 = abs(sensor_val1.val2 - sensor_val2.val2);
	if (result.val2 > FLOAT_TO_SENSOR_VAL_CONST) {
		result.val2 = result.val2 - FLOAT_TO_SENSOR_VAL_CONST;
		result.val1++;
	}
	return result;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CAF_SENSOR_COMMON_H_ */
