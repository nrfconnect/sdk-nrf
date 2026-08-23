/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_WIFI_PM_H__
#define __ZEPHYR_WIFI_PM_H__

/**
 * @brief Power on the Wi-Fi core and start the LMAC.
 *
 * Enables Wi-Fi resources, clears the LMAC System Off token, and starts the
 * LMAC. The caller must still wait for firmware readiness.
 *
 * @retval 0 on success
 */
int nrf_wifi_power_on(void);

/**
 * @brief Power off the Wi-Fi core (full cold shutdown, no retention).
 *
 * Halts both VPRs, resets the RPU, clears RAM retention, disables Wi-Fi
 * resources, and drops the LRC power request. The next power_on cold-boots
 * the LMAC.
 *
 * @retval 0 on success
 * @retval -EIO if RPU reset did not complete within the expected time
 */
int nrf_wifi_power_off(void);

#endif /* __ZEPHYR_WIFI_PM_H__ */
