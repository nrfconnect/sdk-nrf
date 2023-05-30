/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @addtogroup nrf700x_fmac_api_radio_test FMAC radio test API
 * @{
 *
 * @brief Header containing API declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_API_H__
#define __FMAC_API_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"
#include "host_rpu_data_if.h"
#include "host_rpu_sys_if.h"

#include "fmac_structs.h"
#include "fmac_cmd.h"
#include "fmac_event.h"
#include "fmac_vif.h"
#include "fmac_bb.h"
#include "fmac_api_common.h"


/**
 * @brief Initializes the UMAC IF layer.
 *
 * This function initializes the UMAC IF layer of the RPU WLAN FullMAC driver.
 *     It does the following:
 *
 *		- Creates and initializes the context for the UMAC IF layer.
 *		- Initializes the HAL layer.
 *		- Registers the driver to the underlying OS.
 *
 * @return Pointer to the context of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv *wifi_nrf_fmac_init_rt(void);


/**
 * @brief Initialize the RPU for radio tests.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param params Parameters necessary for the initialization.
 *
 * This function is used to send a command to RPU to initialize it
 *	    for the radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						   struct rpu_conf_params *params);

/**
 * @brief Start TX tests in radio test mode.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param params Parameters necessary for the TX tests.
 *
 * This function is used to send a command to RPU to start
 *	    the TX tests in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_prog_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						      struct rpu_conf_params *params);

/**
 * @brief Start RX tests in radio test mode.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param params Parameters necessary for the RX tests.
 *
 * This function is used to send a command to RPU to start
 *	    the RX tests in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_prog_rx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						      struct rpu_conf_params *params);


/**
 * @brief Start RF test capture in radio test mode.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param rf_test_type Type of RF test to be performed.
 * @param cap_data Pointer to the memory where the RF test capture is to be stored.
 * @param num_samples Number of RF test samples to capture.
 * @param lna_gain LNA gain value.
 * @param bb_gain Baseband gain value.
 *
 * This function is used to send a command to RPU to start
 *	    the RF test capture in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_rx_cap(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  enum nrf_wifi_rf_test rf_test_type,
						  void *cap_data,
						  unsigned short int num_samples,
						  unsigned char lna_gain,
						  unsigned char bb_gain);


/**
 * @brief Start/Stop RF TX tone test in radio test mode.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param enable Enable/Disable TX tone test.
 * @param tone_freq Desired tone frequency in MHz in steps of 1 MHz from -10 MHz to +10 MHz.
 * @param tx_power Desired TX power in the range -16dBm to +24dBm.
 *
 * This function is used to send a command to RPU to start the RF TX tone test
 *     in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_tx_tone(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  unsigned char enable,
						  signed char tone_freq,
						  signed char tx_power);



/**
 * @brief Start/Stop RF DPD test in radio test mode.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param enable Enable/Disable DPD test.
 *
 * This function is used to send a command to RPU to start
 *	    the RF DPD test in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_dpd(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       unsigned char enable);



/**
 * @brief Get temperature in Fahrenheit using temperature sensor.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU to get the current temperature.
 *	    using the radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_get_temp(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);



/**
 * @brief Get RF RSSI status.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU to get
 *	    RF RSSI status in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_get_rf_rssi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * @brief Set XO adjustment value.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param value XO adjustment value.
 *
 * This function is used to send a command to RPU to set XO adjustment
 *	    value in radio test mode.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_set_xo_val(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					      unsigned char value);

/**
 * @brief Get XO calibrated value.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU. RPU estimates and
 *	    returns optimal XO value.
 *
 * @return Command execution status
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_compute_xo(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * @brief De-initializes the UMAC IF layer.
 * @param fpriv Pointer to the context of the UMAC IF layer.
 *
 * This function de-initializes the UMAC IF layer of the RPU WLAN FullMAC
 *	    driver. It does the following:
 *
 *		- De-initializes the HAL layer.
 *		- Frees the context for the UMAC IF layer.
 *
 * @return None
 */
void wifi_nrf_fmac_deinit_rt(struct wifi_nrf_fmac_priv *fpriv);


/**
 * @brief Removes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function handles the removal of an RPU instance at the UMAC IF layer.
 *
 * @return None.
 */
void wifi_nrf_fmac_dev_rem_rt(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * @brief Initializes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 * @param sleep_type Type of RPU sleep.
 * @param phy_calib PHY calibration flags to be passed to the RPU.
 * @param op_band Operating band of the RPU.
 * @param tx_pwr_ctrl_params TX power control parameters to be passed to the RPU.
 *
 * This function initializes the firmware of an RPU instance.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_dev_init_rt(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
#if defined(CONFIG_NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
			int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
			unsigned int phy_calib,
			enum op_band op_band,
			struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params);


/**
 * @brief De-initializes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function de-initializes the firmware of an RPU instance.
 *
 * @return None.
 */
void wifi_nrf_fmac_dev_deinit_rt(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @}
 */
#endif /* __FMAC_API_H__ */
