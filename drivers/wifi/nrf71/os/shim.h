/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing OS interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __SHIM_H__
#define __SHIM_H__

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>

/**
 * @brief Get pointers to the driver control and data heaps.
 *
 * @param ctrl If non-NULL, set to the control heap (for small/control allocations).
 * @param data If non-NULL, set to the data heap (for data-path allocations).
 */
void nrf_wifi_shim_get_heaps(struct k_heap **ctrl, struct k_heap **data);

#endif /* __SHIM_H__ */
