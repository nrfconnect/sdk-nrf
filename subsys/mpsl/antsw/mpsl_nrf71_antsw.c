/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <nrfx.h>
#include <soc_nrf_common.h>
#include <hal/nrf_gpio.h>
#include <mpsl_radio_notification.h>

#define ANTSW_NODE     DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf71_antsw)
#define ANTSW_SEL_PSEL NRF_DT_GPIOS_TO_PSEL(ANTSW_NODE, sel_gpios)

static void m_radio_notification_cb(mpsl_radio_notification_source_t source)
{
	if (source == MPSL_RADIO_NOTIFICATION_SOURCE_ACTIVE) {
		nrf_gpio_pin_control_select(ANTSW_SEL_PSEL,
					    (nrf_gpio_pin_sel_t)GPIO_PIN_CNF_CTRLSEL_ANTSWC);
	} else {
		nrf_gpio_pin_control_select(ANTSW_SEL_PSEL, NRF_GPIO_PIN_SEL_GPIO);
	}
}

static int mpsl_nrf71_antsw_init(void)
{
	nrf_gpio_cfg_output(ANTSW_SEL_PSEL);
	nrf_gpio_pin_clear(ANTSW_SEL_PSEL);

	return mpsl_radio_notification_cfg_set(MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH,
					       MPSL_RADIO_NOTIFICATION_DISTANCE_MIN_US,
					       m_radio_notification_cb);
}

SYS_INIT(mpsl_nrf71_antsw_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
