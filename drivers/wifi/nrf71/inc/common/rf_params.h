/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing RF parameters specific declarations
 */

#ifndef __RF_PARAMS_H__
#define __RF_PARAMS_H__

#include <stdint.h>

#include <common/fw_if/nrf71_wifi_common.h>

#define NUM_RF_PARAM_ADDRS 22

struct rf_hex_param {
	const char *hex_str;
	uint8_t *bytes;
	int bytes_len;
};

/**
 * @brief Configure RF parameters in the RPU.
 *
 * @param dev_ctx Pointer to the FMAC device context.
 * @param rf_params_addr Array of RF parameter buffer addresses (NUM_RF_PARAM_ADDRS entries).
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On success.
 * @retval NRF_WIFI_STATUS_FAIL On failure.
 */
enum nrf_wifi_status nrf_wifi_fmac_config_rf_params(void *dev_ctx,
						    unsigned int *rf_params_addr);

/**
 * @brief Fill the transmit power control and ceiling parameters from devicetree.
 *
 * The values come from the Wi-Fi node in the board devicetree. Antenna gain is
 * additionally applied to the RF parameters by
 * nrf_wifi_fmac_config_rf_params(), which is where the transmit power ceilings
 * and the band edge ceilings are applied as well.
 *
 * @param tx_pwr_ctrl_params Transmit power control parameters to fill.
 * @param tx_pwr_ceil_params Transmit power ceiling parameters to fill.
 */
void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
			       struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params);

#endif /* __RF_PARAMS_H__ */
