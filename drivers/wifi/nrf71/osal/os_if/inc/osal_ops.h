/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing OPs declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_OPS_H__
#define __OSAL_OPS_H__

#include "osal_structs.h"

/**
 * @brief struct nrf_wifi_osal_ops - Ops to be provided by a specific OS implementation.
 */
struct nrf_wifi_osal_ops {
	/**
	 * @brief Get a random 8-bit value.
	 *
	 * @return A random 8-bit value.
	 */
	unsigned char (*rand8_get)(void);
};
#endif /* __OSAL_OPS_H__ */
