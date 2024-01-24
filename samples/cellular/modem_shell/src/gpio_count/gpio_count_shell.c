/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>

#include "mosh_print.h"
#include "gpio_count.h"

static int cmd_gpio_count_enable(const struct shell *shell, size_t argc, char **argv)
{
	int gpio_pin;

	gpio_pin = atoi(argv[1]);
	if (gpio_pin < 0 || gpio_pin >= GPIO_MAX_PINS_PER_PORT) {
		mosh_error("Invalid pin value: %d", gpio_pin);

		return -EINVAL;
	}

	return gpio_count_enable(gpio_pin);
}

static int cmd_gpio_count_disable(const struct shell *shell, size_t argc, char **argv)
{
	return gpio_count_disable();
}

static int cmd_gpio_count_get(const struct shell *shell, size_t argc, char **argv)
{
	mosh_print("Number of pulses: %d", gpio_count_get());

	return 0;
}

static int cmd_gpio_count_reset(const struct shell *shell, size_t argc, char **argv)
{
	gpio_count_reset();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gpio_count,
	SHELL_CMD_ARG(
		enable, NULL,
		"<gpio pin number>\nEnable pulse counting for a GPIO pin.",
		cmd_gpio_count_enable, 2, 0),
	SHELL_CMD_ARG(
		disable, NULL,
		"Disable pulse counting.",
		cmd_gpio_count_disable, 1, 0),
	SHELL_CMD_ARG(
		get, NULL,
		"Get the number of pulses.",
		cmd_gpio_count_get, 1, 0),
	SHELL_CMD_ARG(
		reset, NULL,
		"Reset the number of pulses.",
		cmd_gpio_count_reset, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gpio_count, &sub_gpio_count, "Commands for GPIO pin pulse counter.",
		   mosh_print_help_shell);
