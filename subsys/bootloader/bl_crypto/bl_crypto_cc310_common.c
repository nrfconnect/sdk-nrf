/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <errno.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include <toolchain.h>
#include <nrf.h>
#include <nrf_cc310_bl_init.h>

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
	static bool initialized;

	if (!initialized) {
		cc310_bl_backend_enable();
		if (nrf_cc310_bl_init() != CRYS_OK) {
			return -EFAULT;
		}
		initialized = true;
		cc310_bl_backend_disable();
	}

	return 0;
}
