/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

/* Only PORT 0 can be used. */
BUILD_ASSERT(CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN <
			 ARRAY_SIZE(NRF_P0_S->PIN_CNF));
BUILD_ASSERT(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN <
			 ARRAY_SIZE(NRF_P0_S->PIN_CNF));

static const uint8_t pins_used[] = {CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN,
				    CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN};

/** @brief Allow access to specific GPIOs for the network core. */
static int network_gpio_allow(void)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(pins_used); i++) {
		uint8_t pin = pins_used[i];

		NRF_P0_S->PIN_CNF[pin] =
			(GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos);
	}

	return 0;
}
SYS_INIT(network_gpio_allow, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
