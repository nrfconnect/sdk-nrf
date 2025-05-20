/*
 * Copyright (c) 2019 - 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "bh1749.h"

LOG_MODULE_REGISTER(BH1749, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT rohm_bh1749

static const uint32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_RESET_CHECK] = 2,
	[ASYNC_INIT_RGB_ENABLE] = 0,
	[ASYNC_INIT_STEP_CONFIGURE] = 0
};

static int bh1749_async_init_reset_check(const struct device *dev);
static int bh1749_async_init_rgb_enable(const struct device *dev);
static int bh1749_async_init_configure(const struct device *dev);

static int(*const async_init_fn[ASYNC_INIT_STEP_COUNT])(const struct device *) = {
	[ASYNC_INIT_STEP_RESET_CHECK] = bh1749_async_init_reset_check,
	[ASYNC_INIT_RGB_ENABLE] = bh1749_async_init_rgb_enable,
	[ASYNC_INIT_STEP_CONFIGURE] = bh1749_async_init_configure,
};

static int bh1749_sample_fetch(const struct device *dev, enum sensor_channel chan)

{
	struct bh1749_data *data = dev->data;
	const struct bh1749_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	uint8_t status;
	int err;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	if (!bh1749_is_ready(dev)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	LOG_DBG("Fetching sample...\n");

	err = i2c_reg_read_byte_dt(i2c, BH1749_MODE_CONTROL2, &status);
	if (err < 0) {
		LOG_ERR("Could not read status register CONTROL2");
		return err;
	}

	LOG_DBG("MODE_CONTROL_2 %x", status);

	if ((status & (BH1749_MODE_CONTROL2_VALID_Msk)) == 0) {
		LOG_ERR("No valid data to fetch.");
		return -EIO;
	}

	err = i2c_burst_read_dt(i2c, BH1749_RED_DATA_LSB,
				(uint8_t *)data->sample_rgb_ir,
				BH1749_SAMPLES_TO_FETCH * sizeof(uint16_t));
	if (err < 0) {
		LOG_ERR("Could not read sensor samples");
		return err;
	}

	if (IS_ENABLED(CONFIG_BH1749_TRIGGER)) {
		/* Clearing interrupt by reading INTERRUPT register */
		uint8_t dummy;

		err = i2c_reg_read_byte_dt(i2c, BH1749_INTERRUPT, &dummy);
		if (err < 0) {
			LOG_ERR("Could not disable sensor interrupt.");
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->int_gpio,
						      GPIO_INT_LEVEL_LOW);
		if (err < 0) {
			LOG_ERR("Could not enable pin callback");
			return err;
		}
	}

	return 0;
}

static int bh1749_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1749_data *data = dev->data;

	if (!bh1749_is_ready(dev)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	switch (chan) {
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_ir[BH1749_SAMPLE_POS_RED]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_ir[BH1749_SAMPLE_POS_GREEN]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_ir[BH1749_SAMPLE_POS_BLUE]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_IR:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_ir[BH1749_SAMPLE_POS_IR]);
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bh1749_check(const struct i2c_dt_spec *i2c)
{
	uint8_t manufacturer_id;
	int err;

	err = i2c_reg_read_byte_dt(i2c, BH1749_MANUFACTURER_ID, &manufacturer_id);
	if (err < 0) {
		LOG_ERR("Failed when reading manufacturer ID: %d", err);
		return err;
	}

	LOG_DBG("Manufacturer ID: 0x%02x", manufacturer_id);

	if (manufacturer_id != BH1749_MANUFACTURER_ID_DEFAULT) {
		LOG_ERR("Invalid manufacturer ID: 0x%02x", manufacturer_id);
		return -EIO;
	}

	uint8_t part_id;

	err = i2c_reg_read_byte_dt(i2c, BH1749_SYSTEM_CONTROL, &part_id);
	if (err < 0) {
		LOG_ERR("Failed when reading part ID: %d", err);
		return err;
	}

	if ((part_id & BH1749_SYSTEM_CONTROL_PART_ID_Msk) !=
	    BH1749_SYSTEM_CONTROL_PART_ID) {
		LOG_ERR("Invalid part ID: 0x%02x", part_id);
		return -EIO;
	}

	LOG_DBG("Part ID: 0x%02x", part_id);

	return 0;
}

static int bh1749_sw_reset(const struct i2c_dt_spec *i2c)
{
	return i2c_reg_update_byte_dt(i2c, BH1749_SYSTEM_CONTROL,
				      BH1749_SYSTEM_CONTROL_SW_RESET_Msk,
				      BH1749_SYSTEM_CONTROL_SW_RESET);
}

static int bh1749_rgb_measurement_enable(const struct i2c_dt_spec *i2c, bool enable)
{
	uint8_t en = enable ?
		  BH1749_MODE_CONTROL2_RGB_EN_ENABLE :
		  BH1749_MODE_CONTROL2_RGB_EN_DISABLE;

	return i2c_reg_update_byte_dt(i2c, BH1749_MODE_CONTROL2,
				      BH1749_MODE_CONTROL2_RGB_EN_Msk,
				      en);
}

static void bh1749_async_init(struct k_work *work)
{
	struct k_work_delayable *init_work = k_work_delayable_from_work(work);
	struct bh1749_data *data = CONTAINER_OF(init_work,
						struct bh1749_data,
						init_work);

	LOG_DBG("BH1749 async init step %d", data->async_init_step);

	data->err = async_init_fn[data->async_init_step](data->dev);

	if (data->err) {
		LOG_ERR("BH1749 initialization failed");
	} else {
		data->async_init_step++;

		if (data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			atomic_set(&data->ready, 1);
			LOG_INF("BH1749 initialized");
		} else {
			k_work_schedule(init_work,
					K_MSEC(async_init_delay[data->async_init_step]));
		}
	}
}

static int bh1749_async_init_reset_check(const struct device *dev)
{
	struct bh1749_data *data = dev->data;
	const struct bh1749_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	int err;

	(void)memset(data->sample_rgb_ir, 0, sizeof(data->sample_rgb_ir));

	err = bh1749_sw_reset(i2c);
	if (err < 0) {
		LOG_ERR("Could not apply software reset");
		return err;
	}

	return bh1749_check(i2c);
}

static int bh1749_async_init_rgb_enable(const struct device *dev)
{
	const struct bh1749_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	int err;

	err = bh1749_rgb_measurement_enable(i2c, true);
	if (err < 0) {
		LOG_ERR("Could not set measurement mode.");
	}
	return err;
}

static int bh1749_async_init_configure(const struct device *dev)
{
	const struct bh1749_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	int err;

	err = i2c_reg_write_byte_dt(i2c, BH1749_MODE_CONTROL1,
				    BH1749_MODE_CONTROL1_DEFAULTS);
	if (err < 0) {
		LOG_ERR("Could not set gain and measurement mode configuration.");
		return err;
	}

#ifdef CONFIG_BH1749_TRIGGER
	err = bh1749_gpio_interrupt_init(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize interrupt with error %d", err);
		return err;
	}

	LOG_DBG("GPIO Sense Interrupts initialized");
#endif /* CONFIG_BH1749_TRIGGER */

	return 0;
}

static int bh1749_init(const struct device *dev)
{
	struct bh1749_data *data = dev->data;
	const struct bh1749_config *config = dev->config;
	const struct device *bus = config->i2c.bus;

	if (!device_is_ready(bus)) {
		LOG_ERR("%s: bus device %s is not ready", dev->name, bus->name);
		return -ENODEV;
	}
	data->dev = dev;
	k_work_init_delayable(&data->init_work, bh1749_async_init);
	k_work_schedule(&data->init_work, K_MSEC(async_init_delay[data->async_init_step]));
	return 0;
};

static const struct sensor_driver_api bh1749_driver_api = {
	.sample_fetch = &bh1749_sample_fetch,
	.channel_get = &bh1749_channel_get,
#ifdef CONFIG_BH1749_TRIGGER
	.attr_set = &bh1749_attr_set,
	.trigger_set = &bh1749_trigger_set,
#endif
};

#define BH1749_DEFINE(inst)						\
	static struct bh1749_data bh1749_data_##inst;			\
	static const struct bh1749_config bh1749_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),	\
	};								\
	DEVICE_DT_INST_DEFINE(inst, bh1749_init, NULL,			\
			      &bh1749_data_##inst,			\
			      &bh1749_config_##inst,			\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
			      &bh1749_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BH1749_DEFINE)
