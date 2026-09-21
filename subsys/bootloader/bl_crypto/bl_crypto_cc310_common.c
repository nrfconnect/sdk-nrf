/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/storage/flash_map.h>
#include <nrfx.h>
#include <nrf_cc310_bl_init.h>

#ifdef CONFIG_SOC_SERIES_NRF91
#define NRF_CRYPTOCELL NRF_CRYPTOCELL_S
#endif

#if (defined(CONFIG_BL_ROT_VERIFY_EXT_API_ENABLED) || defined(CONFIG_BL_SHA256_EXT_API_ENABLED) \
	|| defined(CONFIG_BL_SECP256R1_EXT_API_ENABLED)) \
	&& DT_NODE_EXISTS(DT_NODELABEL(b0_partition))
#define CHECK_CC310_CALLER
#define B0_END_ADDRESS \
	(PARTITION_ADDRESS(b0_partition) + PARTITION_SIZE(b0_partition))
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
#ifdef CHECK_CC310_CALLER
	uint32_t msp = __get_MSP();

	if (msp < B0_END_ADDRESS) {
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
#ifdef CHECK_CC310_CALLER
	}
#endif
	return 0;
}
