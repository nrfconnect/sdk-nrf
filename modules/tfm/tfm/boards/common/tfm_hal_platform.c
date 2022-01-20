/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_hal_platform.h"
#include "tfm_hal_platform_common.h"
#include "cmsis.h"
#include "uart_stdout.h"
#include "tfm_spm_log.h"
#include "hw_unique_key.h"
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>

enum tfm_hal_status_t tfm_hal_platform_init(void)
{
	enum tfm_hal_status_t status;

	status = tfm_hal_platform_common_init();
	if (status != TFM_HAL_SUCCESS) {
		return status;
	}

#if defined(TFM_PARTITION_CRYPTO)
	int err;

	/* Initialize the nrf_cc3xx runtime */
	err = nrf_cc3xx_platform_init();
	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		return TFM_HAL_ERROR_BAD_STATE;
	}

#if !defined(TFM_CRYPTO_RNG_MODULE_DISABLED)
	/* Needed by mbedtls_psa_external_get_random in random_ext.c */
	err = nrf_cc3xx_platform_ctr_drbg_init(NULL, NULL, 0);
	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		return TFM_HAL_ERROR_BAD_STATE;
	}
#endif /* !defined(TFM_CRYPTO_RNG_MODULE_DISABLED) */

#if !defined(TFM_CRYPTO_KEY_DERIVATION_MODULE_DISABLED) && \
    !defined(PLATFORM_DEFAULT_CRYPTO_KEYS)
	if (!hw_unique_key_are_any_written()) {
		SPMLOG_INFMSG("Writing random Hardware Unique Keys to the KMU.\r\n");
		hw_unique_key_write_random();
		SPMLOG_INFMSG("Success\r\n");
	}
#endif
#endif /* defined(TFM_PARTITION_CRYPTO) */

	return TFM_HAL_SUCCESS;
}
