/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include "bh1749.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(BH1749_TRIGGER, CONFIG_SENSOR_LOG_LEVEL);

/* Callback for active sense pin from BH1749 */
static void bh1749_gpio_callback(struct device *dev, struct gpio_callback *cb,
				 u32_t pins)
{
	struct bh1749_data *drv_data =
		CONTAINER_OF(cb, struct bh1749_data, gpio_cb);

	gpio_pin_disable_callback(dev, DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN);

	k_work_submit(&drv_data->work);
}

static void bh1749_work_cb(struct k_work *work)
{
	struct bh1749_data *data = CONTAINER_OF(work,
						struct bh1749_data,
						work);
	struct device *dev = data->dev;

	if (data->trg_handler != NULL) {
		data->trg_handler(dev, &data->trigger);
	}
}

/* Set sensor trigger attributes */
int bh1749_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct bh1749_data *data = dev->driver_data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		if (i2c_reg_write_byte(data->i2c,
				       DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				       BH1749_TH_HIGH_LSB,
				       (u8_t)val->val1)) {
			LOG_ERR("Could not set upper threshold");
			return -EIO;
		}

		if (i2c_reg_write_byte(data->i2c,
				       DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				       BH1749_TH_HIGH_MSB,
				       (u8_t)(val->val1 >> 8))) {
			LOG_ERR("Could not set upper threshold");
			return -EIO;
		}
		return 0;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		if (i2c_reg_write_byte(data->i2c,
				       DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				       BH1749_TH_LOW_LSB,
				       (u8_t)val->val1)) {
			LOG_ERR("Could not set lower threshold");
			return -EIO;
		}

		if (i2c_reg_write_byte(data->i2c,
				       DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				       BH1749_TH_LOW_MSB,
				       (u8_t)(val->val1 >> 8))) {
			LOG_ERR("Could not set lower threshold");
			return -EIO;
		}
		return 0;
	}
	return -ENOTSUP;
}

int bh1749_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bh1749_data *data = dev->driver_data;
	u8_t interrupt_source = 0;

	gpio_pin_disable_callback(data->gpio,
		DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (i2c_reg_update_byte(
			    data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
			    BH1749_PERSISTENCE,
			    BH1749_PERSISTENCE_PERSISTENCE_Msk,
			    BH1749_PERSISTENCE_PERSISTENCE_8_SAMPLES)) {
			LOG_ERR("Unable to set threshold persistence");
			return -EIO;
		}
		if (trig->chan == SENSOR_CHAN_RED) {
			interrupt_source = BH1749_INTERRUPT_INT_SOURCE_RED;
		} else if (trig->chan == SENSOR_CHAN_GREEN) {
			interrupt_source = BH1749_INTERRUPT_INT_SOURCE_GREEN;
		} else if (trig->chan == SENSOR_CHAN_BLUE) {
			interrupt_source = BH1749_INTERRUPT_INT_SOURCE_BLUE;
		} else {
			LOG_ERR("Unsupported interrupt source");
			return -ENOTSUP;
		}
		break;
	case SENSOR_TRIG_DATA_READY:
		if (i2c_reg_update_byte(data->i2c,
				DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				BH1749_PERSISTENCE,
				BH1749_PERSISTENCE_PERSISTENCE_Msk,
				BH1749_PERSISTENCE_PERSISTENCE_ACTIVE_END)) {
			LOG_ERR("Unable to set data ready trigger");
			return -EIO;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (i2c_reg_update_byte(data->i2c, DT_INST_0_ROHM_BH1749_BASE_ADDRESS,
				BH1749_INTERRUPT,
				BH1749_INTERRUPT_ENABLE_Msk |
				BH1749_INTERRUPT_INT_SOURCE_Msk,
				BH1749_INTERRUPT_ENABLE_ENABLE |
				interrupt_source)) {
		LOG_ERR("Interrupts could not be enabled.");
		return -EIO;
	}

	data->trg_handler = handler;
	data->trigger = *trig;
	gpio_pin_enable_callback(data->gpio,
		DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN);

	return 0;
}

/* Enabling GPIO sense on BH1749 INT pin. */
int bh1749_gpio_interrupt_init(struct device *dev)
{
	int err;
	struct bh1749_data *drv_data = dev->driver_data;

	/* Setup gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_INST_0_ROHM_BH1749_INT_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get binding to %s device!",
			DT_INST_0_ROHM_BH1749_INT_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	if (gpio_pin_configure(drv_data->gpio,
			DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP))) {
		LOG_DBG("Failed to configure interrupt GPIO");
		return -EIO;
	}

	gpio_init_callback(&drv_data->gpio_cb, bh1749_gpio_callback,
			   BIT(DT_INST_0_ROHM_BH1749_INT_GPIOS_PIN));

	err = gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb);
	if (err) {
		LOG_DBG("Failed to set GPIO callback");
		return err;
	}

	drv_data->work.handler = bh1749_work_cb;
	drv_data->dev = dev;

	return 0;
}
