/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file light_sensor.h
 * @defgroup light_sensors Light sensor interface
 * @{
 * @brief Module for interfacing light sensor for asset tracker
 *
 * @details Interface for RGB IR light sensor for asset tracker.
 *
 */
#include <zephyr.h>
#include <drivers/sensor.h>

struct light_sensor_data {
	/* light levels in lux */
	s32_t red;
	s32_t green;
	s32_t blue;
	s32_t ir;
};

typedef void (*light_sensor_data_ready_cb)(void);

/**
 * @brief Initialize and start sampling the light sensor.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int light_sensor_init_and_start(struct k_work_q *work_q,
				const light_sensor_data_ready_cb cb);

/**
 * @brief Get latest sampled light data.
 *
 * @param data Pointer to memory to store latest light sensor data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int light_sensor_get_data(struct light_sensor_data *const data);

/**
 * @brief Set light sensor's poll/send interval.
 *
 * @param interval_s Interval, in seconds. 0 to disable.
 *
 */
void light_sensor_set_send_interval(const u32_t interval_s);

/**
 * @brief Get light sensor's poll/send interval.
 *
 * @return Interval, in seconds.
 */
u32_t light_sensor_get_send_interval(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* LIGHT_SENSOR_H_ */
