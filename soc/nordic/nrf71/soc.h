/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file SoC configuration macros for the Nordic Semiconductor NRF71 family processors.
 */

#ifndef _NORDICSEMI_NRF71_SOC_H_
#define _NORDICSEMI_NRF71_SOC_H_

#include <soc_nrf_common.h>

#define FLASH_PAGE_ERASE_MAX_TIME_US 42000UL
#define FLASH_PAGE_MAX_CNT	     381UL

#ifdef CONFIG_SOC_NRF7120_PREENG
int configure_playout_capture(uint32_t rx_mode, uint32_t tx_mode, uint32_t rx_holdoff_length,
			      uint32_t rx_wrap_length, uint32_t back_to_back_mode);
#endif /* CONFIG_SOC_NRF7120_PREENG */

#endif /* _NORDICSEMI_NRF71_SOC_H_ */
