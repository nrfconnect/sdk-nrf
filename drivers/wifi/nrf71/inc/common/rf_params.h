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

#endif /* __RF_PARAMS_H__ */
