/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 *   This file contains system-agnostic implementation of the nRF 802.15.4 GPIOTE
 *   critical section abstraction.
 */

#include <platform/gpiote/nrf_802154_gpiote.h>

#include <hal/nrf_gpio.h>
#include <nrf_802154_sl_utils.h>
#include <platform/irq/nrf_802154_irq.h>

/* Whether the GPIOTE interrupt was turned on before entering the first critical section. */
static volatile bool gpiote_irq_enabled;
/* Counter of how many times GPIOTE interrupt was disabled while entering critical section. */
static volatile uint32_t gpiote_irq_disabled_cnt;

void nrf_802154_gpiote_critical_section_enter(void)
{
	nrf_802154_sl_mcu_critical_state_t mcu_cs;
	uint32_t cnt;

	nrf_802154_sl_mcu_critical_enter(mcu_cs);
	cnt = gpiote_irq_disabled_cnt;

	if (cnt == 0U) {
		gpiote_irq_enabled = nrf_802154_irq_is_enabled(GPIOTE_IRQn);
		nrf_802154_irq_disable(GPIOTE_IRQn);
	}

	cnt++;
	gpiote_irq_disabled_cnt = cnt;

	nrf_802154_sl_mcu_critical_exit(mcu_cs);
}

void nrf_802154_gpiote_critical_section_exit(void)
{
	nrf_802154_sl_mcu_critical_state_t mcu_cs;
	uint32_t cnt;

	nrf_802154_sl_mcu_critical_enter(mcu_cs);

	cnt = gpiote_irq_disabled_cnt;
	cnt--;

	if (cnt == 0U) {
		if (gpiote_irq_enabled) {
			nrf_802154_irq_enable(GPIOTE_IRQn);
		}
	}

	gpiote_irq_disabled_cnt = cnt;

	nrf_802154_sl_mcu_critical_exit(mcu_cs);
}
