/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief IO Adapter CLI Application
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define MIN_BUTTON_TIME_MS  5

#define IOADAPTER_CHANNELS_NUMBER   3
#define BUTTONS_NUMBER_PER_CHANNEL  4
#define LEDS_NUMBER_PER_CHANNEL     4

#define IOADAPTER_CHANNEL_0  0
#define IOADAPTER_CHANNEL_1  1
#define IOADAPTER_CHANNEL_2  2

#define PRINT_STATUS(status, fmt, ...)  \
	shell_print(shell, "{\"status\": \"%s\", \"msg\": \"" fmt "\"}", status, ##__VA_ARGS__)

#define DISABLE_PIN(gpio, pin) gpio_pin_configure(gpio, pin, GPIO_DISCONNECTED)
#define ENABLE_BUTTON_PIN(gpio, pin) gpio_pin_configure(gpio, pin, GPIO_OUTPUT_INACTIVE)
#define ENABLE_LED_PIN(gpio, pin) gpio_pin_configure(gpio, pin, GPIO_INPUT)

/* IO Adapter channel to port/pin mapping */
typedef struct {
	uint8_t gpio_port;
	uint8_t gpio_pin;
} io_map;

/* IO Adapter channel to button mapping */
static const io_map button_map[IOADAPTER_CHANNELS_NUMBER][BUTTONS_NUMBER_PER_CHANNEL] = {
	[IOADAPTER_CHANNEL_0] = {
		{0, 11},  /* Button 0 */
		{0, 12},  /* Button 1 */
		{0, 24},  /* Button 2 */
		{0, 25}   /* Button 3 */
	},
	[IOADAPTER_CHANNEL_1] = {
		{1, 3},   /* Button 0 */
		{1, 4},   /* Button 1 */
		{1, 8},   /* Button 2 */
		{0, 3}    /* Button 3 */
	},
	[IOADAPTER_CHANNEL_2] = {
		{1, 13},  /* Button 0 */
		{1, 14},  /* Button 1 */
		{0, 27},  /* Button 2 */
		{1, 10}   /* Button 3 */
	}
};

/* IO Adapter channel to LED mapping */
static const io_map led_map[IOADAPTER_CHANNELS_NUMBER][LEDS_NUMBER_PER_CHANNEL] = {
	[IOADAPTER_CHANNEL_0] = {
		{0, 31},  /* LED 0 */
		{0, 4},   /* LED 1 */
		{0, 30},  /* LED 2 */
		{0, 28}   /* LED 3 */
	},
	[IOADAPTER_CHANNEL_1] = {
		{1, 2},   /* LED 0 */
		{1, 5},   /* LED 1 */
		{1, 1},   /* LED 2 */
		{1, 6}    /* LED 3 */
	},
	[IOADAPTER_CHANNEL_2] = {
		{1, 12},  /* LED 0 */
		{1, 15},  /* LED 1 */
		{1, 11},  /* LED 2 */
		{0, 2}    /* LED 3 */
	}
};

/* GPIO device declarations */
static const struct device *gpio_devs[2];

static int cmd_button_push(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3 || argc > 5) {
		shell_print(shell, "Usage: button push <ioadapter_channel> <button_number> "
			"[time_ms] [repeats]\n");
		return -EINVAL;
	}

	int channel = atoi(argv[1]);
	int button = atoi(argv[2]);
	int time_ms = (argc >= 4) ? atoi(argv[3]) : 500;
	int repeats = (argc == 5) ? atoi(argv[4]) : 1;

	if (channel < 0 || channel >= IOADAPTER_CHANNELS_NUMBER) {
		PRINT_STATUS("error", "Invalid channel number. Must be 0-%d",
			IOADAPTER_CHANNELS_NUMBER - 1);
		return -EINVAL;
	}

	if (button < 0 || button >= BUTTONS_NUMBER_PER_CHANNEL) {
		PRINT_STATUS("error", "Invalid button number. Must be 0-%d",
			BUTTONS_NUMBER_PER_CHANNEL - 1);
		return -EINVAL;
	}

	if (time_ms <= MIN_BUTTON_TIME_MS) {
		PRINT_STATUS("error", "Invalid time. Must be > %d", MIN_BUTTON_TIME_MS);
		return -EINVAL;
	}

	if (repeats < 1) {
		PRINT_STATUS("error", "Invalid repeats. Must be >= 1");
		return -EINVAL;
	}

	/* Get button mapping for the specified channel and button */
	const io_map *btn = &button_map[channel][button];
	const struct device *gpio = gpio_devs[btn->gpio_port];

	if (gpio == NULL) {
		PRINT_STATUS("error", "GPIO device not found for port %d", btn->gpio_port);
		return -ENODEV;
	}

	if (!device_is_ready(gpio)) {
		PRINT_STATUS("error", "GPIO device not ready for port %d", btn->gpio_port);
		return -ENODEV;
	}

	for (int i = 0; i < repeats; i++) {
		/* Press button (set to LOW) */
		int ret = ENABLE_BUTTON_PIN(gpio, btn->gpio_pin);

		if (ret < 0) {
			PRINT_STATUS("error", "Failed to press button on port %d pin %d",
					btn->gpio_port, btn->gpio_pin);
			return ret;
		}

		/* Wait for specified time */
		k_sleep(K_MSEC(time_ms));

		/* Release button (disconnect pin) */
		ret = DISABLE_PIN(gpio, btn->gpio_pin);
		if (ret < 0) {
			PRINT_STATUS("error", "Failed to release button on port %d pin %d",
					btn->gpio_port, btn->gpio_pin);
			return ret;
		}

		/* Add a small delay between repeats if not the last repeat */
		if (i < repeats - 1) {
			k_sleep(K_MSEC(time_ms));
		}
	}

	PRINT_STATUS("success", "BUTTON %d pressed (repeats: %d, press_time: %d ms, "
		"channel: %d, gpio: %d_%d)",
		button, repeats, time_ms, channel, btn->gpio_port, btn->gpio_pin);
	return 0;
}

static int cmd_button_hold(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, "Usage: button hold <ioadapter_channel> <button_number>\n");
		return -EINVAL;
	}

	int channel = atoi(argv[1]);
	int button = atoi(argv[2]);

	if (channel < 0 || channel >= IOADAPTER_CHANNELS_NUMBER) {
		PRINT_STATUS("error", "Invalid channel number. Must be 0-%d",
			IOADAPTER_CHANNELS_NUMBER - 1);
		return -EINVAL;
	}

	if (button < 0 || button >= BUTTONS_NUMBER_PER_CHANNEL) {
		PRINT_STATUS("error", "Invalid button number. Must be 0-%d",
			BUTTONS_NUMBER_PER_CHANNEL - 1);
		return -EINVAL;
	}

	/* Get button mapping for the specified channel and button */
	const io_map *btn = &button_map[channel][button];
	const struct device *gpio = gpio_devs[btn->gpio_port];

	if (gpio == NULL) {
		PRINT_STATUS("error", "GPIO device not found for port %d", btn->gpio_port);
		return -ENODEV;
	}

	if (!device_is_ready(gpio)) {
		PRINT_STATUS("error", "GPIO device not ready for port %d", btn->gpio_port);
		return -ENODEV;
	}

	/* Press button (set to LOW) */
	int ret = ENABLE_BUTTON_PIN(gpio, btn->gpio_pin);

	if (ret < 0) {
		PRINT_STATUS("error", "Failed to press button on port %d pin %d",
				btn->gpio_port, btn->gpio_pin);
		return ret;
	}

	PRINT_STATUS("success", "BUTTON %d is held (channel: %d, gpio: %d_%d)",
				button, channel, btn->gpio_port, btn->gpio_pin);
	return 0;
}

static int cmd_button_release(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, "Usage: button release <ioadapter_channel> <button_number>\n");
		return -EINVAL;
	}

	int channel = atoi(argv[1]);
	int button = atoi(argv[2]);

	if (channel < 0 || channel >= IOADAPTER_CHANNELS_NUMBER) {
		PRINT_STATUS("error", "Invalid channel number. Must be 0-%d",
			IOADAPTER_CHANNELS_NUMBER - 1);
		return -EINVAL;
	}

	if (button < 0 || button >= BUTTONS_NUMBER_PER_CHANNEL) {
		PRINT_STATUS("error", "Invalid button number. Must be 0-%d",
			BUTTONS_NUMBER_PER_CHANNEL - 1);
		return -EINVAL;
	}

	/* Get button mapping for the specified channel and button */
	const io_map *btn = &button_map[channel][button];
	const struct device *gpio = gpio_devs[btn->gpio_port];

	if (gpio == NULL) {
		PRINT_STATUS("error", "GPIO device not found for port %d", btn->gpio_port);
		return -ENODEV;
	}

	if (!device_is_ready(gpio)) {
		PRINT_STATUS("error", "GPIO device not ready for port %d", btn->gpio_port);
		return -ENODEV;
	}

	/* Release button (disconnect pin) */
	int ret = DISABLE_PIN(gpio, btn->gpio_pin);

	if (ret < 0) {
		PRINT_STATUS("error", "Failed to release button on port %d pin %d",
				btn->gpio_port, btn->gpio_pin);
		return ret;
	}

	PRINT_STATUS("success", "BUTTON %d released (channel: %d, gpio: %d_%d)",
				button, channel, btn->gpio_port, btn->gpio_pin);
	return 0;
}

static int cmd_led_state(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, "Usage: led <ioadapter_channel> <led_number>");
		return -EINVAL;
	}

	int channel = atoi(argv[1]);
	int led = atoi(argv[2]);

	if (channel < 0 || channel >= IOADAPTER_CHANNELS_NUMBER) {
		PRINT_STATUS("error", "Invalid channel number. Must be 0-%d",
			IOADAPTER_CHANNELS_NUMBER - 1);
		return -EINVAL;
	}

	if (led < 0 || led >= LEDS_NUMBER_PER_CHANNEL) {
		PRINT_STATUS("error", "Invalid LED number. Must be 0-%d",
			LEDS_NUMBER_PER_CHANNEL - 1);
		return -EINVAL;
	}

	/* Get LED mapping for the specified channel and LED */
	const io_map *led_map_entry = &led_map[channel][led];
	const struct device *gpio = gpio_devs[led_map_entry->gpio_port];

	if (gpio == NULL) {
		PRINT_STATUS("error", "GPIO device not found for port %d",
			led_map_entry->gpio_port);
		return -ENODEV;
	}

	if (!device_is_ready(gpio)) {
		PRINT_STATUS("error", "GPIO device not ready for port %d",
			led_map_entry->gpio_port);
		return -ENODEV;
	}

	int state;
	int ret = ENABLE_LED_PIN(gpio, led_map_entry->gpio_pin);

	if (ret < 0) {
		PRINT_STATUS("error", "Failed to enable LED pin");
		return ret;
	}

	ret = gpio_pin_get(gpio, led_map_entry->gpio_pin);
	if (ret < 0) {
		PRINT_STATUS("error", "Failed to read LED state");
		return ret;
	}
	state = ret;

	ret = DISABLE_PIN(gpio, led_map_entry->gpio_pin);
	if (ret < 0) {
		PRINT_STATUS("error", "Failed to disable LED pin");
		return ret;
	}

	shell_print(shell, "{\"status\": \"success\", "
				"\"result\": \"%s\", "
				"\"msg\": \"LED %d %s (channel: %d, gpio: %d_%d)\"}",
				state ? "off" : "on",
				led,
				state ? "OFF" : "ON",
				channel,
				led_map_entry->gpio_port,
				led_map_entry->gpio_pin);
	return 0;
}

static int cmd_io_map(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "\nIO Adapter Mapping:");
	shell_print(shell, "==================\n");

	for (int channel = 0; channel < IOADAPTER_CHANNELS_NUMBER; channel++) {
		shell_print(shell, "Channel %d:", channel);
		shell_print(shell, "----------");

		/* Print Buttons */
		shell_print(shell, "Buttons (Outputs):");
		for (int btn = 0; btn < BUTTONS_NUMBER_PER_CHANNEL; btn++) {
			const io_map *btn_map = &button_map[channel][btn];

			shell_print(shell, "  Button %d: Port %d, Pin %d",
				btn, btn_map->gpio_port, btn_map->gpio_pin);
		}

		/* Print LEDs */
		shell_print(shell, "LEDs (Inputs):");
		for (int led = 0; led < LEDS_NUMBER_PER_CHANNEL; led++) {
			const io_map *led_map_entry = &led_map[channel][led];

			shell_print(shell, "  LED %d: Port %d, Pin %d",
				led, led_map_entry->gpio_port, led_map_entry->gpio_pin);
		}
		shell_print(shell, "");
	}
	return 0;
}

static int cmd_blink(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_print(shell, "Usage: blink <ioadapter_channel> <led_number> <time_ms>");
		return -EINVAL;
	}

	int channel = atoi(argv[1]);
	int led = atoi(argv[2]);
	int time_ms = atoi(argv[3]);

	if (channel < 0 || channel >= IOADAPTER_CHANNELS_NUMBER) {
		PRINT_STATUS("error", "Invalid channel number. Must be 0-%d",
			IOADAPTER_CHANNELS_NUMBER - 1);
		return -EINVAL;
	}

	if (led < 0 || led >= LEDS_NUMBER_PER_CHANNEL) {
		PRINT_STATUS("error", "Invalid LED number. Must be 0-%d",
			LEDS_NUMBER_PER_CHANNEL - 1);
		return -EINVAL;
	}

	if (time_ms <= 5) {
		PRINT_STATUS("error", "Invalid time. Must be > 5");
		return -EINVAL;
	}

	/* Get LED mapping for the specified channel and LED */
	const io_map *led_map_entry = &led_map[channel][led];

	const struct device *gpio = gpio_devs[led_map_entry->gpio_port];

	if (gpio == NULL) {
		PRINT_STATUS("error", "GPIO device not found for port %d",
			led_map_entry->gpio_port);
		return -ENODEV;
	}

	if (!device_is_ready(gpio)) {
		PRINT_STATUS("error", "GPIO device not ready for port %d",
			led_map_entry->gpio_port);
		return -ENODEV;
	}

	int state_changes = 0;
	int last_state = -1;
	int64_t start_time = k_uptime_get();
	int64_t end_time = start_time + time_ms;

	int ret = ENABLE_LED_PIN(gpio, led_map_entry->gpio_pin);

	if (ret < 0) {
		PRINT_STATUS("error", "Failed to enable LED pin");
		return ret;
	}

	while (k_uptime_get() < end_time) {
		int current_state = gpio_pin_get(gpio, led_map_entry->gpio_pin);

		if (current_state < 0) {
			PRINT_STATUS("error", "Failed to read LED state");
			return current_state;
		}

		if (last_state != -1 && current_state != last_state) {
			state_changes++;
		}
		last_state = current_state;

		/* Small delay to prevent CPU hogging */
		k_sleep(K_MSEC(5));
	}

	ret = DISABLE_PIN(gpio, led_map_entry->gpio_pin);
	if (ret < 0) {
		PRINT_STATUS("error", "Failed to disable LED pin");
		return ret;
	}

	shell_print(shell,
				"{\"status\": \"success\", "
				"\"result\": \"%d\", "
				"\"msg\": \"LED %d changed state %d times "
				"(time: %d ms, channel: %d, gpio: %d_%d)\"}",
				state_changes,
				led,
				state_changes,
				time_ms,
				channel,
				led_map_entry->gpio_port,
				led_map_entry->gpio_pin);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_button,
	SHELL_CMD(push, NULL,
		"Push a button on the given port\n"
		"Usage: button push <ioadapter_channel> <button_number> [time_ms] [repeats]\n"
		"- ioadapter_channel: IO adapter channel (0-2)\n"
		"- button: Button number (0-3)\n"
		"- time_ms: Press duration in milliseconds (optional, defaults to 500ms, must be > 5ms)\n"
		"- repeats: Number of times to push the button (optional, defaults to 1, must be >= 1)",
		cmd_button_push),
	SHELL_CMD(hold, NULL,
		"Hold a button on the given port\n"
		"Usage: button hold <ioadapter_channel> <button_number>\n"
		"- ioadapter_channel: IO adapter channel (0-2)\n"
		"- button: Button number (0-3)",
		cmd_button_hold),
	SHELL_CMD(release, NULL,
		"Release a button on the given port\n"
		"Usage: button release <ioadapter_channel> <button_number>\n"
		"- ioadapter_channel: IO adapter channel (0-2)\n"
		"- button: Button number (0-3)",
		cmd_button_release),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(button, &sub_button, "Button control commands", NULL);

SHELL_CMD_REGISTER(led, NULL,
	"Read LED state on the given port\n"
	"Usage: led <ioadapter_channel> <led_number>\n"
	"- ioadapter_channel: IO adapter channel (0-2)\n"
	"- led_number: LED number (0-3)",
	cmd_led_state);

SHELL_CMD_REGISTER(mapping, NULL,
	"Print IO adapter mapping\n"
	"Usage: mapping",
	cmd_io_map);

SHELL_CMD_REGISTER(blink, NULL,
	"Monitor LED state changes over time\n"
	"Usage: blink <ioadapter_channel> <led_number> <time_ms>\n"
	"- ioadapter_channel: IO adapter channel (0-2)\n"
	"- led_number: LED number (0-3)\n"
	"- time_ms: Monitoring duration in milliseconds (must be > 5)",
	cmd_blink);

int main(void)
{
	/* Initialize GPIO devices */
	gpio_devs[0] = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	gpio_devs[1] = DEVICE_DT_GET(DT_NODELABEL(gpio1));

	if (!device_is_ready(gpio_devs[0]) || !device_is_ready(gpio_devs[1])) {
		LOG_ERR("Error: GPIO devices not ready");
		return -ENODEV;
	}

	return 0;
}
