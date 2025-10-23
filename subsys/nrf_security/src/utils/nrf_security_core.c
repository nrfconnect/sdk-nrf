/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_security_core.h"

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)
#include <zephyr/kernel.h>
#else
#include <soc/nrfx_coredep.h>
#endif

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)
void nrf_security_core_delay_us(uint32_t time_us)
{
	k_usleep(time_us);
}

#else
void nrf_security_core_delay_us(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}
#endif
