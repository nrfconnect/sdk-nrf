/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <dk_buttons_and_leds.h>

#define RUN_STATUS_LED DK_LED1
#define SHELL_LED      DK_LED2

#define STATUS_LED_INTERVAL K_MSEC(1000)

void main(void)
{
	uint32_t blink_status = 0;
	int err;

	printk("Starting NFC shell example\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(STATUS_LED_INTERVAL);
	}
}

static int led_on_handler(const struct shell *shell, size_t argc, char **argv)
{
	int err = dk_set_led_on(SHELL_LED);

	if (err) {
		shell_error(shell, "Failed to turn on the LED, (err: %d)", err);
		return err;
	}

	shell_print(shell, "The LED is on");

	return 0;
}

static int led_off_handler(const struct shell *shell, size_t argc, char **argv)
{
	int err = dk_set_led_off(SHELL_LED);

	if (err) {
		shell_error(shell, "Failed to turn off the LED, (err: %d)", err);
		return err;
	}

	shell_print(shell, "The LED is off");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	led_subcmd,
	SHELL_CMD(on, NULL, "Turn on DK LED", led_on_handler),
	SHELL_CMD(off, NULL, "Turn off DK LED", led_off_handler),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(led, &led_subcmd, "Control DK LED", NULL);
