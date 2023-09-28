/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing Coexistence APIs.
 */

#ifndef __ZEPHYR_FMAC_COEX_H__
#define __ZEPHYR_FMAC_COEX_H__

#include <stdbool.h>

/* Indicates WLAN frequency band of operation */
enum nrf_wifi_pta_wlan_op_band {
	NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ = 0,
	NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ,

	NRF_WIFI_PTA_WLAN_OP_BAND_NONE = 0xFF
};

/**
 * @function   nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band,
 *             bool antenna_mode, bool ble_role, bool wlan_role)
 *
 * @brief      Function used to configure PTA tables of coexistence hardware.
 *
 * @param[in]  enum nrf_wifi_pta_wlan_op_band wlan_band
 * @param[in]  antenna_mode
 *             Indicates whether separate antenans are used or not.
 * @param[in]  ble_role
 *             Indicates whether BLE role is central or not.
 @param[in]    wlan_role
 *             Indicates whether WLAN role is server or not.
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band, bool antenna_mode,
		bool ble_role, bool wlan_role);

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
/**
 * @function   nrf_wifi_config_sr_switch(bool antenna_mode)
 *
 * @brief      Function used to configure SR side switch (nRF5340 side switch in nRF7002 DK).
 *
 * @param[in]  antenna_mode
 *               Indicates whether separate antenans are used or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_config_sr_switch(bool antenna_mode);
#endif

/**
 * @function   nrf_wifi_coex_config_non_pta(bool antenna_mode)
 *
 * @brief      Function used to configure non-PTA registers of coexistence hardware.
 *
 * @param[in]  antenna_mode
 *             Indicates whether separate antenans are used or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_non_pta(bool antenna_mode);

/**
 * @function   nrf_wifi_coex_hw_reset(void)
 *
 * @brief      Function used to reset coexistence hardware.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_hw_reset(void);

#endif /* __ZEPHYR_FMAC_COEX_H__ */
