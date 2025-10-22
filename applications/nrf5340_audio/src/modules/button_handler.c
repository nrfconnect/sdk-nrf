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
#include <zephyr/zbus/zbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "macros_common.h"
#include "zbus_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(button_handler, CONFIG_MODULE_BUTTON_HANDLER_LOG_LEVEL);

ZBUS_CHAN_DEFINE(button_chan, struct button_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* Button configuration structure. */
struct btn_cfg_data {
	const char *name;
	const struct gpio_dt_spec gpio;
	void *user_cfg;
};

struct btn_unit_cfg {
	uint32_t num_buttons;
	struct btn_cfg_data *buttons;
};

#define BASE_10 10

/* Only allow one button msg at a time, as a mean of debounce */
K_MSGQ_DEFINE(button_q, sizeof(struct button_msg), 1, sizeof(void *));

static bool debounce_is_ongoing;

#if DT_NODE_EXISTS(DT_PATH(buttons))
#define BUTTON DT_PATH(buttons)
#else
#pragma error("No buttons node found")
#endif

#define BUTTON_NAME(b)                                                                             \
	((b) == BUTTON_VOLUME_DOWN  ? "BUTTON_VOLUME_DOWN"                                         \
	 : (b) == BUTTON_VOLUME_UP  ? "BUTTON_VOLUME_UP"                                           \
	 : (b) == BUTTON_PLAY_PAUSE ? "BUTTON_PLAY_PAUSE"                                          \
	 : (b) == BUTTON_4	    ? "BUTTON_4"                                                   \
	 : (b) == BUTTON_5	    ? "BUTTON_5"                                                   \
				    : NULL)

#define BUTTON_GPIO(button_node_id)                                                                \
	{                                                                                          \
		.name = BUTTON_NAME(DT_GPIO_PIN(button_node_id, gpios)),                           \
		.gpio = GPIO_DT_SPEC_GET(button_node_id, gpios),                                   \
	},

static struct btn_cfg_data btn_config_data[] = {DT_FOREACH_CHILD(BUTTON, BUTTON_GPIO)};

static const struct btn_unit_cfg btn_config = {.num_buttons = ARRAY_SIZE(btn_config_data),
					       .buttons = btn_config_data};

static struct gpio_callback btn_callback[ARRAY_SIZE(btn_config_data)];

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
	for (uint8_t i = 0; i < btn_config.num_buttons; i++) {
		if (btn_pin == btn_config.buttons[i].gpio.pin) {
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

static void button_publish_thread(void)
{
	int ret;
	struct button_msg msg;

	while (1) {
		k_msgq_get(&button_q, &msg, K_FOREVER);

		ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
		}
	}
}

K_THREAD_DEFINE(button_publish, CONFIG_BUTTON_PUBLISH_STACK_SIZE, button_publish_thread, NULL, NULL,
		NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_PUBLISH_THREAD_PRIO), 0, 0);

/*  ISR triggered by GPIO when assigned button(s) are pushed */
static void button_isr(const struct device *port, struct gpio_callback *cb, uint32_t pin_msk)
{
	int ret;
	struct button_msg msg;

	if (debounce_is_ongoing) {
		LOG_DBG("Btn debounce in action");
		return;
	}

	uint32_t btn_pin = 0;
	uint32_t btn_idx = 0;

	ret = pin_msk_to_pin(pin_msk, &btn_pin);
	ERR_CHK(ret);

	ret = pin_to_btn_idx(btn_pin, &btn_idx);
	ERR_CHK(ret);

	LOG_DBG("Pushed button idx: %d pin: %d name: %s", btn_idx, btn_pin,
		btn_config.buttons[btn_idx].name);

	msg.button_pin = btn_pin;
	msg.button_action = BUTTON_PRESS;

	ret = k_msgq_put(&button_q, (void *)&msg, K_NO_WAIT);
	if (ret == -EAGAIN) {
		LOG_WRN("Btn msg queue full");
	}

	debounce_is_ongoing = true;
	k_timer_start(&button_debounce_timer, K_MSEC(CONFIG_BUTTON_DEBOUNCE_MS), K_NO_WAIT);
}

int button_pressed(gpio_pin_t button_pin, bool *button_pressed)
{
	int ret;
	uint32_t btn_idx;

	if (button_pin == BUTTON_NOT_ASSIGNED || button_pressed == NULL) {
		return -EINVAL;
	}

	ret = pin_to_btn_idx(button_pin, &btn_idx);
	if (ret) {
		return -EINVAL;
	}

	if (!device_is_ready(btn_config.buttons[btn_idx].gpio.port)) {
		return -EINVAL;
	}

	ret = gpio_pin_get(btn_config.buttons[btn_idx].gpio.port, button_pin);
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

	if (btn_config.num_buttons == 0) {
		LOG_WRN("No buttons assigned");
		return -EINVAL;
	}

	for (size_t i = 0; i < btn_config.num_buttons; i++) {
		struct btn_cfg_data *button = &btn_config.buttons[i];

		if (device_is_ready(button->gpio.port)) {
			ret = gpio_pin_configure(button->gpio.port, button->gpio.pin,
						 GPIO_INPUT | button->gpio.dt_flags);
			if (ret) {
				LOG_ERR("Cannot configure GPIO (ret %d)", ret);
				return ret;
			}
		} else {
			LOG_ERR("GPIO device not ready");
			return -ENODEV;
		}

		gpio_init_callback(&btn_callback[i], button_isr, BIT(button->gpio.pin));

		ret = gpio_add_callback(button->gpio.port, &btn_callback[i]);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure(button->gpio.port, button->gpio.pin,
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

	for (uint8_t i = 0; i < btn_config.num_buttons; i++) {
		shell_print(shell, "Id %d: pin: %d %s", i, btn_config.buttons[i].gpio.pin,
			    btn_config.buttons[i].name);
	}

	return 0;
}

static int cmd_push_btn(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t btn_idx;
	struct button_msg msg;

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

	if (btn_idx >= ARRAY_SIZE(btn_config_data)) {
		shell_error(shell, "Selected button ID out of range");
		return -EINVAL;
	}

	msg.button_pin = btn_config.buttons[btn_idx].gpio.pin;
	msg.button_action = BUTTON_PRESS;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}

	shell_print(shell, "Pushed button idx: %d pin: %d : %s", btn_idx,
		    btn_config.buttons[btn_idx].gpio.pin, btn_config.buttons[btn_idx].name);

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
