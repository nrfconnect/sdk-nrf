/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/modem_battery.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#define WORK_Q_STACK_SIZE 1024
#define WORK_Q_PRIORITY 5

K_THREAD_STACK_DEFINE(low_level_work_q_stack_area, WORK_Q_STACK_SIZE);
K_THREAD_STACK_DEFINE(pofwarn_work_q_stack_area, WORK_Q_STACK_SIZE);

struct k_work_q low_level_work_q;
struct k_work_q pofwarn_work_q;
struct k_work low_level_work;
struct k_work pofwarn_work;

/* Protect `state` with mutually exclusive access. */
K_SEM_DEFINE(state_sem, 1, 1);
static enum state_type {
	INIT,
	POFWARN,
	RX_ONLY_LOW_BAT,
	RX_ONLY_HIGH_BAT,
	NORMAL_LOW_BAT,
	NORMAL_HIGH_BAT,
} state = INIT;

static int refresh_interval = CONFIG_HIGH_BAT_REFRESH_INTERVAL_SEC;

static void state_update(enum state_type new_state)
{
	switch (new_state) {
		/* State after receiving modem power off warning. */
		case POFWARN: {
			state = POFWARN;
			refresh_interval = CONFIG_POFWARN_REFRESH_INTERVAL_SEC;
			break;
		}
		/* State of low battery voltage and receive only connectivity
		 * for connection evaluation.
		 */
		case RX_ONLY_LOW_BAT: {
			state = RX_ONLY_LOW_BAT;
			refresh_interval = CONFIG_LOW_BAT_REFRESH_INTERVAL_SEC;
			break;
		}
		/* State of high battery voltage and receive only connectivity
		 * for connection evaluation.
		 * If evaluation is positive, switch to normal mode.
		 */
		case RX_ONLY_HIGH_BAT: {
			state = RX_ONLY_HIGH_BAT;
			refresh_interval = CONFIG_HIGH_BAT_REFRESH_INTERVAL_SEC;
			break;
		}
		/* State of low battery voltage and normal connectivity. */
		case NORMAL_LOW_BAT: {
			state = NORMAL_LOW_BAT;
			break;
		}
		/* State of high battery voltage and normal connectivity. */
		case NORMAL_HIGH_BAT: {
			state = NORMAL_HIGH_BAT;
			break;
		}
		default: {
			break;
		}
	}
}

static void low_level_state_update(struct k_work *item)
{
	int err;

	k_sem_take(&state_sem, K_FOREVER);

	if (state == NORMAL_HIGH_BAT) {
		err = lte_lc_deinit();
		if (err) {
			printk("Modem deinitialization failed, err: %d\n", err);
			return;
		}

		state_update(NORMAL_LOW_BAT);
	}

	k_sem_give(&state_sem);
}

static void pofwarn_state_update(struct k_work *item)
{
	k_sem_take(&state_sem, K_FOREVER);
	state_update(POFWARN);
	k_sem_give(&state_sem);
}

static void low_level_handler(int battery_voltage)
{
	printk("Battery low level: %d\n", battery_voltage);

	/* Offload to workqueue to not block the AT monitor. */
	k_work_submit_to_queue(&low_level_work_q, &low_level_work);
}

static void print_battery_low_warning(void)
{
	printk("Modem Event Battery LOW:\n"
	       "******************************************************************\n"
	       "* Attention! Do not attempt to write to NVM while in this state. *\n"
	       "* The NVM operation will sometimes appear to finish successfully *\n"
	       "* without actually being executed at all.                        *\n"
	       "* The modem has been set to Offline.                             *\n"
	       "******************************************************************\n");
}

static void pofwarn_handler(void)
{
	print_battery_low_warning();

	/* Offload to workqueue to not block the AT monitor. */
	k_work_submit_to_queue(&pofwarn_work_q, &pofwarn_work);
}

static void ip_traffic(void)
{
	int err;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	/* Rotation of hosts to bypass DNS caching. */
	static const char * const hosts[] = {
		"example.com", "google.com", "apple.com", "amazon.com", "microsoft.com"
	};
	static int index;

	printk("Executing DNS lookup for '%s'...\n", hosts[index % 5]);
	err = getaddrinfo(hosts[index % 5], NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d\n", errno);
		return;
	}

	freeaddrinfo(res);

	index++;
}

static int set_rx_only_mode(void)
{
	int err;

	printk("Setting modem to RX only mode...\n");

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_RX_ONLY);
	if (err) {
		return err;
	}

	printk("RX only mode set.\n");

	return 0;
}

static int set_normal_mode(void)
{
	int err;

	printk("Setting modem to normal mode...\n");

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	if (err) {
		return err;
	}

	printk("Normal mode set.\n");

	return 0;
}

static int modem_init_and_connect(void)
{
	int err;

	printk("Initializing modem and connecting...\n");

	err = lte_lc_init_and_connect();
	if (err) {
		printk("Modem initialization and connect failed, err: %d\n", err);
		return err;
	}

	printk("Connected.\n");

	return 0;
}

static void setup_workqueues(void)
{
	k_work_queue_init(&low_level_work_q);
	k_work_queue_init(&pofwarn_work_q);

	k_work_queue_start(&low_level_work_q, low_level_work_q_stack_area,
			   K_THREAD_STACK_SIZEOF(low_level_work_q_stack_area), WORK_Q_PRIORITY,
			   NULL);
	k_work_queue_start(&pofwarn_work_q, pofwarn_work_q_stack_area,
			   K_THREAD_STACK_SIZEOF(pofwarn_work_q_stack_area), WORK_Q_PRIORITY,
			   NULL);

	k_work_init(&low_level_work, low_level_state_update);
	k_work_init(&pofwarn_work, pofwarn_state_update);
}

static int init_activity(int battery_voltage)
{
	int err;

	/* At initialization, set RX only mode if battery voltage is low, normal mode otherwise. */
	if (battery_voltage < CONFIG_MODEM_BATTERY_LOW_LEVEL) {
		err = set_rx_only_mode();
		if (err) {
			printk("Modem RX only mode failed, err: %d\n", err);
			return err;
		}

		state_update(RX_ONLY_LOW_BAT);
	} else {
		err = set_normal_mode();
		if (err) {
			printk("Modem normal mode failed, err: %d\n", err);
			return err;
		}

		state_update(NORMAL_HIGH_BAT);
	}

	err = modem_init_and_connect();
	if (err) {
		return err;
	}

	return 0;
}

static int pofwarn_activity(int battery_voltage)
{
	int err;

	/* MODEM_BATTERY_POFWARN_VOLTAGE `3X` to corresponding voltage value `3X00`. */
	if (battery_voltage >= CONFIG_MODEM_BATTERY_POFWARN_VOLTAGE * 100) {
		err = set_rx_only_mode();
		if (err) {
			printk("Modem RX only mode failed, err: %d\n", err);
			return err;
		}

		err = modem_init_and_connect();
		if (err) {
			return err;
		}

		state_update(RX_ONLY_LOW_BAT);
	}

	return 0;
}

static int rx_only_high_bat_activity(void)
{
	int err;
	struct lte_lc_conn_eval_params params = { 0 };

	err = lte_lc_conn_eval_params_get(&params);
	if (err) {
		printk("Energy estimate failed, err: %d\n", err);
		return err;
	}

	printk("Energy estimate: %d\n", params.energy_estimate);

	/* Set normal mode when conditions are either normal (7), good (8) or excellent (9).
	 * In these conditions the number of repetitions are only a few or none at all
	 * which contributes to more efficient battery consumption.
	 * If conditions are not sufficient, the Modem will stay in the low power RX only mode.
	 */
	if (params.energy_estimate >= 7) {
		err = set_normal_mode();
		if (err) {
			printk("Modem normal mode failed, err: %d\n", err);
			return err;
		}

		state_update(NORMAL_HIGH_BAT);
	}

	return 0;
}

static int normal_low_bat_activity(void)
{
	int err;

	/* Set RX only mode to decrease the power consumption of the
	 * Modem and to allow the application to periodically evaluate
	 * the connection's energy estimate.
	 */
	err = set_rx_only_mode();
	if (err) {
		printk("Modem RX only mode failed, err: %d\n", err);
		return err;
	}

	err = modem_init_and_connect();
	if (err) {
		return err;
	}

	state_update(RX_ONLY_LOW_BAT);

	return 0;
}

int main(void)
{
	int err;
	int battery_voltage;

	printk("Battery sample started\n");

	/* Setup handlers before initializing modem library. */
	modem_battery_low_level_handler_set(low_level_handler);
	modem_battery_pofwarn_handler_set(pofwarn_handler);

	setup_workqueues();

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return err;
	}

	while (true) {
		/* At every iteration, get battery voltage and decide what to do based on that. */
		err = modem_battery_voltage_get(&battery_voltage);
		if (err) {
			printk("Battery voltage read failed, err: %d\n", err);
			return err;
		}

		printk("Battery voltage: %d\n", battery_voltage);

		k_sem_take(&state_sem, K_FOREVER);

		switch (state) {
			case INIT: {
				err = init_activity(battery_voltage);
				if (err) {
					return err;
				}

				break;
			}
			case POFWARN: {
				err = pofwarn_activity(battery_voltage);
				if (err) {
					return err;
				}

				break;
			}
			case RX_ONLY_LOW_BAT: {
				if (battery_voltage >= CONFIG_MODEM_BATTERY_LOW_LEVEL) {
					state_update(RX_ONLY_HIGH_BAT);
				}

				break;
			}
			case RX_ONLY_HIGH_BAT: {
				err = rx_only_high_bat_activity();
				if (err) {
					return err;
				}

				break;
			}
			case NORMAL_LOW_BAT: {
				err = normal_low_bat_activity();
				if (err) {
					return err;
				}

				break;
			}
			case NORMAL_HIGH_BAT: {
				/* Periodically execute some IP traffic to maintain LTE connection
				 * and trigger modem polling of battery voltage.
				 */
				ip_traffic();
				break;
			}
			default: {
				break;
			}
		}

		k_sem_give(&state_sem);

		k_sleep(K_SECONDS(refresh_interval));
	}

	return 0;
}
