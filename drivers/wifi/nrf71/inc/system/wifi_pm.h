/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_WIFI_PM_H__
#define __ZEPHYR_WIFI_PM_H__

/**
 * @brief Power on the Wi-Fi subsystem and start the LMAC.
 *
 * Enables the WIFICORE resources, clears the Wi-Fi System Off token, and starts
 * the LMAC. Firmware readiness is reported asynchronously.
 *
 * @retval 0 on success.
 */
int nrf_wifi_power_on(void);

/**
 * @brief Power off the Wi-Fi subsystem for a cold restart.
 *
 * Halts the LMAC and UMAC VPRs, resets the RPU, disables Wi-Fi RAM and active
 * power-domain retention, gates the Wi-Fi clocks and RAMs, and removes the LRC
 * power requests. A subsequent nrf_wifi_power_on() call performs a cold LMAC
 * boot. Power isolation is completed even if the RPU reset status times out.
 *
 * @retval 0 on success.
 * @retval -EIO if the RPU reset status is not observed before the timeout.
 */
int nrf_wifi_power_off(void);

#endif /* __ZEPHYR_WIFI_PM_H__ */
