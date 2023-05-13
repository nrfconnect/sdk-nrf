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
 *             bool separate_antennas)
 *
 * @brief      Function used to configure PTA tables of coexistence hardware.
 *
 * @param[in]  enum nrf_wifi_pta_wlan_op_band wlan_band
 * @param[in]  separate_antennas
 *             Indicates whether separate antenans are used or not.
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band, bool separate_antennas);

#if defined(CONFIG_BOARD_NRF7000DK_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7001DK_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
/**
 * @function   nrf_wifi_config_sr_switch(bool separate_antennas)
 *
 * @brief      Function used to configure SR side switch (nRF5340 side switch in nRF7002 DK).
 *
 * @param[in]  separate_antennas
 *               Indicates whether separate antenans are used or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_config_sr_switch(bool separate_antennas);
#endif

/**
 * @function   nrf_wifi_coex_config_non_pta(bool separate_antennas)
 *
 * @brief      Function used to configure non-PTA registers of coexistence hardware.
 *
 * @param[in]  separate_antennas
 *             Indicates whether separate antenans are used or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_non_pta(bool separate_antennas);

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
