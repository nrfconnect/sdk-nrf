/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * @addtogroup nrf_wifi_fmac_off_raw_tx_api FMAC offloaded raw tx API
 * @{
 *
 * @brief Header containing API declarations for FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_API_H__
#define __FMAC_API_H__

#include <nrf71_wifi_ctrl.h>
#include "common/fmac_api_common.h"
#include "offload_raw_tx/fmac_structs.h"
#include "util.h"


/**
 * @brief Initialize the UMAC IF layer.
 *
 * This function initializes the UMAC IF layer of the RPU WLAN FullMAC driver.
 * It does the following:
 *	- Creates and initializes the context for the UMAC IF layer.
 *	- Initialize the OS abstraction Layer
 *	- Initialize the HAL layer.
 *	- Registers the driver to the underlying Operating System.
 *
 * @return Pointer to the context of the UMAC IF layer.
 */
struct nrf_wifi_fmac_priv *nrf_wifi_off_raw_tx_fmac_init(void);

/**
 * @brief Adds a RPU instance.
 * @param fpriv Pointer to the context of the UMAC IF layer.
 * @param os_dev_ctx Pointer to the OS specific context of the RPU instance.
 *
 * This function adds an RPU instance. This function will return the
 *	    pointer to the context of the RPU instance. This pointer will need to be
 *	    supplied while invoking further device specific APIs,
 *	    for example, nrf_wifi_sys_fmac_scan() etc.
 *
 * @return Pointer to the context of the RPU instance.
 */
struct nrf_wifi_fmac_dev_ctx *nrf_wifi_off_raw_tx_fmac_dev_add(struct nrf_wifi_fmac_priv *fpriv,
							       void *os_dev_ctx);


/**
 * @brief Initialize a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 * @param sleep_type Type of RPU sleep.
 * @param phy_calib PHY calibration flags to be passed to the RPU.
 * @param op_band Operating band of the RPU.
 * @param beamforming Enable/disable Wi-Fi beamforming.
 * @param tx_pwr_ctrl TX power control parameters to be passed to the RPU.
 * @param tx_pwr_ceil_params TX power ceiling parameters to be passed to the RPU.
 * @param board_params Board parameters to be passed to the RPU.
 * @param country_code Country code to be set for regularity domain.
 *
 * This function initializes the firmware of an RPU instance.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On Success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status
nrf_wifi_off_raw_tx_fmac_dev_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
				  int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				  unsigned int phy_calib, unsigned char op_band, bool beamforming,
				  struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl,
				  struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params,
				  struct nrf_wifi_board_params *board_params,
				  unsigned char *country_code, unsigned int *rf_params_addr,
				  unsigned int vtf_buffer_start_address);

/**
 * @brief Configure the offloaded raw TX parameters.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 * @param off_ctrl_params Offloaded raw tx control information.
 * @param off_tx_params Offloaded raw tx parameters.
 *
 * This function configures offloaded raw TX.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On Success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_conf(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_offload_ctrl_params *off_ctrl_params,
	struct nrf_wifi_offload_tx_ctrl *off_tx_params);

/**
 * @brief Start the offloaded raw TX.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance.
 *
 * This function starts offloaded raw TX.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On Success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_start(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Stop the offloaded raw TX.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function stops offloaded raw TX.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On Success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_stop(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Get the RF parameters to be programmed to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param rf_params Pointer to the address where the RF params information needs to be copied.
 *
 * This function is used to fetch RF parameters information from the RPU and
 *	    update the default RF parameter with the OTP values. The updated RF
 *	    parameters are then returned in the \p f_params.
 *
 * @return Command execution status
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_rf_params_get(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_phy_rf_params *rf_params);

/**
 * @brief Issue a request to get stats from the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param op_mode RPU operation mode.
 * @param stats Pointer to memory where the stats are to be copied.
 *
 * This function is used to send a command to
 *	    instruct the firmware to return the current RPU statistics. The RPU will
 *	    send the event with the current statistics.
 *
 * @return Command execution status
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							enum rpu_op_mode op_mode,
							struct rpu_off_raw_tx_op_stats *stats);

/**
 * @}
 */
#endif /* __FMAC_API_H__ */
