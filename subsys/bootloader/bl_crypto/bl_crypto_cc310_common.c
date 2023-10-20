/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <nrf.h>
#include <nrf_cc310_bl_init.h>
#include <pm_config.h>

#ifdef CONFIG_SOC_SERIES_NRF91X
#define NRF_CRYPTOCELL NRF_CRYPTOCELL_S
#endif

void cc310_bl_backend_enable(void)
{
	/* Enable the cryptocell hardware */
	NRF_CRYPTOCELL->ENABLE = 1;
}

void cc310_bl_backend_disable(void)
{
	/* Disable the cryptocell hardware */
	NRF_CRYPTOCELL->ENABLE = 0;
}

int cc310_bl_init(void)
{
#if (defined(CONFIG_BL_ROT_VERIFY_EXT_API_ENABLED) || defined(CONFIG_BL_SHA256_EXT_API_ENABLED) \
	|| defined(CONFIG_BL_SECP256R1_EXT_API_ENABLED)) && defined(PM_B0_END_ADDRESS)
	uint32_t msp = __get_MSP();

	if (msp < PM_B0_END_ADDRESS) {
#endif
		static bool initialized;

		if (!initialized) {
			cc310_bl_backend_enable();
			if (nrf_cc310_bl_init() != CRYS_OK) {
				return -EFAULT;
			}
			initialized = true;
			cc310_bl_backend_disable();
		}
#if (defined(CONFIG_BL_ROT_VERIFY_EXT_API_ENABLED) || defined(CONFIG_BL_SHA256_EXT_API_ENABLED) \
	|| defined(CONFIG_BL_SECP256R1_EXT_API_ENABLED)) && defined(PM_B0_END_ADDRESS)
	}
#endif
	return 0;
}
