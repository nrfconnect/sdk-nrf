/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define MODEM_NODE		DT_NODELABEL(modem)
#define MODEM_DCDC_NODE		DT_NODELABEL(modem_dcdc)
#define DELAY_MODEM_POWERKEY	K_MSEC(200)
#define DELAY_MODEM_BOOT	K_MSEC(100)

/* The devicetree node identifier for the LED alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct gpio_dt_spec modem_powerkey_gpio =
	GPIO_DT_SPEC_GET(MODEM_NODE, mdm_power_gpios);
static const struct gpio_dt_spec modem_rst_gpio =
	GPIO_DT_SPEC_GET(MODEM_NODE, mdm_reset_gpios);

static void modem_reset(void)
{
	if (!gpio_is_ready_dt(&modem_rst_gpio)) {
		return;
	}
	gpio_pin_set_dt(&modem_rst_gpio, 1);
	k_sleep(K_MSEC(100));
	gpio_pin_set_dt(&modem_rst_gpio, 0);
}

static void button_handler(struct input_event *evt, void *user_data)
{
	if (evt->type == INPUT_EV_KEY && evt->code == INPUT_KEY_0 && evt->value == 1) {
		modem_reset();
	} else if (evt->type == INPUT_EV_KEY && evt->code == INPUT_KEY_1) {
		gpio_pin_set_dt(&modem_powerkey_gpio, evt->value ? 1 : 0);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_handler, NULL);

int main(void)
{
	printf("Modem_Bypass on %s\n", CONFIG_BOARD_TARGET);
	printf("Build on %s %s\n", __DATE__, __TIME__);

	/* LED4 blue */
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_ACTIVE);

	/* Keep GPIO active */
	pm_device_runtime_get(DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(button0))));
	return 0;
}

static void modem_boot_work(struct k_work *work);

struct {
	enum {
		ACTIVATE_PWRKEY,
		DEACTIVATE_PWRKEY,
		DONE
	} state;
	struct k_work_delayable dwork;
} static nrf93_pwr_state_data = {
	.state = ACTIVATE_PWRKEY,
	.dwork = Z_WORK_DELAYABLE_INITIALIZER(modem_boot_work),
};

static void modem_boot_work(struct k_work *work)
{
	int ret = 0;

	switch (nrf93_pwr_state_data.state) {
	case ACTIVATE_PWRKEY:
		pm_device_runtime_get(DEVICE_DT_GET(MODEM_NODE));
		ret = gpio_pin_configure_dt(&modem_powerkey_gpio, GPIO_OUTPUT_ACTIVE);
		nrf93_pwr_state_data.state = DEACTIVATE_PWRKEY;
		k_work_reschedule(&nrf93_pwr_state_data.dwork, DELAY_MODEM_POWERKEY);
		break;
	case DEACTIVATE_PWRKEY:
		ret = gpio_pin_set_dt(&modem_powerkey_gpio, 0);
		nrf93_pwr_state_data.state = DONE;
		break;
	default:
		return;
	}

	if (ret < 0) {
		printf("Failed to set modem GPIO: %d\n", ret);
	}
}

static int power_modem_on_boot(void)
{
	k_work_schedule(&nrf93_pwr_state_data.dwork, DELAY_MODEM_BOOT);
	return 0;
}
SYS_INIT(power_modem_on_boot, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/*
 * Dummy Modem drivers to allow power-domain drivers to work
 */

int dev_pm_control(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int modem_init(const struct device *dev)
{
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(dev);

	return 0;
}

PM_DEVICE_DT_DEFINE(MODEM_NODE, dev_pm_control);
DEVICE_DT_DEFINE(MODEM_NODE, modem_init, PM_DEVICE_DT_GET(MODEM_NODE),
		 NULL, NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, NULL);
