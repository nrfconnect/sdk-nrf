/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <net/lwm2m.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_temp, CONFIG_APP_LOG_LEVEL);

/* use 25.5C if no sensor available */
static struct float32_value temp_float = { 25, 500000 };
static struct device *die_dev;
static s32_t timestamp;

#if defined(CONFIG_TEMP_NRF5_NAME)
static int read_temperature(struct device *temp_dev,
			    struct float32_value *float_val)
{
	const char *name = temp_dev->config->name;
	struct sensor_value temp_val;
	int ret;

	ret = sensor_sample_fetch(temp_dev);
	if (ret) {
		LOG_ERR("%s: I/O error: %d", name, ret);
		return ret;
	}

	ret = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &temp_val);
	if (ret) {
		LOG_ERR("%s: can't get data: %d", name, ret);
		return ret;
	}

	LOG_DBG("%s: read %d.%d C", name, temp_val.val1, temp_val.val2);
	float_val->val1 = temp_val.val1;
	float_val->val2 = temp_val.val2;

	return 0;
}
#endif

static void *temp_read_cb(u16_t obj_inst_id, u16_t res_id, u16_t res_inst_id,
			  size_t *data_len)
{
	s32_t ts;

	/* Only object instance 0 is currently used */
	if (obj_inst_id != 0) {
		*data_len = 0;
		return NULL;
	}

#if defined(CONFIG_TEMP_NRF5_NAME)
	/*
	 * No need to check if read was successful, just reuse the
	 * previous value which is already stored at temp_float.
	 * This is because there is currently no way to report read_cb
	 * failures to the LWM2M engine.
	 */
	read_temperature(die_dev, &temp_float);
#endif
	lwm2m_engine_set_float32("3303/0/5700", &temp_float);
	*data_len = sizeof(temp_float);
	/* get current time from device */
	lwm2m_engine_get_s32("3/0/13", &ts);
	/* set timestamp */
	lwm2m_engine_set_s32("3303/0/5518", ts);

	return &temp_float;
}

int lwm2m_init_temp(void)
{
#if defined(CONFIG_TEMP_NRF5_NAME)
	die_dev = device_get_binding(CONFIG_TEMP_NRF5_NAME);
	LOG_INF("%s on-die temperature sensor %s",
		die_dev ? "Found" : "Did not find", CONFIG_TEMP_NRF5_NAME);
#endif

	if (!die_dev) {
		LOG_ERR("No temperature device found.");
	}

	lwm2m_engine_create_obj_inst("3303/0");
	lwm2m_engine_register_read_callback("3303/0/5700", temp_read_cb);
	lwm2m_engine_set_res_data("3303/0/5518",
				  &timestamp, sizeof(timestamp), 0);
	return 0;
}
