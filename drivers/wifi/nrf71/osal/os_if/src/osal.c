/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Implements OSAL APIs to abstract OS primitives.
 */

#include "osal_api.h"
#include "osal_ops.h"

const struct nrf_wifi_osal_ops *os_ops;

void nrf_wifi_osal_init(const struct nrf_wifi_osal_ops *ops)
{
	os_ops = ops;
}

void nrf_wifi_osal_deinit(void)
{
	os_ops = NULL;
}

unsigned char nrf_wifi_osal_rand8_get(void)
{
	return os_ops->rand8_get();
}
