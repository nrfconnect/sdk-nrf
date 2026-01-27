/*
 * Copyright (c) 2020-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <openthread/cli.h>

#include "radio_test.h"

#if NRF_POWER_HAS_DCDCEN_VDDH
#define TOGGLE_DCDC_HELP                                                                           \
	"Toggle DCDC state <state>, "                                                              \
	"if state = 1 then toggle DC/DC state, or "                                                \
	"if state = 0 then toggle DC/DC VDDH state"
#elif NRF_POWER_HAS_DCDCEN
#define TOGGLE_DCDC_HELP                                                                           \
	"Toggle DCDC state <state>, "                                                              \
	"Toggle DC/DC state regardless of state value"
#endif

#define REGULATOR_MODE_HELP "Set regulator mode <ldo|dcdc>"

#define shell_print(fmt, ...)	otCliOutputFormat(fmt "\r\n" __VA_OPT__(, ) __VA_ARGS__)
#define shell_hexdump(ptr, len) otCliOutputBytes(ptr, len), otCliOutputFormat("\r\n")

/* Subcommand definition */
struct radio_test_cmd {
	const char *name;
	const char *help;
	otError (*handler)(void *ctx, uint8_t argc, char *argv[]);
};

/* Execute one of the subcommands */
otError exec_subcommand(struct radio_test_cmd cmd[], size_t num, void *ctx, uint8_t argc,
			char *argv[])
{
	if (argc == 0 || strcmp(argv[0], "help") == 0) {
		for (size_t i = 0; i < num; i++) {
			otCliOutputFormat("%-30s : %s\r\n", cmd[i].name, cmd[i].help);
		}

		return argc == 0 ? OT_ERROR_INVALID_ARGS : OT_ERROR_NONE;
	}

	for (size_t i = 0; i < num; i++) {
		if (strcmp(argv[0], cmd[i].name) == 0) {
			return cmd[i].handler(ctx, argc - 1, argv + 1);
		}
	}

	return OT_ERROR_INVALID_ARGS;
}

/* Radio parameter configuration. */
static struct radio_param_config {
	/** Radio transmission pattern. */
	enum transmit_pattern tx_pattern;

	/** Radio mode. Data rate and modulation. */
	nrf_radio_mode_t mode;

	/** Radio output power. */
	int8_t txpower;

	/** Radio start channel (frequency). */
	uint8_t channel_start;

	/** Radio end channel (frequency). */
	uint8_t channel_end;

	/** Delay time in milliseconds. */
	uint32_t delay_ms;

	/** Duty cycle. */
	uint32_t duty_cycle;

	/** Interval. */
	uint32_t interval;

	bool crc_enabled;
} config = {
	.tx_pattern = TRANSMIT_PATTERN_RANDOM,
	.mode = NRF_RADIO_MODE_BLE_1MBIT,
	.txpower = 0,
	.channel_start = 0,
	.channel_end = 80,
	.delay_ms = 10,
	.duty_cycle = 50,
	.crc_enabled = true,
	.interval = 100,
};

/* Radio test configuration. */
static struct radio_test_config test_config;

/* If true, RX sweep, TX sweep or duty cycle test is performed. */
static bool test_in_progress;

/* If true, RX test is performed. */
static bool rx_test_in_progress;

K_SEM_DEFINE(cpu_activity_thread_start, 0, 1);
volatile uint8_t cpu_activity;
volatile uint8_t cpu_activity_duty_cycle;
volatile uint8_t cpu_active;

static void cpu_duty_cycle_timer_handler(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);
	cpu_active = false;
}

K_TIMER_DEFINE(cpu_duty_cycle_timer, cpu_duty_cycle_timer_handler, NULL);

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
static void ieee_channel_check(uint8_t channel)
{
	if (config.mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		if ((channel < IEEE_MIN_CHANNEL) || (channel > IEEE_MAX_CHANNEL)) {
			shell_print("For %s config.mode channel must be between %d and %d",
				    STRINGIFY(NRF_RADIO_MODE_IEEE802154_250KBIT), IEEE_MIN_CHANNEL,
					      IEEE_MAX_CHANNEL);

			shell_print("Channel set to %d", IEEE_MIN_CHANNEL);
		}
	}
}
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

static otError cmd_gpio_set(void *ctx, uint8_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	uint8_t gpio_port = atoi(argv[0]);
	uint8_t gpio_pin = atoi(argv[1]);
	if (gpio_port > 2) {
		shell_print("Invalid GPIO port: %d. Should be between 0-2", gpio_port);
		return OT_ERROR_INVALID_ARGS;
	}
	if (gpio_pin > 31) {
		shell_print("Invalid GPIO pin: %d. Should be between 0-31", gpio_pin);
		return OT_ERROR_INVALID_ARGS;
	}
	uint32_t gpio_pin_port_map = NRF_GPIO_PIN_MAP(gpio_port, gpio_pin);
	nrf_gpio_cfg_output(gpio_pin_port_map);
	nrf_gpio_pin_set(gpio_pin_port_map);
	shell_print("GPIO P%d.%d set to logic high", gpio_port, gpio_pin);
	return OT_ERROR_NONE;
}

static otError cmd_gpio_clear(void *ctx, uint8_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	uint8_t gpio_port = atoi(argv[0]);
	uint8_t gpio_pin = atoi(argv[1]);
	if (gpio_port > 2) {
		shell_print("Invalid GPIO port: %d. Should be between 0-2", gpio_port);
		return OT_ERROR_INVALID_ARGS;
	}
	if (gpio_pin > 31) {
		shell_print("Invalid GPIO pin: %d. Should be between 0-31", gpio_pin);
		return OT_ERROR_INVALID_ARGS;
	}
	uint32_t gpio_pin_port_map = NRF_GPIO_PIN_MAP(gpio_port, gpio_pin);
	nrf_gpio_cfg_output(gpio_pin_port_map);
	nrf_gpio_pin_clear(gpio_pin_port_map);
	shell_print("GPIO P%d.%d cleared to logic low", gpio_port, gpio_pin);
	return OT_ERROR_NONE;
}

static otError cmd_gpio_get(void *ctx, uint8_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	uint8_t gpio_port = atoi(argv[0]);
	uint8_t gpio_pin = atoi(argv[1]);
	if (gpio_port > 2) {
		shell_print("Invalid GPIO port: %d. Should be between 0-2", gpio_port);
		return OT_ERROR_INVALID_ARGS;
	}
	if (gpio_pin > 31) {
		shell_print("Invalid GPIO pin: %d. Should be between 0-31", gpio_pin);
		return OT_ERROR_INVALID_ARGS;
	}
	uint32_t gpio_pin_port_map = NRF_GPIO_PIN_MAP(gpio_port, gpio_pin);
	uint32_t gpio_out_state = nrf_gpio_pin_out_read(gpio_pin_port_map);
	shell_print("State of GPIO P%d.%02d: %d", gpio_port, gpio_pin, gpio_out_state);
	return OT_ERROR_NONE;
}

static otError cmd_print_gpio(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"set", "Set GPIO to logic high <port> <pin>", cmd_gpio_set},
		{"clear", "Clear GPIO to logic low <port> <pin>", cmd_gpio_clear},
		{"get", "Get current logic level of GPIO <port> <pin>", cmd_gpio_get},
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

__attribute__((section(".ramfunc"))) void cpu_work_ram(uint8_t duty_cycle)
{
	volatile int64_t i = 0;
	while (1) {
		if (duty_cycle == 100) {
			for (int j = 0; j < 50000; j++) {
				i++;
				i = i * 2;
				i = i / 2;
				i--;
				i++;
			}
			k_yield(); /* Yield to allow other threads to run */
		} else {
			uint32_t sleep_time_us = (100 - cpu_activity_duty_cycle) * 200;
			uint32_t work_time_us = cpu_activity_duty_cycle * 200;

			cpu_active = true;
			k_timer_start(&cpu_duty_cycle_timer, K_USEC(work_time_us), K_NO_WAIT);

			while (cpu_active) {
				i++;
				i = i * 2;
				i = i / 2;
				i--;
				i++;
				k_yield(); /* Yield to allow other threads to run */
			}

			k_sleep(K_USEC(sleep_time_us));
		}

		if (cpu_activity != 1) {
			return;
		}
	}
}

void cpu_work_rram(uint8_t duty_cycle)
{
	volatile int64_t i = 0;
	while (1) {
		if (duty_cycle == 100) {
			for (int j = 0; j < 50000; j++) {
				i++;
				i = i * 2;
				i = i / 2;
				i--;
				i++;
			}
			k_yield(); /* Yield to allow other threads to run */
		} else {
			uint32_t sleep_time_us = (100 - cpu_activity_duty_cycle) * 200;
			uint32_t work_time_us = cpu_activity_duty_cycle * 200;

			cpu_active = true;
			k_timer_start(&cpu_duty_cycle_timer, K_USEC(work_time_us), K_NO_WAIT);

			while (cpu_active) {
				i++;
				i = i * 2;
				i = i / 2;
				i--;
				i++;
				k_yield(); /* Yield to allow other threads to run */
			}

			k_sleep(K_USEC(sleep_time_us));
		}

		if (cpu_activity != 2) {
			return;
		}
	}
}

void cpu_activity_thread(void)
{
	/* Don't go any further until command to enable CPU activity is given */
	while (1) {
		if (cpu_activity == 0) { // IDLE
			k_sem_take(&cpu_activity_thread_start, K_FOREVER);
		} else if (cpu_activity == 1) { // Blocking RAM
			cpu_work_ram(cpu_activity_duty_cycle);
		} else if (cpu_activity == 2) { // Blocking RRAM
			cpu_work_rram(cpu_activity_duty_cycle);
		}
	}
}

K_THREAD_DEFINE(cpu_activity_thread_id, 1024, cpu_activity_thread, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static otError cmd_cpu_activity_idle(void *ctx, uint8_t argc, char *argv[])
{
	cpu_activity = 0;
	return OT_ERROR_NONE;
}

static otError cmd_cpu_activity_blocking_ram(void *ctx, uint8_t argc, char *argv[])
{
	cpu_activity_duty_cycle = 100;
	cpu_activity = 1;
	k_sem_give(&cpu_activity_thread_start);
	return OT_ERROR_NONE;
}

static otError cmd_cpu_activity_blocking_rram(void *ctx, uint8_t argc, char *argv[])
{
	cpu_activity_duty_cycle = 100;
	cpu_activity = 2;
	k_sem_give(&cpu_activity_thread_start);
	return OT_ERROR_NONE;
}

static otError cmd_cpu_activity_blocking_ram_duty_cycle(void *ctx, uint8_t argc, char *argv[])
{
	int duty_cycle;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	duty_cycle = atoi(argv[0]);

	if (duty_cycle < 10 || duty_cycle > 90) {
		shell_print("Invalid duty cycle: %d. Should be between 10-90 (%%)", duty_cycle);
		return -EINVAL;
	}

	cpu_activity_duty_cycle = duty_cycle;
	cpu_activity = 1;
	k_sem_give(&cpu_activity_thread_start);

	return OT_ERROR_NONE;
}

static otError cmd_cpu_activity_blocking_rram_duty_cycle(void *ctx, uint8_t argc, char *argv[])
{
	int duty_cycle;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	duty_cycle = atoi(argv[0]);

	if (duty_cycle < 10 || duty_cycle > 90) {
		shell_print("Invalid duty cycle: %d. Should be between 10-90 (%%)", duty_cycle);
		return -EINVAL;
	}

	cpu_activity_duty_cycle = duty_cycle;
	cpu_activity = 2;
	k_sem_give(&cpu_activity_thread_start);

	return OT_ERROR_NONE;
}

static otError cmd_cpu_activity_set(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"idle", "CPU goes to sleep whenever it can (default)", cmd_cpu_activity_idle},
		{"ram_function", "CPU executes a function from RAM doing calculations",
		 cmd_cpu_activity_blocking_ram},
		{"rram_function", "CPU executes a function from RRAM (NVM) doing calculations",
		 cmd_cpu_activity_blocking_rram},
		{"ram_duty_cycle",
		 "CPU switches between executing a function from RAM doing calculations and going "
		 "to sleep",
		 cmd_cpu_activity_blocking_ram_duty_cycle},
		{"rram_duty_cycle",
		 "CPU switches between executing a function from RRAM (NVM) doing calculations and "
		 "going to sleep",
		 cmd_cpu_activity_blocking_rram_duty_cycle},
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

static otError cmd_crc_enable(void *ctx, uint8_t argc, char *argv[])
{
	shell_print("CRC enabled in packets");

	config.crc_enabled = true;

	return OT_ERROR_NONE;
}

static otError cmd_crc_disable(void *ctx, uint8_t argc, char *argv[])
{
	shell_print("CRC disabled in packets");

	config.crc_enabled = false;

	return OT_ERROR_NONE;
}

static otError cmd_print_crc(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"enable", "Enable CRC in packets", cmd_crc_enable},
		{"disable", "Disable CRC in packets", cmd_crc_disable},
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

static otError cmd_start_channel_set(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t channel;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	channel = atoi(argv[0]);

	if (channel > 80) {
		shell_print("Channel must be between 0 and 80");
		return OT_ERROR_INVALID_ARGS;
	}

	config.channel_start = (uint8_t)channel;

	shell_print("Start channel set to: %d", channel);
	return OT_ERROR_NONE;
}

static otError cmd_end_channel_set(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t channel;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	channel = atoi(argv[0]);

	if (channel > 80) {
		shell_print("Channel must be between 0 and 80");
		return OT_ERROR_INVALID_ARGS;
	}

	config.channel_end = (uint8_t)channel;

	shell_print("End channel set to: %d", channel);
	return OT_ERROR_NONE;
}

static otError cmd_time_set(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t time;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	time = atoi(argv[0]);

	if (time > 99) {
		shell_print("Delay time must be between 0 and 99 ms");
		return OT_ERROR_INVALID_ARGS;
	}

	config.delay_ms = time;

	shell_print("Delay time set to: %d", time);
	return OT_ERROR_NONE;
}

static otError cmd_cancel(void *ctx, uint8_t argc, char *argv[])
{
	radio_test_cancel();
	test_in_progress = false;
	rx_test_in_progress = false;
	return OT_ERROR_NONE;
}

static otError cmd_nrf_1mbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_NRF_1MBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_1MBIT));

	return OT_ERROR_NONE;
}

static otError cmd_nrf_2mbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_NRF_2MBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_2MBIT));

	return OT_ERROR_NONE;
}

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
static otError cmd_nrf_250kbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_NRF_250KBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_250KBIT));

	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_5)
static otError cmd_nrf_4mbit_h_0_5(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_NRF_4MBIT_H_0_5;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_4MBIT_H_0_5));

	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_5) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_25)
static otError cmd_nrf_4mbit_h_0_25(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_NRF_4MBIT_H_0_25;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_4MBIT_H_0_25));

	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_25) */

static otError cmd_ble_1mbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_BLE_1MBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_1MBIT));

	return OT_ERROR_NONE;
}

static otError cmd_ble_2mbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_BLE_2MBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_2MBIT));

	return OT_ERROR_NONE;
}

#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
static otError cmd_ble_lr125kbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_BLE_LR125KBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_LR125KBIT));

	return OT_ERROR_NONE;
}

static otError cmd_ble_lr500kbit(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_BLE_LR500KBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_LR500KBIT));

	return OT_ERROR_NONE;
}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
static otError cmd_ble_ieee(void *ctx, uint8_t argc, char *argv[])
{
	config.mode = NRF_RADIO_MODE_IEEE802154_250KBIT;
	shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_IEEE802154_250KBIT));

	return OT_ERROR_NONE;
}
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

static otError cmd_data_rate_set(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"nrf_1Mbit", "1 Mbit/s Nordic proprietary radio mode", cmd_nrf_1mbit},
		{"nrf_2Mbit", "2 Mbit/s Nordic proprietary radio mode", cmd_nrf_2mbit},
#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
		{"nrf_250Kbit", "250 kbit/s Nordic proprietary radio mode", cmd_nrf_250kbit},
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */
#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_5)
		{"nrf_4Mbit0_5", "4 Mbit/s Nordic proprietary radio mode (BT=0.5/h=0.5)",
		 cmd_nrf_4mbit_h_0_5},
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_5) */
#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_25)
		{"nrf_4Mbit0_25", NULL, "4 Mbit/s Nordic proprietary radio mode (BT=0.5/h=0.25)",
		 cmd_nrf_4mbit_h_0_25},
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_25) */
		{"ble_1Mbit", "1 Mbit/s Bluetooth Low Energy", cmd_ble_1mbit},
		{"ble_2Mbit", "2 Mbit/s Bluetooth Low Energy", cmd_ble_2mbit},
#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
		{"ble_lr125Kbit", "Long range 125 kbit/s TX, 125 kbit/s and 500 kbit/s RX",
		 cmd_ble_lr125kbit},
		{"ble_lr500Kbit", "Long range 500 kbit/s TX, 125 kbit/s and 500 kbit/s RX",
		 cmd_ble_lr500kbit},
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
		{"ieee802154_250Kbit", "IEEE 802.15.4-2006 250 kbit/s", cmd_ble_ieee},
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

static otError cmd_tx_carrier_start(void *ctx, uint8_t argc, char *argv[])
{
	if (test_in_progress) {
		radio_test_cancel();
		test_in_progress = false;
	}

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	ieee_channel_check(config.channel_start);
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	memset(&test_config, 0, sizeof(test_config));
	test_config.type = UNMODULATED_TX;
	test_config.mode = config.mode;
	test_config.params.unmodulated_tx.txpower = config.txpower;
	test_config.params.unmodulated_tx.channel = config.channel_start;
	test_config.crc_enabled = config.crc_enabled;
	radio_test_start(&test_config);

	shell_print("Start the TX carrier");
	return OT_ERROR_NONE;
}

static void tx_modulated_carrier_end(void)
{
	printk("The modulated TX has finished\n");
}

static otError cmd_tx_modulated_carrier_start(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t tx_payload_length = RADIO_MAX_PAYLOAD_LEN;

	if (test_in_progress) {
		radio_test_cancel();
		test_in_progress = false;
	}

	memset(&test_config, 0, sizeof(test_config));

	if (argc > 2) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	if (argc >= 1) {
		test_config.params.modulated_tx.packets_num = atoi(argv[0]);
		test_config.params.modulated_tx.cb = tx_modulated_carrier_end;
	}

	if (argc == 2) {
		tx_payload_length = atoi(argv[1]);

		if (tx_payload_length > RADIO_MAX_PAYLOAD_LEN) {
			shell_print("Payload size can be max %d bytes", RADIO_MAX_PAYLOAD_LEN);
			shell_print("Payload set to %d", RADIO_MAX_PAYLOAD_LEN);
			tx_payload_length = RADIO_MAX_PAYLOAD_LEN;
		}
	}

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	if (config.mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		if (tx_payload_length > IEEE_MAX_PAYLOAD_LEN) {
			shell_print("Payload size can be max %d bytes in IEEE802.15.4 mode",
				    IEEE_MAX_PAYLOAD_LEN);

			shell_print("Payload set to %d", IEEE_MAX_PAYLOAD_LEN);
			tx_payload_length = IEEE_MAX_PAYLOAD_LEN;
		}
	}
	ieee_channel_check(config.channel_start);
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	test_config.type = MODULATED_TX;
	test_config.mode = config.mode;
	test_config.params.modulated_tx.txpower = config.txpower;
	test_config.params.modulated_tx.channel = config.channel_start;
	test_config.params.modulated_tx.pattern = config.tx_pattern;
	test_config.params.modulated_tx.payload_size = tx_payload_length;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);

	shell_print("Start the modulated TX carrier");
	return OT_ERROR_NONE;
}

static otError cmd_duty_cycle_set(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t duty_cycle;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	duty_cycle = atoi(argv[0]);

	if (duty_cycle > 90) {
		shell_print("Duty cycle must be between 1 and 90");
		return OT_ERROR_INVALID_ARGS;
	}

	config.duty_cycle = duty_cycle;

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	ieee_channel_check(config.channel_start);
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	memset(&test_config, 0, sizeof(test_config));
	test_config.type = MODULATED_TX_DUTY_CYCLE;
	test_config.mode = config.mode;
	test_config.params.modulated_tx_duty_cycle.txpower = config.txpower;
	test_config.params.modulated_tx_duty_cycle.pattern = config.tx_pattern;
	test_config.params.modulated_tx_duty_cycle.channel = config.channel_start;
	test_config.params.modulated_tx_duty_cycle.duty_cycle = config.duty_cycle;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);
	test_in_progress = true;

	return OT_ERROR_NONE;
}

static otError cmd_interval_set(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t interval;
	uint32_t tx_payload_length = RADIO_MAX_PAYLOAD_LEN;

	if (argc != 1 && argc != 2) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	interval = atoi(argv[0]);

	if (interval < 1 || interval > MODULATED_TX_INTERVAL_MAX_MS) {
		shell_print("Interval must be between 1 and %d", MODULATED_TX_INTERVAL_MAX_MS);
		return OT_ERROR_INVALID_ARGS;
	}

	config.interval = interval;

	if (argc == 2) {
		tx_payload_length = atoi(argv[1]);

		if (tx_payload_length > RADIO_MAX_PAYLOAD_LEN) {
			shell_print("Payload size can be max %d bytes", RADIO_MAX_PAYLOAD_LEN);
			shell_print("Payload set to %d", RADIO_MAX_PAYLOAD_LEN);
			tx_payload_length = RADIO_MAX_PAYLOAD_LEN;
		}
	}

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	if (config.mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		if (tx_payload_length > IEEE_MAX_PAYLOAD_LEN) {
			shell_print("Payload size can be max %d bytes in IEEE802.15.4 mode",
				    IEEE_MAX_PAYLOAD_LEN);

			shell_print("Payload set to %d", IEEE_MAX_PAYLOAD_LEN);
			tx_payload_length = IEEE_MAX_PAYLOAD_LEN;
		}
	}
	ieee_channel_check(config.channel_start);
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	memset(&test_config, 0, sizeof(test_config));
	test_config.type = MODULATED_TX_INTERVAL;
	test_config.mode = config.mode;
	test_config.params.modulated_tx_interval.txpower = config.txpower;
	test_config.params.modulated_tx_interval.pattern = config.tx_pattern;
	test_config.params.modulated_tx_interval.channel = config.channel_start;
	test_config.params.modulated_tx_interval.interval = config.interval;
	test_config.params.modulated_tx_interval.payload_size = tx_payload_length;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);
	test_in_progress = true;

	return OT_ERROR_NONE;
}

#if defined(TOGGLE_DCDC_HELP)
static otError cmd_toggle_dc(void *ctx, uint8_t argc, char *argv[])
{
	uint32_t state;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	state = atoi(argv[0]);
	if (state > 1) {
		shell_print("Invalid DCDC value provided");
		return OT_ERROR_INVALID_ARGS;
	}

	toggle_dcdc_state((uint8_t)state);

#if NRF_POWER_HAS_DCDCEN_VDDH
	shell_print("DCDC VDDH state %d\n"
		    "Write '0' to toggle state of DCDC REG0\n"
		    "Write '1' to toggle state of DCDC REG1",
		    nrf_power_dcdcen_vddh_get(NRF_POWER));
#endif /* NRF_POWER_HAS_DCDCEN_VDDH */

#if NRF_POWER_HAS_DCDCEN
	shell_print("DCDC state %d\n"
		    "Write '1' or '0' to toggle",
		    nrf_power_dcdcen_get(NRF_POWER));
#endif /* NRF_POWER_HAS_DCDCEN */

	return OT_ERROR_NONE;
}
#endif

#if defined(CONFIG_MPSL_FEM_HOT_POTATO_REGULATOR_CONTROL_MANUAL)
static int regulator_mode_handle(enum radio_test_regulator_mode mode)
{
	if (!NRF_REGULATORS_HAS_VREG_ANY && !NRF_POWER_HAS_DCDCEN_VDDH && !NRF_POWER_HAS_DCDCEN) {
		shell_print("Regulator control not supported on this SoC");
		return -ENOTSUP;
	}

	if (test_in_progress) {
		shell_print("Stop radio test before changing regulator mode");
		return -EBUSY;
	}

	radio_test_regulator_mode_set(mode);

	shell_print("Regulator mode request: %s",
		    (mode == RADIO_TEST_REGULATOR_MODE_DCDC) ? "dcdc" : "ldo");

#if NRF_REGULATORS_HAS_VREG_MAIN
	shell_print("VREGMAIN DCDC state %d",
		    nrf_regulators_vreg_enable_check(NRF_REGULATORS, NRF_REGULATORS_VREG_MAIN));
#endif /* NRF_REGULATORS_HAS_VREG_MAIN */

#if NRF_REGULATORS_HAS_VREG_HIGH
	shell_print("VREGH DCDC state %d",
		    nrf_regulators_vreg_enable_check(NRF_REGULATORS, NRF_REGULATORS_VREG_HIGH));
#endif /* NRF_REGULATORS_HAS_VREG_HIGH */

#if NRF_REGULATORS_HAS_MAIN_STATUS
	shell_print("MAINREGSTATUS %s", (nrf_regulators_main_status_get(NRF_REGULATORS) ==
					 NRF_REGULATORS_MAIN_STATUS_HIGH)
						? "high (VDDH)"
						: "normal (VDD)");
#elif NRF_POWER_HAS_MAINREGSTATUS
	shell_print("MAINREGSTATUS %s",
		    (nrf_power_mainregstatus_get(NRF_POWER) == NRF_POWER_MAINREGSTATUS_HIGH)
			    ? "high (VDDH)"
			    : "normal (VDD)");
#endif /* NRF_REGULATORS_HAS_MAIN_STATUS */

#if NRF_POWER_HAS_MAINREGSTATUS
	shell_print("MAINREGSTATUS %s",
		    (nrf_power_mainregstatus_get(NRF_POWER) == NRF_POWER_MAINREGSTATUS_HIGH)
			    ? "high (VDDH)"
			    : "normal (VDD)");
#endif /* NRF_POWER_HAS_MAINREGSTATUS */

	return 0;
}

static otError cmd_regulator_mode_ldo(void *ctx, uint8_t argc, char *argv[])
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	printk("Regulator mode request: ldo\r\n");
	return regulator_mode_handle(RADIO_TEST_REGULATOR_MODE_LDO);
}

static otError cmd_regulator_mode_dcdc(void *ctx, uint8_t argc, char *argv[])
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	printk("Regulator mode request: dcdc\r\n");
	return regulator_mode_handle(RADIO_TEST_REGULATOR_MODE_DCDC);
}

static otError cmd_regulator_mode(void *ctx, uint8_t argc, char *argv[])
{
	int rc;
	struct radio_test_cmd cmds[] = {
		{"ldo", "Force LDO regulator mode", cmd_regulator_mode_ldo},
		{"dcdc", "Force DCDC regulator mode", cmd_regulator_mode_dcdc},
	};

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	rc = exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
	if (rc == -ENOTSUP) {
		return OT_ERROR_NOT_IMPLEMENTED;
	}

	if (rc == -EBUSY) {
		return OT_ERROR_BUSY;
	}

	if (rc != 0) {
		shell_print("Invalid regulator mode, use ldo/dcdc");
		return OT_ERROR_INVALID_ARGS;
	}

	return OT_ERROR_NONE;
}
#endif /* CONFIG_MPSL_FEM_HOT_POTATO_REGULATOR_CONTROL_MANUAL */

#if defined(RADIO_TXPOWER_TXPOWER_Pos10dBm)
static otError cmd_pos10dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 10;
	shell_print("TX power: %d", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos10dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos9dBm)
static otError cmd_pos9dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 9;
	shell_print("TX power: %d", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos9dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
static otError cmd_pos8dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 8;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos8dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
static otError cmd_pos7dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 7;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos7dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
static otError cmd_pos6dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 6;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos6dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
static otError cmd_pos5dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 5;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos5dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
static otError cmd_pos4dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 4;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos4dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
static otError cmd_pos3dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 3;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos3dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
static otError cmd_pos2dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 2;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos2dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos1dBm)
static otError cmd_pos1dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 1;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos1dBm) */

static otError cmd_pos0dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = 0;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg1dBm)
static otError cmd_neg1dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -1;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg1dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg2dBm)
static otError cmd_neg2dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -2;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg2dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg3dBm)
static otError cmd_neg3dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -3;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg3dBm) */

static otError cmd_neg4dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -4;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg5dBm)
static otError cmd_neg5dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -5;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg5dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg6dBm)
static otError cmd_neg6dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -6;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg6dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg7dBm)
static otError cmd_neg7dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -7;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg7dBm) */

static otError cmd_neg8dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -8;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg9dBm)
static otError cmd_neg9dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -9;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg9dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg10dBm)
static otError cmd_neg10dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -10;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg10dBm) */

static otError cmd_neg12dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -12;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg14dBm)
static otError cmd_neg14dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -14;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg14dBm) */

static otError cmd_neg16dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -16;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

static otError cmd_neg20dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -20;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg26dBm)
static otError cmd_neg26dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -26;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg26dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg30dBm)
static otError cmd_neg30dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -30;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg30dBm) */

static otError cmd_neg40dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -40;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}

#if defined(RADIO_TXPOWER_TXPOWER_Neg46dBm)
static otError cmd_neg46dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -46;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg46dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Neg70dBm)
static otError cmd_neg70dbm(void *ctx, uint8_t argc, char *argv[])
{
	config.txpower = -70;
	shell_print("TX power : %d dBm", config.txpower);
	return OT_ERROR_NONE;
}
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg70dBm) */

static otError cmd_output_power_set(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
#if defined(RADIO_TXPOWER_TXPOWER_Pos10dBm)
		{"pos10dBm", "TX power: +10 dBm", cmd_pos10dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos10dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos9dBm)
		{"pos9dBm", "TX power: +9 dBm", cmd_pos9dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos9dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
		{"pos8dBm", "TX power: +8 dBm", cmd_pos8dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos8dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
		{"pos7dBm", "TX power: +7 dBm", cmd_pos7dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos7dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
		{"pos6dBm", "TX power: +6 dBm", cmd_pos6dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos6dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
		{"pos5dBm", "TX power: +5 dBm", cmd_pos5dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos5dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
		{"pos4dBm", "TX power: +4 dBm", cmd_pos4dbm},
#endif /* RADIO_TXPOWER_TXPOWER_Pos4dBm */
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
		{"pos3dBm", "TX power: +3 dBm", cmd_pos3dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos3dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
		{"pos2dBm", "TX power: +2 dBm", cmd_pos2dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos2dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos1dBm)
		{"pos1dBm", "TX power: +1 dBm", cmd_pos1dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos1dBm) */
		{"pos0dBm", "TX power: 0 dBm", cmd_pos0dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg1dBm)
		{"neg1dBm", "TX power: -1 dBm", cmd_neg1dbm},
#endif /* RADIO_TXPOWER_TXPOWER_Neg1dBm */
#if defined(RADIO_TXPOWER_TXPOWER_Neg2dBm)
		{"neg2dBm", "TX power: -2 dBm", cmd_neg2dbm},
#endif /* RADIO_TXPOWER_TXPOWER_Neg2dBm */
#if defined(RADIO_TXPOWER_TXPOWER_Neg3dBm)
		{"neg3dBm", "TX power: -3 dBm", cmd_neg3dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg3dBm) */
		{"neg4dBm", "TX power: -4 dBm", cmd_neg4dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg5dBm)
		{"neg5dBm", "TX power: -5 dBm", cmd_neg5dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg5dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg6dBm)
		{"neg6dBm", "TX power: -6 dBm", cmd_neg6dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg6dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg7dBm)
		{"neg7dBm", "TX power: -7 dBm", cmd_neg7dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg7dBm) */
		{"neg8dBm", "TX power: -8 dBm", cmd_neg8dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg9dBm)
		{"neg9dBm", "TX power: -9 dBm", cmd_neg9dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg9dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg10dBm)
		{"neg10dBm", "TX power: -10 dBm", cmd_neg10dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg10dBm) */
		{"neg12dBm", "TX power: -12 dBm", cmd_neg12dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg14dBm)
		{"neg14dBm", "TX power: -14 dBm", cmd_neg14dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg14dBm) */
		{"neg16dBm", "TX power: -16 dBm", cmd_neg16dbm},
		{"neg20dBm", "TX power: -20 dBm", cmd_neg20dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg26dBm)
		{"neg26dBm", "TX power: -26 dBm", cmd_neg26dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg26dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg30dBm)
		{"neg30dBm", "TX power: -30 dBm", cmd_neg30dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg30dBm) */
		{"neg40dBm", "TX power: -40 dBm", cmd_neg40dbm},
#if defined(RADIO_TXPOWER_TXPOWER_Neg46dBm)
		{"neg46dBm", "TX power: -46 dBm", cmd_neg46dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg46dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg70dBm)
		{"neg70dBm", "TX power: -70 dBm", cmd_neg70dbm},
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg70dBm) */
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

static otError cmd_pattern_random(void *ctx, uint8_t argc, char *argv[])
{
	config.tx_pattern = TRANSMIT_PATTERN_RANDOM;
	shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_RANDOM));

	return OT_ERROR_NONE;
}

static otError cmd_pattern_11110000(void *ctx, uint8_t argc, char *argv[])
{
	config.tx_pattern = TRANSMIT_PATTERN_11110000;
	shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_11110000));

	return OT_ERROR_NONE;
}

static otError cmd_pattern_11001100(void *ctx, uint8_t argc, char *argv[])
{
	config.tx_pattern = TRANSMIT_PATTERN_11001100;
	shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_11001100));

	return OT_ERROR_NONE;
}

static otError cmd_transmit_pattern_set(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"pattern_random", "Set transmission pattern to random", cmd_pattern_random},
		{"pattern_11110000", "Set transmission pattern to 11110000", cmd_pattern_11110000},
		{"pattern_11001100", "Set transmission pattern to 10101010", cmd_pattern_11001100},
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}

static otError cmd_print(void *ctx, uint8_t argc, char *argv[])
{
	shell_print("Parameters:");

	switch (config.mode) {
#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	case NRF_RADIO_MODE_NRF_250KBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_250KBIT));
		break;

#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_5)
	case NRF_RADIO_MODE_NRF_4MBIT_H_0_5:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_4MBIT_H_0_5));
		break;
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_5) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit0_25)
	case NRF_RADIO_MODE_NRF_4MBIT_H_0_25:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_4MBIT_H_0_25));
		break;
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit0_25) */

	case NRF_RADIO_MODE_NRF_1MBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_1MBIT));
		break;

	case NRF_RADIO_MODE_NRF_2MBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_NRF_2MBIT));
		break;

	case NRF_RADIO_MODE_BLE_1MBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_1MBIT));
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_2MBIT));
		break;

#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_LR125KBIT));
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_BLE_LR500KBIT));
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		shell_print("Data rate: %s", STRINGIFY(NRF_RADIO_MODE_IEEE802154_250KBIT));
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	default:
		shell_print("Data rate unknown or deprecated: %d\n\r", config.mode);
		break;
	}

	shell_print("TX power : %d dBm", config.txpower);

	switch (config.tx_pattern) {
	case TRANSMIT_PATTERN_RANDOM:
		shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_RANDOM));
		break;

	case TRANSMIT_PATTERN_11110000:
		shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_11110000));
		break;

	case TRANSMIT_PATTERN_11001100:
		shell_print("Transmission pattern: %s", STRINGIFY(TRANSMIT_PATTERN_11001100));
		break;

	default:
		shell_print("Transmission pattern unknown: %d", config.tx_pattern);
		break;
	}

	shell_print("CRC : %s", config.crc_enabled ? "Enabled" : "Disabled");

	shell_print("Start Channel: %u\n"
		    "End Channel: %u\n"
		    "Time on each channel: %u ms\n"
		    "Duty cycle: %u percent\n",
		    (unsigned)config.channel_start, (unsigned)config.channel_end, config.delay_ms,
		    config.duty_cycle);

	return OT_ERROR_NONE;
}

static otError cmd_rx_sweep_start(void *ctx, uint8_t argc, char *argv[])
{
	memset(&test_config, 0, sizeof(test_config));
	test_config.type = RX_SWEEP;
	test_config.mode = config.mode;
	test_config.params.rx_sweep.channel_start = config.channel_start;
	test_config.params.rx_sweep.channel_end = config.channel_end;
	test_config.params.rx_sweep.delay_ms = config.delay_ms;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);

	test_in_progress = true;
	rx_test_in_progress = true;

	shell_print("RX sweep");
	return OT_ERROR_NONE;
}

static otError cmd_tx_sweep_start(void *ctx, uint8_t argc, char *argv[])
{
	memset(&test_config, 0, sizeof(test_config));
	test_config.type = TX_SWEEP;
	test_config.mode = config.mode;
	test_config.params.tx_sweep.channel_start = config.channel_start;
	test_config.params.tx_sweep.channel_end = config.channel_end;
	test_config.params.tx_sweep.delay_ms = config.delay_ms;
	test_config.params.tx_sweep.txpower = config.txpower;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);

	test_in_progress = true;

	shell_print("TX sweep");
	return OT_ERROR_NONE;
}

static otError cmd_rx_start(void *ctx, uint8_t argc, char *argv[])
{
	if (test_in_progress) {
		radio_test_cancel();
		test_in_progress = false;
	}

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	ieee_channel_check(config.channel_start);
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	memset(&test_config, 0, sizeof(test_config));
	test_config.type = RX;
	test_config.mode = config.mode;
	test_config.params.rx.channel = config.channel_start;
	test_config.params.rx.pattern = config.tx_pattern;
	test_config.crc_enabled = config.crc_enabled;

	radio_test_start(&test_config);
	test_in_progress = true;
	rx_test_in_progress = true;

	return OT_ERROR_NONE;
}

static otError cmd_print_payload(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_rx_stats rx_stats;

	memset(&rx_stats, 0, sizeof(rx_stats));

	radio_rx_stats_get(&rx_stats);

	shell_print("Received payload:");
	shell_hexdump(rx_stats.last_packet.buf, rx_stats.last_packet.len);
	shell_print("Number of packets: %d", rx_stats.packet_cnt);

	if (argc == 1) {
		uint32_t num_average_rssi_samples = atoi(argv[0]);
		if (num_average_rssi_samples > MAX_RSSISAMPLE_STORED ||
		    num_average_rssi_samples < 1) {
			shell_print("Invalid parameter: %s. Maximum supported number of RSSI "
				    "samples stored: %d",
				    argv[0], MAX_RSSISAMPLE_STORED);
			return OT_ERROR_INVALID_ARGS;
		} else {
			if (rx_stats.packet_cnt < num_average_rssi_samples) {
				shell_print("Only received %d packets, reporing RSSI for received "
					    "packets only",
					    rx_stats.packet_cnt);
				num_average_rssi_samples = rx_stats.packet_cnt;
			}
			uint8_t averaged_rssi_measurement =
				rssi_sample_averaged_packets_get(num_average_rssi_samples);
			shell_print("Averaged RSSI for last %d packets: -%d",
				    num_average_rssi_samples, averaged_rssi_measurement);
		}
	}

	return OT_ERROR_NONE;
}

#if CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC
static otError cmd_total_output_power_set(void *ctx, uint8_t argc, char *argv[])
{
	int power;

	if (argc != 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	power = atoi(argv[0]);

	if ((power > INT8_MAX) || (power < INT8_MIN)) {
		shell_print("Out of range power value");
		return OT_ERROR_INVALID_ARGS;
	}

	config.txpower = (int8_t)power;

	return OT_ERROR_NONE;
}
#endif /* CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC */

static otError cmd_rssi_get(void *ctx, uint8_t argc, char *argv[])
{
	if (!(test_in_progress && rx_test_in_progress)) {
		shell_print("Invalid state, RSSI only available during RX test");
		return OT_ERROR_INVALID_STATE;
	}

	if (argc > 1) {
		shell_print("Bad parameters count");
		return OT_ERROR_INVALID_ARGS;
	}

	int num_samples = 1;
	if (argc == 1) {
		num_samples = atoi(argv[0]);
		if (num_samples < 1) {
			shell_print("Cannot do negative number of RSSI samples");
			return OT_ERROR_INVALID_ARGS;
		}
	}

	int rssi_sample = 0;
	for (int i = 0; i < num_samples; i++) {
		NRF_RADIO->TASKS_RSSISTART = 1;
		k_usleep(1);
		rssi_sample += NRF_RADIO->RSSISAMPLE;
	}
	rssi_sample /= num_samples;
	shell_print("Results of %d instant RSSI samples averaged: -%d", num_samples, rssi_sample);

	return OT_ERROR_NONE;
}

static otError cmd_temp_get(void *ctx, uint8_t argc, char *argv[])
{
	NRF_TEMP->EVENTS_DATARDY = 0;
	NRF_TEMP->TASKS_START = 1;
	while (!NRF_TEMP->EVENTS_DATARDY)
		;
	NRF_TEMP->EVENTS_DATARDY = 0;

	int32_t temperature_sample = NRF_TEMP->TEMP;
	uint32_t temperature_fraction = (temperature_sample & 0x00000003) * 25;

	shell_print("Die temperature: %d.%d", (temperature_sample >> 2), temperature_fraction);
	return OT_ERROR_NONE;
}

static otError cmd_start(void *ctx, uint8_t argc, char *argv[])
{
	int rc;

	rc = radio_test_init(&test_config);

	if (rc) {
		shell_print("Cannot initialize radio test: %d", rc);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError VendorRadioTest(void *ctx, uint8_t argc, char *argv[])
{
	struct radio_test_cmd cmds[] = {
		{"start_channel",
		 "Start channel for sweep or channel for constant carrier <channel>",
		 cmd_start_channel_set},
		{"end_channel", "End channel for sweep or channel for constant carrier <channel>",
		 cmd_end_channel_set},
		{"time_on_channel", "Time on each channel in ms (1-99) <time>", cmd_time_set},
		{"cancel", "Cancel sweep or carrier", cmd_cancel},
		{"data_rate", "Set data rate", cmd_data_rate_set},
		{"start_tx_carrier", "Start TX carrier", cmd_tx_carrier_start},
		{"start_tx_modulated_carrier",
		 "Start modulated TX carrier [num packets] [tx payload len]",
		 cmd_tx_modulated_carrier_start},
		{"output_power",
		 "Set output power. For FEM with automatic power control, this includes FEM gain",
		 cmd_output_power_set},
#if CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC
		{"total_output_power", "Set output power in dBm, including FEM gain <tx power>",
		 cmd_total_output_power_set},
#endif /* CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC */
		{"transmit_pattern", "Set TX pattern", cmd_transmit_pattern_set},
		{"start_duty_cycle_modulated_tx",
		 "Duty cycle in percent (two decimal digits, 01-90) <duty_cycle>",
		 cmd_duty_cycle_set},
		{"start_interval_modulated_tx",
		 "Interval of packet TX (1 to 10000 ms) <interval> [tx payload len]",
		 cmd_interval_set},
		{"parameters_print", "Print current delay, channel and so on", cmd_print},
		{"start_rx_sweep", "Start RX sweep", cmd_rx_sweep_start},
		{"start_tx_sweep", "Start TX sweep", cmd_tx_sweep_start},
		{"start_rx", "Start RX", cmd_rx_start},
		{"print_rx", "Print RX payload", cmd_print_payload},
		{"crc", "Enable or disable CRC ", cmd_print_crc},
		{"gpio", "Set or clear GPIO output", cmd_print_gpio},
		{"rssi", "Get instant RSSI Sample", cmd_rssi_get},
		{"temp", "Get die temperature", cmd_temp_get},
		{"cpu_activity", "Set CPU activity during radio operations", cmd_cpu_activity_set},
#if defined(TOGGLE_DCDC_HELP)
		{"toggle_dcdc_state", TOGGLE_DCDC_HELP, cmd_toggle_dc},
#endif
#if defined(CONFIG_MPSL_FEM_HOT_POTATO_REGULATOR_CONTROL_MANUAL)
		{"set_regulator_mode", REGULATOR_MODE_HELP, cmd_regulator_mode},
#endif
		{"start", "Start radio test", cmd_start},
	};

	return exec_subcommand(cmds, ARRAY_SIZE(cmds), ctx, argc, argv);
}
