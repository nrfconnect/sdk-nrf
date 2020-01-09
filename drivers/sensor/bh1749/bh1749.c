/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <logging/log.h>

#include "bh1749.h"

static struct k_delayed_work bh1749_init_work;
static struct device *bh1749_dev;

LOG_MODULE_REGISTER(BH1749, CONFIG_SENSOR_LOG_LEVEL);

static const s32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_RESET_CHECK] = 2,
	[ASYNC_INIT_RGB_ENABLE] = 0,
	[ASYNC_INIT_STEP_CONFIGURE] = 0
};

static int bh1749_async_init_reset_check(struct bh1749_data *data);
static int bh1749_async_init_rgb_enable(struct bh1749_data *data);
static int bh1749_async_init_configure(struct bh1749_data *data);

static int(*const async_init_fn[ASYNC_INIT_STEP_COUNT])(
	struct bh1749_data *data) = {
	[ASYNC_INIT_STEP_RESET_CHECK] = bh1749_async_init_reset_check,
	[ASYNC_INIT_RGB_ENABLE] = bh1749_async_init_rgb_enable,
	[ASYNC_INIT_STEP_CONFIGURE] = bh1749_async_init_configure,
};

static int bh1749_sample_fetch(struct device *dev, enum sensor_channel chan)

{
	struct bh1749_data *data = dev->driver_data;
	u8_t status;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	if (unlikely(!data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	LOG_DBG("Fetching sample...\n");

	if (i2c_reg_read_byte(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			      BH1749_MODE_CONTROL2, &status)) {
		LOG_ERR("Could not read status register CONTROL2");
		return -EIO;
	}

	LOG_DBG("MODE_CONTROL_2 %x", status);

	if ((status & (BH1749_MODE_CONTROL2_VALID_Msk)) == 0) {
		LOG_ERR("No valid data to fetch.");
		return -EIO;
	}

	if (i2c_burst_read(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			   BH1749_RED_DATA_LSB,
			   (u8_t *)data->sample_rgb_ir,
			   BH1749_SAMPLES_TO_FETCH * sizeof(u16_t))) {
		LOG_ERR("Could not read sensor samples");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_BH1749_TRIGGER)) {
		/* Clearing interrupt by reading INTERRUPT register */
		u8_t dummy;

		if (i2c_reg_read_byte(data->i2c,
				      DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				      BH1749_INTERRUPT, &dummy)) {
			LOG_ERR("Could not disable sensor interrupt.");
			return -EIO;
		}

		if (gpio_pin_enable_callback(data->gpio,
					     DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN)) {
			LOG_ERR("Could not enable pin callback");
			return -EIO;
		}
	}

	return 0;
}

static int bh1749_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1749_data *data = dev->driver_data;

	if (unlikely(!data->ready)) {
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

static int bh1749_check(struct bh1749_data *data)
{
	u8_t manufacturer_id;

	if (i2c_reg_read_byte(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			      BH1749_MANUFACTURER_ID, &manufacturer_id)) {
		LOG_ERR("Failed when reading manufacturer ID");
		return -EIO;
	}

	LOG_DBG("Manufacturer ID: 0x%02x", manufacturer_id);

	if (manufacturer_id != BH1749_MANUFACTURER_ID_DEFAULT) {
		LOG_ERR("Invalid manufacturer ID: 0x%02x", manufacturer_id);
		return -EIO;
	}

	u8_t part_id;

	if (i2c_reg_read_byte(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			      BH1749_SYSTEM_CONTROL, &part_id)) {
		LOG_ERR("Failed when reading part ID");
		return -EIO;
	}

	if ((part_id & BH1749_SYSTEM_CONTROL_PART_ID_Msk) !=
	    BH1749_SYSTEM_CONTROL_PART_ID) {
		LOG_ERR("Invalid part ID: 0x%02x", part_id);
		return -EIO;
	}

	LOG_DBG("Part ID: 0x%02x", part_id);

	return 0;
}

static int bh1749_sw_reset(struct device *dev)
{
	return i2c_reg_update_byte(dev, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				   BH1749_SYSTEM_CONTROL,
				   BH1749_SYSTEM_CONTROL_SW_RESET_Msk,
				   BH1749_SYSTEM_CONTROL_SW_RESET);
}

static int bh1749_rgb_measurement_enable(struct bh1749_data *data, bool enable)
{
	u8_t en = enable ?
		  BH1749_MODE_CONTROL2_RGB_EN_ENABLE :
		  BH1749_MODE_CONTROL2_RGB_EN_DISABLE;

	return i2c_reg_update_byte(data->i2c,
				   DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				   BH1749_MODE_CONTROL2,
				   BH1749_MODE_CONTROL2_RGB_EN_Msk,
				   en);
}


static void bh1749_async_init(struct k_work *work)
{
	struct bh1749_data *data = bh1749_dev->driver_data;

	ARG_UNUSED(work);



	LOG_DBG("BH1749 async init step %d", data->async_init_step);

	data->err = async_init_fn[data->async_init_step](data);

	if (data->err) {
		LOG_ERR("BH1749 initialization failed");
	} else {
		data->async_init_step++;

		if (data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			data->ready = true;
			LOG_INF("BH1749 initialized");
		} else {
			k_delayed_work_submit(&bh1749_init_work,
					      async_init_delay[data->async_init_step]);
		}
	}
}

static int bh1749_async_init_reset_check(struct bh1749_data *data)
{
	(void)memset(data->sample_rgb_ir, 0, sizeof(data->sample_rgb_ir));
	int err;

	err = bh1749_sw_reset(data->i2c);

	if (err) {
		LOG_ERR("Could not apply software reset.");
		return -EIO;
	}
	err = bh1749_check(data);
	if (err) {
		LOG_ERR("Communication with BH1749 failed with error %d", err);
		return -EIO;
	}
	return 0;
}

static int bh1749_async_init_rgb_enable(struct bh1749_data *data)
{
	int err;

	err = bh1749_rgb_measurement_enable(data, true);
	if (err) {
		LOG_ERR("Could not set measurement mode.");
		return -EIO;
	}
	return 0;
}

static int bh1749_async_init_configure(struct bh1749_data *data)
{
	if (i2c_reg_write_byte(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			       BH1749_MODE_CONTROL1,
			       BH1749_MODE_CONTROL1_DEFAULTS)) {
		LOG_ERR(
			"Could not set gain and measurement mode configuration.");
		return -EIO;
	}

#ifdef CONFIG_BH1749_TRIGGER
	int err = bh1749_gpio_interrupt_init(bh1749_dev);
	if (err) {
		LOG_ERR("Failed to initialize interrupt with error %d", err);
		return -EIO;
	}

	LOG_DBG("GPIO Sense Interrupts initialized");
#endif /* CONFIG_BH1749_TRIGGER */

	return 0;
}

static int bh1749_init(struct device *dev)
{
	bh1749_dev = dev;

	struct bh1749_data *data = bh1749_dev->driver_data;
	data->i2c = device_get_binding(DT_INST_0_ROHM_BH1749_BUS_NAME);

	if (data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DT_INST_0_ROHM_BH1749_BUS_NAME);

		return -EINVAL;
	}
	k_delayed_work_init(&bh1749_init_work, bh1749_async_init);
	return k_delayed_work_submit(&bh1749_init_work, async_init_delay[data->async_init_step]);
};

static const struct sensor_driver_api bh1749_driver_api = {
	.sample_fetch = &bh1749_sample_fetch,
	.channel_get = &bh1749_channel_get,
#ifdef CONFIG_BH1749_TRIGGER
	.attr_set = &bh1749_attr_set,
	.trigger_set = &bh1749_trigger_set,
#endif
};

static struct bh1749_data bh1749_data;

DEVICE_DEFINE(bh1749, DT_INST_0_ROHM_BH1749_LABEL,
	      bh1749_init, device_pm_control_nop, &bh1749_data, NULL,
	      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bh1749_driver_api);
