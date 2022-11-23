/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "button_handler.h"
#include "button_assignments.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "macros_common.h"
#include "ctrl_events.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(button_handler, CONFIG_MODULE_BUTTON_HANDLER_LOG_LEVEL);

/* How many buttons does the module support. Increase at memory cost */
#define BUTTONS_MAX 5
#define BASE_10 10

static bool debounce_is_ongoing;
static struct gpio_callback btn_callback[BUTTONS_MAX];

const static struct btn_config btn_cfg[] = {
	{
		.btn_name = STRINGIFY(BUTTON_VOLUME_DOWN),
		.btn_pin = BUTTON_VOLUME_DOWN,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_VOLUME_UP),
		.btn_pin = BUTTON_VOLUME_UP,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_PLAY_PAUSE),
		.btn_pin = BUTTON_PLAY_PAUSE,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw2), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_4),
		.btn_pin = BUTTON_4,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw3), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_5),
		.btn_pin = BUTTON_5,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw4), gpios),
	}
};

static const struct device *gpio_53_dev;

/**@brief Simple debouncer for buttons
 *
 * @note Needed as low-level driver debouce is not
 * implemented in Zephyr for nRF53 yet
 */
static void on_button_debounce_timeout(struct k_timer *timer)
{
	debounce_is_ongoing = false;
}

K_TIMER_DEFINE(button_debounce_timer, on_button_debounce_timeout, NULL);

/** @brief Find the index of a button from the pin number
 */
static int pin_to_btn_idx(uint8_t btn_pin, uint32_t *pin_idx)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(btn_cfg); i++) {
		if (btn_pin == btn_cfg[i].btn_pin) {
			*pin_idx = i;
			return 0;
		}
	}

	LOG_WRN("Button idx not found");
	return -ENODEV;
}

/** @brief Convert from mask to pin
 *
 * @note: Will check that a single bit and a single bit only is set in the mask.
 */
static int pin_msk_to_pin(uint32_t pin_msk, uint32_t *pin_out)
{
	if (!pin_msk) {
		LOG_ERR("Mask is empty");
		return -EACCES;
	}

	if (pin_msk & (pin_msk - 1)) {
		LOG_ERR("Two or more buttons set in mask");
		return -EACCES;
	}

	*pin_out = 0;

	while (pin_msk) {
		pin_msk = pin_msk >> 1;
		(*pin_out)++;
	}

	/* Deduct 1 for zero indexing */
	(*pin_out)--;

	return 0;
}

/*  ISR triggered by GPIO when assigned button(s) are pushed */
static void button_isr(const struct device *port, struct gpio_callback *cb, uint32_t pin_msk)
{
	int ret;
	struct event_t event;

	if (debounce_is_ongoing) {
		LOG_WRN("Btn debounce in action");
		return;
	}

	uint32_t btn_pin = 0;
	uint32_t btn_idx = 0;

	ret = pin_msk_to_pin(pin_msk, &btn_pin);
	ERR_CHK(ret);

	ret = pin_to_btn_idx(btn_pin, &btn_idx);
	ERR_CHK(ret);

	LOG_DBG("Pushed button idx: %d pin: %d name: %s", btn_idx, btn_pin,
		btn_cfg[btn_idx].btn_name);

	event.button_activity.button_pin = btn_pin;
	event.button_activity.button_action = BUTTON_PRESS;
	event.event_source = EVT_SRC_BUTTON;

	/* To avoid filling up the event queue with button presses,
	 * we only allow button events if all other events have been processed
	 */
	if (ctrl_events_queue_empty()) {
		ret = ctrl_events_put(&event);
		ERR_CHK(ret);
		debounce_is_ongoing = true;
		k_timer_start(&button_debounce_timer, K_MSEC(CONFIG_BUTTON_DEBOUNCE_MS), K_NO_WAIT);
	} else {
		LOG_WRN("Event queue is not empty, try again later");
	}
}

int button_pressed(gpio_pin_t button_pin, bool *button_pressed)
{
	int ret;

	if (!device_is_ready(gpio_53_dev)) {
		return -ENODEV;
	}

	if (button_pressed == NULL) {
		return -EINVAL;
	}

	ret = gpio_pin_get(gpio_53_dev, button_pin);
	switch (ret) {
	case 0:
		*button_pressed = false;
		break;
	case 1:
		*button_pressed = true;
		break;
	default:
		return ret;
	}

	return 0;
}

int button_handler_init(void)
{
	int ret;

	if (ARRAY_SIZE(btn_cfg) == 0) {
		LOG_WRN("No buttons assigned");
		return -EINVAL;
	}

	gpio_53_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio_53_dev)) {
		LOG_ERR("Device driver not ready.");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(btn_cfg); i++) {
		ret = gpio_pin_configure(gpio_53_dev, btn_cfg[i].btn_pin,
					 GPIO_INPUT | btn_cfg[i].btn_cfg_mask);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&btn_callback[i], button_isr, BIT(btn_cfg[i].btn_pin));

		ret = gpio_add_callback(gpio_53_dev, &btn_callback[i]);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure(gpio_53_dev, btn_cfg[i].btn_pin,
						   GPIO_INT_EDGE_TO_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

/* Shell functions */
static int cmd_print_all_btns(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (uint8_t i = 0; i < ARRAY_SIZE(btn_cfg); i++) {
		shell_print(shell, "Id %d: pin: %d %s", i, btn_cfg[i].btn_pin, btn_cfg[i].btn_name);
	}

	return 0;
}

static int cmd_push_btn(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t btn_idx;
	struct event_t event;

	/* First argument is function, second is button idx */
	if (argc != 2) {
		shell_error(shell, "Wrong number of arguments provided");
		return -EINVAL;
	}

	if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		return -EINVAL;
	}

	btn_idx = strtoul(argv[1], NULL, BASE_10);

	if (btn_idx >= ARRAY_SIZE(btn_cfg)) {
		shell_error(shell, "Selected button ID out of range");
		return -EINVAL;
	}

	event.button_activity.button_pin = btn_cfg[btn_idx].btn_pin;
	event.button_activity.button_action = BUTTON_PRESS;
	event.event_source = EVT_SRC_BUTTON;

	ret = ctrl_events_put(&event);

	if (ret == -ENOMSG) {
		LOG_WRN("Event queue is full, ignoring button press");
		ret = 0;
	}

	ERR_CHK(ret);

	shell_print(shell, "Pushed button idx: %d pin: %d : %s", btn_idx, btn_cfg[btn_idx].btn_pin,
		    btn_cfg[btn_idx].btn_name);

	return 0;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(buttons_cmd,
			       SHELL_COND_CMD(CONFIG_SHELL, print, NULL, "Print all buttons.",
					      cmd_print_all_btns),
			       SHELL_COND_CMD(CONFIG_SHELL, push, NULL, "Push button.",
					      cmd_push_btn),
			       SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(buttons, &buttons_cmd, "List and push buttons", NULL);
