/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <net/lwm2m.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_accel, CONFIG_APP_LOG_LEVEL);

#define SENSOR_UNIT_NAME		"Orientation"
#define FLIP_ACCELERATION_THRESHOLD	5.0
#define CALIBRATION_ITERATIONS		CONFIG_ACCEL_CALIBRATION_ITERATIONS
#define MEASUREMENT_ITERATIONS		CONFIG_ACCEL_ITERATIONS
#define ACCEL_INVERTED			CONFIG_ACCEL_INVERTED

#if defined(CONFIG_FLIP_POLL)
#define FLIP_POLL_INTERVAL		K_MSEC(CONFIG_FLIP_POLL_INTERVAL)
#else
#define FLIP_POLL_INTERVAL		K_NO_WAIT
#endif

#ifdef CONFIG_ACCEL_USE_SIM
#define FLIP_INPUT			CONFIG_FLIP_INPUT
#define CALIBRATION_INPUT		-1
#else
#define FLIP_INPUT			-1
#ifdef CONFIG_ACCEL_CALIBRATE
#define CALIBRATION_INPUT		CONFIG_CALIBRATION_INPUT
#else
#define CALIBRATION_INPUT		-1
#endif /* CONFIG_ACCEL_CALIBRATE */
#endif /* CONFIG_ACCEL_USE_SIM */

/**@brief Orientation states. */
enum orientation_state {
	ORIENTATION_NOT_KNOWN,   /**< Initial state. */
	ORIENTATION_NORMAL,      /**< Has normal orientation. */
	ORIENTATION_UPSIDE_DOWN, /**< System is upside down. */
	ORIENTATION_ON_SIDE      /**< System is placed on its side. */
};


/**@brief Struct containing current orientation and 3 axis acceleration data. */
struct orientation_detector_sensor_data {
	double x;			    /**< x-axis acceleration [m/s^2]. */
	double y;			    /**< y-axis acceleration [m/s^2]. */
	double z;			    /**< z-axis acceleration [m/s^2]. */
	enum orientation_state orientation; /**< Current orientation. */
};

static struct device *accel_dev;
static double accel_offset[3];
static struct k_delayed_work flip_poll_work;
static u32_t timestamp;

int orientation_detector_poll(
	struct orientation_detector_sensor_data *sensor_data)
{
	int err;
	u8_t i;
	double aggregated_data[3] = {0};
	struct sensor_value accel_data[3];
	enum orientation_state current_orientation;

	for (i = 0; i < MEASUREMENT_ITERATIONS; i++) {
		err = sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_ACCEL_Z);
		if (err) {
			LOG_ERR("sensor_sample_fetch failed");
			return err;
		}

		err = sensor_channel_get(accel_dev,
				SENSOR_CHAN_ACCEL_Z, &accel_data[2]);
		if (err) {
			LOG_ERR("sensor_channel_get failed");
			return err;
		}

		aggregated_data[2] += sensor_value_to_double(&accel_data[2]);
	}

	sensor_data->z = (aggregated_data[2] / (double)MEASUREMENT_ITERATIONS) -
				accel_offset[2];

	if (sensor_data->z >= FLIP_ACCELERATION_THRESHOLD) {
		if (IS_ENABLED(ACCEL_INVERTED)) {
			current_orientation = ORIENTATION_UPSIDE_DOWN;
		} else {
			current_orientation = ORIENTATION_NORMAL;
		}
	} else if (sensor_data->z <= -FLIP_ACCELERATION_THRESHOLD) {
		if (IS_ENABLED(ACCEL_INVERTED)) {
			current_orientation = ORIENTATION_NORMAL;
		} else {
			current_orientation = ORIENTATION_UPSIDE_DOWN;
		}
	} else {
		current_orientation = ORIENTATION_ON_SIDE;
	}

	sensor_data->orientation = current_orientation;

	return 0;
}

/**@brief Poll flip orientation and update object if flip mode is enabled. */
static void flip_work(struct k_work *work)
{
	s32_t ts;
	static enum orientation_state last_orientation_state =
		ORIENTATION_NOT_KNOWN;
	static struct orientation_detector_sensor_data sensor_data;
	struct float32_value f32;

	if (orientation_detector_poll(&sensor_data) == 0) {
		if (sensor_data.orientation == last_orientation_state) {
			goto exit;
		}

		/* get current time from device */
		lwm2m_engine_get_s32("3/0/13", &ts);

		f32.val1 = 0;
		f32.val2 = 0;
		switch (sensor_data.orientation) {
		case ORIENTATION_NORMAL:
			/* X=0, Y=0, Z=1 */
			LOG_INF("accelerometer normal");
			lwm2m_engine_set_float32("3313/0/5702", &f32);
			lwm2m_engine_set_float32("3313/0/5703", &f32);
			f32.val1 = 1;
			lwm2m_engine_set_float32("3313/0/5704", &f32);
			break;
		case ORIENTATION_UPSIDE_DOWN:
			/* X=0, Y=0, Z=-1 */
			LOG_INF("accelerometer upside down");
			lwm2m_engine_set_float32("3313/0/5702", &f32);
			lwm2m_engine_set_float32("3313/0/5703", &f32);
			f32.val1 = -1;
			lwm2m_engine_set_float32("3313/0/5704", &f32);
			break;
		case ORIENTATION_ON_SIDE:
			/* X=1, Y=0, Z=0 */
			LOG_INF("accelerometer on side");
			lwm2m_engine_set_float32("3313/0/5702", &f32);
			lwm2m_engine_set_float32("3313/0/5704", &f32);
			f32.val1 = 1;
			lwm2m_engine_set_float32("3313/0/5703", &f32);
			break;
		default:
			/* X=0, Y=0, Z=0 */
			LOG_INF("accelerometer in unknown position");
			lwm2m_engine_set_float32("3313/0/5702", &f32);
			lwm2m_engine_set_float32("3313/0/5703", &f32);
			lwm2m_engine_set_float32("3313/0/5704", &f32);
			goto exit;
		}

		/* set timestamp */
		lwm2m_engine_set_s32("3313/0/5518", ts);
		last_orientation_state = sensor_data.orientation;
	}

exit:
	if (work) {
		k_delayed_work_submit(&flip_poll_work, FLIP_POLL_INTERVAL);
	}
}

static int accel_calibrate(void)
{
	u8_t i;
	int err;
	struct sensor_value accel_data[3];
	double aggregated_data[3] = {0};

	for (i = 0; i < CALIBRATION_ITERATIONS; i++) {
		err = sensor_sample_fetch(accel_dev);
		if (err) {
			LOG_INF("sensor_sample_fetch failed");
			return err;
		}

		err = sensor_channel_get(accel_dev,
				SENSOR_CHAN_ACCEL_X, &accel_data[0]);
		err += sensor_channel_get(accel_dev,
				SENSOR_CHAN_ACCEL_Y, &accel_data[1]);
		err += sensor_channel_get(accel_dev,
				SENSOR_CHAN_ACCEL_Z, &accel_data[2]);

		if (err) {
			LOG_ERR("sensor_channel_get failed");
			return err;
		}

		aggregated_data[0] += sensor_value_to_double(&accel_data[0]);
		aggregated_data[1] += sensor_value_to_double(&accel_data[1]);
		aggregated_data[2] +=
			(sensor_value_to_double(&accel_data[2])
			+ ((double)SENSOR_G) / 1000000.0);
	}

	accel_offset[0] = aggregated_data[0] / (double)CALIBRATION_ITERATIONS;
	accel_offset[1] = aggregated_data[1] / (double)CALIBRATION_ITERATIONS;
	accel_offset[2] = aggregated_data[2] / (double)CALIBRATION_ITERATIONS;

	return 0;
}

int handle_accel_events(struct ui_evt *evt)
{
	if (!evt) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_ACCEL_USE_SIM) && (evt->button == FLIP_INPUT)) {
		flip_work(NULL);
		return 0;
	}

	return -ENOENT;
}

int lwm2m_init_accel(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_FLIP_POLL)) {
		k_delayed_work_init(&flip_poll_work, flip_work);
	}

	accel_dev = device_get_binding(CONFIG_ACCEL_DEV_NAME);
	if (accel_dev == NULL) {
		LOG_ERR("Could not get %s device", CONFIG_ACCEL_DEV_NAME);
		return -ENOENT;
	}

	if (IS_ENABLED(CONFIG_ACCEL_CALIBRATE)) {
		ret = accel_calibrate();
		if (ret) {
			LOG_ERR("Could not calibrate accelerometer device: %d",
				ret);
			return ret;
		}
	}

	/* create accel object */
	lwm2m_engine_create_obj_inst("3313/0");
	lwm2m_engine_set_res_data("3313/0/5701",
				  SENSOR_UNIT_NAME, sizeof(SENSOR_UNIT_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3313/0/5518",
				  &timestamp, sizeof(timestamp), 0);

	if (IS_ENABLED(CONFIG_FLIP_POLL)) {
		k_delayed_work_submit(&flip_poll_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_ACCEL_USE_SIM)) {
		flip_work(NULL);
	}

	return 0;
}
