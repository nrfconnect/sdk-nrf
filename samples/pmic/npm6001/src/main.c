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

#include <lib_npm6001.h>

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

#define LED_INIT_OK DK_LED1
#define LED_BUCK3_ON DK_LED2
#define LED_GPIO_ON DK_LED4

#define BUCK0_DEFAULT_VOLTAGE 3300
#define BUCK1_DEFAULT_VOLTAGE 1200
#define BUCK2_DEFAULT_VOLTAGE 1400
#define BUCK3_DEFAULT_VOLTAGE 500
#define LDO0_DEFAULT_VOLTAGE 2700

static K_SEM_DEFINE(n_int_sem, 0, 1);

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_i2c));

static const struct gpio_dt_spec n_int = GPIO_DT_SPEC_GET(DT_NODELABEL(n_int), gpios);
static struct gpio_callback n_int_callback;

static uint16_t buck3_voltage;
static bool buck3_enabled;
static bool npm6001_gpio_set;

static int lib_npm6001_platform_init(void)
{
	if (device_is_ready(i2c_dev)) {
		return 0;
	} else {
		return -ENODEV;
	}
}

static int lib_npm6001_twi_read(uint8_t *buf, uint8_t len, uint8_t reg_addr)
{
	if (IS_ENABLED(CONFIG_TEST)) {
		memset(buf, 0, len);
		return 0;
	}

	LOG_DBG("twi_read %d bytes @ 0x%02X", len, reg_addr);

	return i2c_write_read(i2c_dev, LIB_NPM6001_TWI_ADDR,
		&reg_addr, sizeof(reg_addr),
		buf, len);
}

static int lib_npm6001_twi_write(const uint8_t *buf, uint8_t len, uint8_t reg_addr)
{
	struct i2c_msg msgs[] = {
		{.buf = &reg_addr, .len = sizeof(reg_addr), .flags = I2C_MSG_WRITE},
		{.buf = (uint8_t *)buf, .len = len, .flags = I2C_MSG_WRITE | I2C_MSG_STOP},
	};

	if (IS_ENABLED(CONFIG_TEST)) {
		return 0;
	}

	if (len == 1) {
		LOG_DBG("twi_write value 0x%02X @ 0x%02X", *buf, reg_addr);
	} else {
		LOG_DBG("twi_write %d bytes @ 0x%02X", len, reg_addr);
	}

	return i2c_transfer(i2c_dev, &msgs[0], ARRAY_SIZE(msgs), LIB_NPM6001_TWI_ADDR);
}

/**
 * @brief Print sample help message to log
 */
static void print_info_message(void)
{
	LOG_INF("nPM6001 sample");
	LOG_INF("");
	LOG_INF("Button behavior:");
	LOG_INF("  Button 1: Toggle BUCK3 regulator on/off");
	LOG_INF("  Button 2: Increase BUCK3 voltage by 25 mV");
	LOG_INF("  Button 3: Decrease BUCK3 voltage by 25 mV");
	LOG_INF("  Button 4: Toggle nPM6001 GPIO pins");
	LOG_INF("  All four buttons pressed at the same time: Hibernate for 8 seconds");
	LOG_INF("LED behavior:");
	LOG_INF("  LED 1: On after successful initialization");
	LOG_INF("  LED 2: On when BUCK3 is turned on");
	LOG_INF("  LED 4: On when nPM6001 GPIO pins are set");
	LOG_INF("  All four LEDs blinking: an error occurred during initialization");
	LOG_INF("");
	LOG_INF("NOTE: The LEDs will likely appear weak if initialization fails,");
	LOG_INF("due to the default nPM6001 BUCK0 supply voltage being relatively low.");
	LOG_INF("");
}

/**
 * @brief Print shell help message to log
 */
static void print_shell_message(void)
{
	LOG_INF("The shell is currently enabled.");
	LOG_INF("See relevant 'npm6001' root command and assosicated subcommands.");
	LOG_INF("(hint: use the 'tab' character for autocompletion)");
}

/**
 * @brief Callback for button events.
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
		/* Toggle BUCK3 regulator */
		if (buck3_enabled) {
			err = lib_npm6001_vreg_disable(LIB_NPM6001_BUCK3);
			if (err) {
				LOG_ERR("lib_npm6001_vreg_disable=%d", err);
			} else {
				buck3_enabled = false;
				(void) dk_set_led_off(LED_BUCK3_ON);
			}
		} else {
			err = lib_npm6001_vreg_enable(LIB_NPM6001_BUCK3);
			if (err) {
				LOG_ERR("lib_npm6001_vreg_enable=%d", err);
			} else {
				buck3_enabled = true;
				(void) dk_set_led_on(LED_BUCK3_ON);
			}
		}
	}

	if ((button_state & DK_BTN2_MSK) & has_changed) {
		/* Increase voltage by 25 mV */
		buck3_voltage += 25;
		set_voltage = true;
	}

	if ((button_state & DK_BTN3_MSK) & has_changed) {
		/* Decrease voltage by 25 mV */
		buck3_voltage -= 25;
		set_voltage = true;
	}

	if ((button_state & DK_BTN4_MSK) & has_changed) {
		const enum lib_npm6001_gpio gpio_pins[] = {
			LIB_NPM6001_GPIO0,
			LIB_NPM6001_GPIO1,
			LIB_NPM6001_GPIO2,
		};

		/* Toggle nPM6001 GPIO pins */
		for (int i = 0; i < ARRAY_SIZE(gpio_pins); ++i) {
			if (npm6001_gpio_set) {
				err = lib_npm6001_gpio_clr(gpio_pins[i]);
				if (err) {
					LOG_ERR("lib_npm6001_gpio_set=%d", err);
				}
			} else {
				err = lib_npm6001_gpio_set(gpio_pins[i]);
				if (err) {
					LOG_ERR("lib_npm6001_gpio_set=%d", err);
				}
			}
		}

		if (npm6001_gpio_set) {
			npm6001_gpio_set = false;
			(void) dk_set_led_off(LED_GPIO_ON);
		} else {
			npm6001_gpio_set = true;
			(void) dk_set_led_on(LED_GPIO_ON);
		}
	}

	if (button_state == DK_ALL_BTNS_MSK) {
		/* Hibernate for 8 seconds (= 2 ticks) */
		err = lib_npm6001_hibernate(2);
		if (err) {
			LOG_ERR("lib_npm6001_hibernate=%d", err);
		}
	}

	if (set_voltage) {
		buck3_voltage =
			CLAMP(buck3_voltage, LIB_NPM6001_BUCK3_MINV, LIB_NPM6001_BUCK3_MAXV);
		err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_BUCK3, buck3_voltage);
		if (err) {
			LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		}
	}
}

static void n_int_irq_handler(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	k_sem_give(&n_int_sem);
}

/**
 * @brief Function for initializing LEDs and Buttons.
 */
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

/**
 * @brief Function for initializing nPM6001 specific GPIO.
 */
static int configure_npm6001_gpio(void)
{
	int err;

	err = gpio_pin_configure_dt(&n_int, GPIO_INPUT);
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

/**
 * @brief Function for initializing nPM6001 PMIC.
 */
static int configure_npm6001(void)
{
	const struct lib_npm6001_platform ncs_hw_funcs = {
		.lib_npm6001_platform_init = lib_npm6001_platform_init,
		.lib_npm6001_twi_read = lib_npm6001_twi_read,
		.lib_npm6001_twi_write = lib_npm6001_twi_write,
	};
	int err;

	err = lib_npm6001_init(&ncs_hw_funcs);
	if (err) {
		LOG_ERR("lib_npm6001_init=%d", err);
		return err;
	}

	/* Enable pull-downs on mode pins in case they aren't connected to anything.
	 * Note: this will cause an added power drain when the mode pins are actually pulled high.
	 */
	const struct lib_npm6001_mode_pin_cfg pin_cfg = {
		.pad_type = LIB_NPM6001_MODE_PIN_CFG_PAD_TYPE_CMOS,
		.pulldown = LIB_NPM6001_MODE_PIN_CFG_PULLDOWN_ENABLED,
	};
	const enum lib_npm6001_mode_pin mode_pins[] = {
		LIB_NPM6001_BUCK_PIN_MODE0,
		LIB_NPM6001_BUCK_PIN_MODE1,
		LIB_NPM6001_BUCK_PIN_MODE2,
	};

	for (int i = 0; i < ARRAY_SIZE(mode_pins); ++i) {
		err = lib_npm6001_mode_pin_cfg(mode_pins[i], &pin_cfg);
		if (err) {
			LOG_ERR("lib_npm6001_mode_pin_cfg(%d)=%d", mode_pins[i], err);
			return err;
		}
	}

	/* Enable BUCK_MODE1 pin for BUCK1 */
	err = lib_npm6001_vreg_mode_pin_enable(LIB_NPM6001_BUCK1, LIB_NPM6001_BUCK_PIN_MODE1);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_mode_pin_enable=%d", err);
		return err;
	}

	/* Set BUCK0 voltage */
	err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_BUCK0, BUCK0_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set BUCK1 voltage */
	err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_BUCK1, BUCK1_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set BUCK1 voltage */
	err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_BUCK2, BUCK2_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Set BUCK3 voltage */
	buck3_voltage = BUCK3_DEFAULT_VOLTAGE;
	err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_BUCK3, buck3_voltage);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Enable BUCK3. Unlike BUCK0, BUCK1, and BUCK2, this is not an always-on regulator. */
	err = lib_npm6001_vreg_enable(LIB_NPM6001_BUCK3);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_enable=%d", err);
		return err;
	}

	buck3_enabled = true;

	/* Set LDO0 voltage */
	err = lib_npm6001_vreg_voltage_set(LIB_NPM6001_LDO0, LDO0_DEFAULT_VOLTAGE);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_voltage_set=%d", err);
		return err;
	}

	/* Enable LDO0. */
	err = lib_npm6001_vreg_enable(LIB_NPM6001_LDO0);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_enable=%d", err);
		return err;
	}

	/* Enable LDO1. This regulator has fixed 1.8V voltage. */
	err = lib_npm6001_vreg_enable(LIB_NPM6001_LDO1);
	if (err) {
		LOG_ERR("lib_npm6001_vreg_enable=%d", err);
		return err;
	}

	const struct lib_npm6001_gpio_cfg gpio_cfg = {
		.direction = LIB_NPM6001_GPIO_CFG_DIRECTION_OUTPUT,
		.input = LIB_NPM6001_GPIO_CFG_INPUT_DISABLED,
		.pulldown = LIB_NPM6001_GPIO_CFG_PULLDOWN_DISABLED,
		.drive = LIB_NPM6001_GPIO_CFG_DRIVE_NORMAL,
		.sense = LIB_NPM6001_GPIO_CFG_SENSE_LOW,
	};
	const enum lib_npm6001_gpio gpio_pins[] = {
		LIB_NPM6001_GPIO0,
		LIB_NPM6001_GPIO1,
		LIB_NPM6001_GPIO2,
	};

	/* Configure nPM6001 GPIO pins as outputs */
	for (int i = 0; i < ARRAY_SIZE(gpio_pins); ++i) {
		err = lib_npm6001_gpio_cfg(gpio_pins[i], &gpio_cfg);
		if (err) {
			LOG_ERR("lib_npm6001_gpio_cfg=%d", err);
			return err;
		}
	}

	npm6001_gpio_set = false;

	/* Enable overcurrent interrupts */
	const enum lib_npm6001_int interrupts[] = {
		LIB_NPM6001_INT_BUCK0_OVERCURRENT,
		LIB_NPM6001_INT_BUCK1_OVERCURRENT,
		LIB_NPM6001_INT_BUCK2_OVERCURRENT,
		LIB_NPM6001_INT_BUCK3_OVERCURRENT,
	};

	for (int i = 0; i < ARRAY_SIZE(interrupts); ++i) {
		err = lib_npm6001_int_enable(interrupts[i]);
		if (err) {
			LOG_ERR("lib_npm6001_int_enable(%d)=%d", (int) interrupts[i], err);
			return err;
		}
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

	/* With this hardware configuration, the nPM6001 powers up the nRF5x SoC and associated
	 * Development Kit circuitry. Having a generous delay allows all voltages to settle before
	 * communicating with the nPM6001 EK via the voltage level shifters.
	 */
	k_sleep(K_SECONDS(1));

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

	err = dk_set_led_on(LED_BUCK3_ON);
	if (err) {
		LOG_ERR("dk_set_led_on=%d", err);
		goto init_error;
	}

	/* Wait for interrupts */
	while (true) {
		k_sem_take(&n_int_sem, K_FOREVER);

		/* N_INT pin will be active as long as there is one or more unread interrupts */
		do {
			enum lib_npm6001_int int_src;

			err = lib_npm6001_int_read(&int_src);
			if (err) {
				LOG_ERR("lib_npm6001_int_read=%d", err);
				break;
			}

			switch (int_src) {
			case LIB_NPM6001_INT_THERMAL_WARNING:
				LOG_WRN("Thermal warning interrupt");
				break;
			case LIB_NPM6001_INT_BUCK0_OVERCURRENT:
				LOG_WRN("BUCK0 overcurrent interrupt");
				break;
			case LIB_NPM6001_INT_BUCK1_OVERCURRENT:
				LOG_WRN("BUCK1 overcurrent interrupt");
				break;
			case LIB_NPM6001_INT_BUCK2_OVERCURRENT:
				LOG_WRN("BUCK2 overcurrent interrupt");
				break;
			case LIB_NPM6001_INT_BUCK3_OVERCURRENT:
				LOG_WRN("BUCK3 overcurrent interrupt");
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
