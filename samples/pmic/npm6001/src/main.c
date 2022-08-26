/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>

#include "drv_npm6001.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#define LED_INIT_OK DK_LED1
#define LED_DCDC3_ON DK_LED2
#define LED_GENIO0_ON DK_LED4

/* Set maximum DCDC0 voltage in order for LEDs to appear bright */
#define DCDC0_DEFAULT_VOLTAGE DRV_NPM6001_DCDC0_MAXV
#define DCDC1_DEFAULT_VOLTAGE 1200
#define DCDC2_DEFAULT_VOLTAGE 1200
#define DCDC3_DEFAULT_VOLTAGE 2800
#define LDO0_DEFAULT_VOLTAGE 2700

K_SEM_DEFINE(n_int_sem, 0, 1);

static const struct device *i2c_dev = DEVICE_DT_GET_ONE(nordic_nrf_twim);

static const struct gpio_dt_spec n_int = GPIO_DT_SPEC_GET(DT_ALIAS(nint), gpios);
static struct gpio_callback n_int_callback;

static uint16_t dcdc0_voltage;
static bool dcdc3_enabled;
static bool genio0_set;

static int drv_npm6001_platform_init(void)
{
	if (i2c_dev == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int drv_npm6001_twi_read(uint8_t *buf, uint8_t len, uint8_t reg_addr)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST)) {
		memset(buf, 0, len);
		return len;
	}

	LOG_DBG("twi_read %d bytes @ 0x%02X", len, reg_addr);

	err = i2c_write_read(i2c_dev, DRV_NPM6001_TWI_ADDR,
		&reg_addr, sizeof(reg_addr),
		buf, len);

	if (err == 0) {
		/* Success: return number of bytes read */
		return len;
	} else {
		return -1;
	}
}

static int drv_npm6001_twi_write(const uint8_t *buf, uint8_t len, uint8_t reg_addr)
{
	struct i2c_msg msgs[] = {
		{.buf = &reg_addr, .len = sizeof(reg_addr), .flags = I2C_MSG_WRITE},
		{.buf = (uint8_t *)buf, .len = len, .flags = I2C_MSG_WRITE | I2C_MSG_STOP},
	};
	int err;

	if (IS_ENABLED(CONFIG_TEST)) {
		return len;
	}

	if (len == 1) {
		LOG_DBG("twi_write value 0x%02X @ 0x%02X", *buf, reg_addr);
	} else {
		LOG_DBG("twi_write %d bytes @ 0x%02X", len, reg_addr);
	}

	err = i2c_transfer(i2c_dev, &msgs[0], ARRAY_SIZE(msgs), DRV_NPM6001_TWI_ADDR);
	if (err == 0) {
		/* Success: return number of bytes written */
		return len;
	} else {
		return -1;
	}
}

/**@brief Print sample help message to log
 */
static void print_info_message(void)
{
	LOG_INF("nPM6001 sample");
	LOG_INF("");
	LOG_INF("Button behavior:");
	LOG_INF("  Button 1: Toggle DCDC3 regulator on/off");
	LOG_INF("  Button 2: Increase DCDC0 voltage by 100 mV");
	LOG_INF("  Button 3: Decrease DCDC0 voltage by 100 mV");
	LOG_INF("  Button 4: Toggle nPM6001 GENIO0 pin");
	LOG_INF("  All four buttons pressed at the same time: Hibernate for 8 seconds");
	LOG_INF("LED behavior:");
	LOG_INF("  LED 1: On after successful initialization");
	LOG_INF("  LED 2: On when DCDC3 is turned on");
	LOG_INF("  LED 4: On when GENIO0 pin is set");
	LOG_INF("  All four LEDs blinking: an error occurred during initialization");
	LOG_INF("");
	LOG_INF("NOTE: The LEDs will likely appear weak if initialization fails,");
	LOG_INF("due to the default nPM6001 DCDC0 supply voltage being relatively low.");
	LOG_INF("");
}

/**@brief Print shell help message to log
 */
static void print_shell_message(void)
{
	LOG_INF("The shell is currently enabled.");
	LOG_INF("See relevant 'npm6001' root command and assosicated subcommands.");
	LOG_INF("(hint: use the 'tab' character for autocompletion)");
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons that has
 *                            changed their state.
 */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	bool set_voltage = false;
	int err;

	if ((button_state & DK_BTN1_MSK) & has_changed) {
		/* Toggle DCDC3 regulator */
		if (dcdc3_enabled) {
			err = drv_npm6001_vreg_disable(DRV_NPM6001_DCDC3);
			if (err) {
				LOG_ERR("drv_npm6001_vreg_disable=%d", err);
			} else {
				dcdc3_enabled = false;
				(void) dk_set_led_off(LED_DCDC3_ON);
			}
		} else {
			err = drv_npm6001_vreg_enable(DRV_NPM6001_DCDC3);
			if (err) {
				LOG_ERR("drv_npm6001_vreg_enable=%d", err);
			} else {
				dcdc3_enabled = true;
				(void) dk_set_led_on(LED_DCDC3_ON);
			}
		}
	}

	if ((button_state & DK_BTN2_MSK) & has_changed) {
		/* Increase voltage by 100 mV */
		dcdc0_voltage += 100;
		set_voltage = true;
	}

	if ((button_state & DK_BTN3_MSK) & has_changed) {
		/* Decrease voltage by 100 mV */
		dcdc0_voltage -= 100;
		set_voltage = true;
	}

	if ((button_state & DK_BTN4_MSK) & has_changed) {
		/* Toggle GENIO0 pin */
		if (genio0_set) {
			err = drv_npm6001_genio_clr(DRV_NPM6001_GENIO0);
			if (err) {
				LOG_ERR("drv_npm6001_genio_set=%d", err);
			} else {
				genio0_set = false;
				(void) dk_set_led_off(LED_GENIO0_ON);
			}
		} else {
			err = drv_npm6001_genio_set(DRV_NPM6001_GENIO0);
			if (err) {
				LOG_ERR("drv_npm6001_genio_set=%d", err);
			} else {
				genio0_set = true;
				(void) dk_set_led_on(LED_GENIO0_ON);
			}
		}
	}

	if (button_state == DK_ALL_BTNS_MSK) {
		/* Hibernate for 8 seconds (= 2 ticks) */
		err = drv_npm6001_hibernate(2);
		if (err) {
			LOG_ERR("drv_npm6001_hibernate=%d", err);
		}
	}

	if (set_voltage) {
		dcdc0_voltage =
			CLAMP(dcdc0_voltage, DRV_NPM6001_DCDC0_MINV, DRV_NPM6001_DCDC0_MAXV);
		err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_DCDC0, dcdc0_voltage);
		if (err) {
			LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		}
	}
}

static void n_int_irq_handler(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	k_sem_give(&n_int_sem);
}

/**@brief Function for initializing LEDs and Buttons. */
static int configure_dk_gpio(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init=%d", err);
		return err;
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("dk_leds_init=%d", err);
		return err;
	}

	return 0;
}

/**@brief Function for initializing nPM6001 specific GPIO. */
static int configure_npm6001_gpio(void)
{
	gpio_flags_t flags = n_int.dt_flags & GPIO_ACTIVE_LOW ? GPIO_PULL_UP : GPIO_PULL_DOWN;
	int err;

	err = gpio_pin_configure_dt(&n_int, GPIO_INPUT | flags);
	if (err) {
		LOG_ERR("gpio_pin_configure_dt=%d", err);
		return err;
	}

	gpio_init_callback(&n_int_callback, n_int_irq_handler, BIT(n_int.pin));

	err = gpio_add_callback(n_int.port, &n_int_callback);
	if (err) {
		LOG_ERR("gpio_add_callback=%d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&n_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("gpio_pin_interrupt_configure_dt=%d", err);
		return err;
	}

	return 0;
}

/**@brief Function for initializing nPM6001 PMIC. */
static int configure_npm6001(void)
{
	const struct drv_npm6001_platform ncs_hw_funcs = {
		.drv_npm6001_platform_init = drv_npm6001_platform_init,
		.drv_npm6001_twi_read = drv_npm6001_twi_read,
		.drv_npm6001_twi_write = drv_npm6001_twi_write,
	};
	int err;

	err = drv_npm6001_init(&ncs_hw_funcs);
	if (err) {
		LOG_ERR("drv_npm6001_init=%d", err);
		return err;
	}

	/* Enable pull-downs on mode pins in case they aren't connected to anything.
	 * Note: this will cause an added power drain when the mode pins are actually pulled high.
	 */
	const struct drv_npm6001_mode_pin_cfg pin_cfg = {
		.pad_type = DRV_NPM6001_MODE_PIN_CFG_PAD_TYPE_CMOS,
		.pulldown = DRV_NPM6001_MODE_PIN_CFG_PULLDOWN_ENABLED,
	};
	const enum drv_npm6001_mode_pin mode_pins[] = {
		DRV_NPM6001_DCDC_PIN_MODE0,
		DRV_NPM6001_DCDC_PIN_MODE1,
		DRV_NPM6001_DCDC_PIN_MODE2,
	};

	for (int i = 0; i < ARRAY_SIZE(mode_pins); ++i) {
		err = drv_npm6001_mode_pin_cfg(mode_pins[i], &pin_cfg);
		if (err) {
			LOG_ERR("drv_npm6001_mode_pin_cfg(%d)=%d", mode_pins[i], err);
			return err;
		}
	}

	/* Enable DCDC_MODE1 pin for DCDC1 */
	err = drv_npm6001_vreg_mode_pin_enable(DRV_NPM6001_DCDC1, DRV_NPM6001_DCDC_PIN_MODE1);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_mode_pin_enable=%d", err);
		return err;
	}

	/* Set DCDC0 voltage */
	dcdc0_voltage = DCDC0_DEFAULT_VOLTAGE;
	err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_DCDC0, dcdc0_voltage);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set DCDC1 voltage */
	err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_DCDC1, DCDC1_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set DCDC1 voltage */
	err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_DCDC2, DCDC2_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set DCDC3 voltage */
	err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_DCDC3, DCDC3_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Enable DCDC3. Unlike DCDC0, DCDC1, and DCDC2, this is not an always-on regulator. */
	err = drv_npm6001_vreg_enable(DRV_NPM6001_DCDC3);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_enable=%d", err);
		return err;
	}

	dcdc3_enabled = true;

	/* Set LDO0 voltage */
	err = drv_npm6001_vreg_voltage_set(DRV_NPM6001_LDO0, LDO0_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Enable LDO0. */
	err = drv_npm6001_vreg_enable(DRV_NPM6001_LDO0);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_enable=%d", err);
		return err;
	}

	/* Enable LDO1. This regulator has fixed 1.8V voltage. */
	err = drv_npm6001_vreg_enable(DRV_NPM6001_LDO1);
	if (err) {
		LOG_ERR("drv_npm6001_vreg_enable=%d", err);
		return err;
	}

	const struct drv_npm6001_genio_cfg genio_cfg = {
		.direction = DRV_NPM6001_GENIO_CFG_DIRECTION_OUTPUT,
		.input = DRV_NPM6001_GENIO_CFG_INPUT_DISABLED,
		.pulldown = DRV_NPM6001_GENIO_CFG_PULLDOWN_DISABLED,
		.drive = DRV_NPM6001_GENIO_CFG_DRIVE_NORMAL,
		.sense = DRV_NPM6001_GENIO_CFG_SENSE_LOW,
	};

	/* Configure GENIO0 pin as an output */
	err = drv_npm6001_genio_cfg(DRV_NPM6001_GENIO0, &genio_cfg);
	if (err) {
		LOG_ERR("drv_npm6001_genio_cfg=%d", err);
		return err;
	}

	genio0_set = false;

	/* Enable thermal warning and overcurrent interrupts */
	const enum drv_npm6001_int interrupts[] = {
		DRV_NPM6001_INT_THERMAL_WARNING,
		DRV_NPM6001_INT_DCDC0_OVERCURRENT,
		DRV_NPM6001_INT_DCDC1_OVERCURRENT,
		DRV_NPM6001_INT_DCDC2_OVERCURRENT,
		DRV_NPM6001_INT_DCDC3_OVERCURRENT,
	};

	for (int i = 0; i < ARRAY_SIZE(interrupts); ++i) {
		err = drv_npm6001_int_enable(interrupts[i]);
		if (err) {
			LOG_ERR("drv_npm6001_int_enable(%d)=%d", (int) interrupts[i], err);
			return err;
		}
	}

	/* Enable thermal warning sensor */
	err = drv_npm6001_thermal_sensor_enable(DRV_NPM6001_THERMAL_SENSOR_WARNING);
	if (err) {
		LOG_ERR("drv_npm6001_thermal_sensor_enable=%d", err);
		return err;
	}

	return 0;
}

void main(void)
{
	int err;

	print_info_message();

	if (IS_ENABLED(CONFIG_SHELL)) {
		print_shell_message();
	}

	err = configure_dk_gpio();
	if (err) {
		LOG_ERR("configure_dk_gpio=%d", err);
		goto init_error;
	}

	err = configure_npm6001_gpio();
	if (err) {
		LOG_ERR("configure_npm6001_gpio=%d", err);
		goto init_error;
	}

	err = configure_npm6001();
	if (err) {
		LOG_ERR("configure_npm6001=%d", err);
		goto init_error;
	}

	LOG_INF("nPM6001 successfully initialized.");

	err = dk_set_led_on(LED_INIT_OK);
	if (err) {
		LOG_ERR("dk_set_led_on=%d", err);
		goto init_error;
	}

	err = dk_set_led_on(LED_DCDC3_ON);
	if (err) {
		LOG_ERR("dk_set_led_on=%d", err);
		goto init_error;
	}

	/* Wait for interrupts */
	while (true) {
		k_sem_take(&n_int_sem, K_FOREVER);

		/* N_INT pin will be active as long as there is one or more unread interrupts */
		do {
			enum drv_npm6001_int int_src;

			err = drv_npm6001_int_read(&int_src);
			if (err) {
				LOG_ERR("drv_npm6001_int_read=%d", err);
				break;
			}

			switch (int_src) {
			case DRV_NPM6001_INT_THERMAL_WARNING:
				LOG_WRN("Thermal warning interrupt");
				break;
			case DRV_NPM6001_INT_DCDC0_OVERCURRENT:
				LOG_WRN("DCDC0 overcurrent interrupt");
				break;
			case DRV_NPM6001_INT_DCDC1_OVERCURRENT:
				LOG_WRN("DCDC1 overcurrent interrupt");
				break;
			case DRV_NPM6001_INT_DCDC2_OVERCURRENT:
				LOG_WRN("DCDC2 overcurrent interrupt");
				break;
			case DRV_NPM6001_INT_DCDC3_OVERCURRENT:
				LOG_WRN("DCDC3 overcurrent interrupt");
				break;
			default:
				LOG_ERR("Unknown interrupt: %d", int_src);
				break;
			}
		} while (gpio_pin_get_dt(&n_int));
	}

	return;

init_error:

	while (true) {
		(void) dk_set_leds(DK_ALL_LEDS_MSK);
		k_msleep(250);
		(void) dk_set_leds(DK_NO_LEDS_MSK);
		k_msleep(250);
	}
}
