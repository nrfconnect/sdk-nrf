/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>
#include <bsd.h>
#include <lte_lc.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include "slm_at_host.h"

LOG_MODULE_REGISTER(app, CONFIG_SLM_LOG_LEVEL);

/* global variable used across different files */
struct at_param_list m_param_list;

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("bsdlib recoverable error: %u", err);
}

void start_execute(void)
{
	int err;

	LOG_INF("Serial LTE Modem");
	err = slm_at_host_init();
	if (err != 0) {
		LOG_ERR("Failed to init at_host: %d", err);
		return;
	}

	/* Initialize AT Parser */
	err = at_params_list_init(&m_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return;
	}
}

void enter_sleep(void)
{
#if defined(CONFIG_SLM_GPIO_WAKEUP)
	/*
	 * Due to errata 4, Always configure PIN_CNF[n].INPUT before
	 *  PIN_CNF[n].SENSE.
	 */
	nrf_gpio_cfg_input(CONFIG_SLM_MODEM_WAKEUP_PIN,
		NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(CONFIG_SLM_MODEM_WAKEUP_PIN,
		NRF_GPIO_PIN_SENSE_LOW);
#endif

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
#endif	/* CONFIG_THIN_GPIO_WAKEUP */
