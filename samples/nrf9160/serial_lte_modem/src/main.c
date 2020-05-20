/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <string.h>
#include <bsd.h>
#include <modem/lte_lc.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <modem/modem_info.h>
#include "slm_at_host.h"

LOG_MODULE_REGISTER(app, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(2)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

static struct device *gpio_dev;
static struct gpio_callback gpio_cb;
static struct k_work exit_idle_work;

/* global variable used across different files */
struct at_param_list at_param_list;
struct modem_param_info modem_param;
char rsp_buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];
struct k_work_q slm_work_q;

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("bsdlib recoverable error: %u", err);
}

static void exit_idle(struct k_work *work)
{
	int err;

	LOG_INF("Exit Idle");
	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN,
				     GPIO_INT_DISABLE);
	gpio_remove_callback(gpio_dev, &gpio_cb);
	/* Do the same as nrf_gpio_cfg_default() */
	gpio_pin_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN, GPIO_INPUT);
	/* Restart SLM services */
	err = slm_at_host_init();
	if (err) {
		LOG_ERR("Failed to init at_host: %d", err);
	}
}

static void gpio_callback(struct device *dev,
		     struct gpio_callback *gpio_cb, u32_t pins)
{
	k_work_submit_to_queue(&slm_work_q, &exit_idle_work);
}

void enter_idle(void)
{
	int err;

	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (gpio_dev == NULL) {
		LOG_ERR("GPIO_0 bind error");
		return;
	}
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN,
				GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return;
	}
	gpio_init_callback(&gpio_cb, gpio_callback,
			BIT(CONFIG_SLM_INTERFACE_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("GPIO_0 add callback error: %d", err);
		return;
	}
	err = gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN,
					   GPIO_INT_LEVEL_LOW);
	if (err) {
		LOG_ERR("GPIO_0 enable callback error: %d", err);
	}
}

void enter_sleep(void)
{
#if defined(CONFIG_SLM_GPIO_WAKEUP)
	/*
	 * Due to errata 4, Always configure PIN_CNF[n].INPUT before
	 *  PIN_CNF[n].SENSE.
	 */
	nrf_gpio_cfg_input(CONFIG_SLM_INTERFACE_PIN,
		NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(CONFIG_SLM_INTERFACE_PIN,
		NRF_GPIO_PIN_SENSE_LOW);
#endif	/* CONFIG_SLM_GPIO_WAKEUP */

	/*
	 * The LTE modem also needs to be stopped by issuing a command
	 * through the modem API, before entering System OFF mode.
	 * Once the command is issued, one should wait for the modem
	 * to respond that it actually has stopped as there may be a
	 * delay until modem is disconnected from the network.
	 * Refer to https://infocenter.nordicsemi.com/topic/ps_nrf9160/
	 * pmu.html?cp=2_0_0_4_0_0_1#system_off_mode
	 */
	lte_lc_power_off();
	bsd_shutdown();
	nrf_regulators_system_off(NRF_REGULATORS_NS);
}

void start_execute(void)
{
	int err;

	LOG_INF("Serial LTE Modem");

	err = modem_info_init();
	if (err) {
		LOG_ERR("Modem info could not be established: %d", err);
		return;
	}

	modem_info_params_init(&modem_param);

	/* Initialize AT Parser */
	err = at_params_list_init(&at_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return;
	}

	err = slm_at_host_init();
	if (err) {
		LOG_ERR("Failed to init at_host: %d", err);
		return;
	}

	k_work_q_start(&slm_work_q, slm_wq_stack_area,
		K_THREAD_STACK_SIZEOF(slm_wq_stack_area), SLM_WQ_PRIORITY);
	k_work_init(&exit_idle_work, exit_idle);
}

#if defined(CONFIG_SLM_GPIO_WAKEUP)
void main(void)
{
	u32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);

	LOG_DBG("RR: 0x%08x", rr);
	if (rr & NRF_POWER_RESETREAS_OFF_MASK) {
		nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
		start_execute();
	} else {
		LOG_INF("Sleep");
		enter_sleep();
	}
}
#else
void main(void)
{
	start_execute();
}
#endif	/* CONFIG_SLM_GPIO_WAKEUP */
