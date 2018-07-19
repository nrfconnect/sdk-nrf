/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/* This is a temporary file utilizing SR3 Shield HW. */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <i2c.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"

#define MODULE motion
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_MOTION_MODULE_LEVEL
#include <logging/sys_log.h>


/* Product ID expected to be received from product ID query. */
static const u8_t expected_product_id[] = {
	'T', 'M', '1', '9', '4', '4', '-', '0', '0', '2'};

#define TOUCHPAD_ADDR		0x20 /* I2C Address */
#define TOUCHPAD_PRODUCT_ID	0xA2 /* Product ID string register address */
#define TOUCHPAD_XY		0x30


static struct device *i2c_dev;
static struct k_delayed_work motion_scan;

/* TODO use atomic here */
static bool active;
static bool terminating;


static int read_bytes(struct device *i2c_dev, u16_t addr, u8_t *data,
		u32_t num_bytes)
{
	struct i2c_msg msgs[2];

	u8_t wr_addr = addr;

	/* Send the address to read from */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], ARRAY_SIZE(msgs), TOUCHPAD_ADDR);
}

static void scan_fn(struct k_work *work)
{
	s8_t pos[2];

	/* TODO this implementation MUST be fixed - push SDK to implement I2C
	 * async callback!
	 */
	int err = read_bytes(i2c_dev, TOUCHPAD_XY, pos, sizeof(pos));

	if (err) {
		SYS_LOG_ERR("i2c read error (%d) from %s:%d",
				err, __func__, __LINE__);
	} else {
		if (pos[0] || pos[1]) {
			struct motion_event *event = new_motion_event();

			if (event) {
				if (pos[0] < 7) {
					event->dx = 3 * pos[0] / 2;
				} else {
					event->dx = 2 * pos[0];
				}
				if (pos[1] < 7) {
					event->dy = -3 * pos[1] / 2;
				} else {
					event->dy = -2 * pos[1];
				}

				EVENT_SUBMIT(event);
			}
		}
		/* TODO note that we cannot go below kernel tick
		 * actually if tick is 10ms (default) interval cannot go below
		 * 20ms I set kernel tick to 1ms.
		 */

		if (active) {
			k_delayed_work_submit(&motion_scan, 50);
		}
	}
}

static void async_init_fn(struct k_work *work)
{
	int err = 0;

	struct device *gpio_dev =
		device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	if (!gpio_dev) {
		SYS_LOG_ERR("cannot get GPIO device");
		return;
	}

	/* Turn on touchpad */

	err |= gpio_pin_configure(gpio_dev, 19, GPIO_DIR_OUT); /* discharge */
	err |= gpio_pin_configure(gpio_dev,  3, GPIO_DIR_OUT); /* shutdown */
	if (err) {
		return;
	}

	/* Discharge first */
	err |= gpio_pin_write(gpio_dev, 19, 1);
	k_sleep(10);
	err |= gpio_pin_write(gpio_dev, 19, 0);
	if (err) {
		return;
	}

	/* Enable TP VCC */
	err |= gpio_pin_write(gpio_dev,  3, 1);
	if (err) {
		return;
	}


	/* Wait for VCC to be stable */
	k_sleep(100);


	/* Check if TP is connected */
	i2c_dev = device_get_binding("I2C_0");
	if (!i2c_dev) {
		SYS_LOG_ERR("cannot get I2C device");
		return;
	}

	u8_t product_id[sizeof(expected_product_id)];

	err = read_bytes(i2c_dev, TOUCHPAD_PRODUCT_ID, product_id,
			sizeof(product_id));
	if (err) {
		SYS_LOG_ERR("i2c read error (%d) from %s:%d",
				err, __func__, __LINE__);
	} else {
		for (size_t i = 0; i < sizeof(product_id); i++) {
			if (expected_product_id[i] != product_id[i]) {
				SYS_LOG_ERR("invalid product id (0x%x != 0x%x)",
						expected_product_id[i],
						product_id[i]);
				return;
			}
		}
	}

	/* Inform all that module is ready */

	module_set_state(MODULE_STATE_READY);

	k_delayed_work_submit(&motion_scan, 25);
}
K_WORK_DEFINE(motion_async_init, async_init_fn);

static void async_term_fn(struct k_work *work)
{
	int err = 0;

	/* Cancel scanning routine */
	k_delayed_work_cancel(&motion_scan);
	/* TODO this will work as long as work is executed on the same queue
	 * otherwise scan can be rescheduled if scanning function is processing
	 * just now.
	 */

	struct device *gpio_dev =
		device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	if (!gpio_dev) {
		SYS_LOG_ERR("cannot get GPIO device");
		return;
	}

	/* Disable TP VCC */
	err |= gpio_pin_write(gpio_dev,  3, 0);
	if (err) {
		return;
	}

	/* Discharge */
	err |= gpio_pin_write(gpio_dev, 19, 1);
	k_sleep(2);
	err |= gpio_pin_write(gpio_dev, 19, 0);
	if (err) {
		return;
	}

	/* Inform all that module is off */

	module_set_state(MODULE_STATE_OFF);

	terminating = false;
	active = false;
}
K_WORK_DEFINE(motion_async_term, async_term_fn);

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&motion_scan, scan_fn);
			return false;
		}

		if (check_state(event, MODULE_ID(board), MODULE_STATE_READY)) {
			if (!active) {
				active = true;
				k_work_submit(&motion_async_init);
			}
			return false;
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (!terminating && active) {
			k_work_submit(&motion_async_term);
			terminating = true;
		}

		return active;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
