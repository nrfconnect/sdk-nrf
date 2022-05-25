/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(TFM_PARTITION_CRYPTO)
#include "common.h"
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>
#endif

#include "tfm_hal_platform.h"
#include "tfm_hal_platform_common.h"
#include "cmsis.h"
#include "uart_stdout.h"
#include "tfm_spm_log.h"
#include "hw_unique_key.h"

#if defined(TFM_PARTITION_CRYPTO)
static enum tfm_hal_status_t crypto_platform_init(void)
{
	int err;

	/* Initialize the nrf_cc3xx runtime */
#if defined(TFM_CRYPTO_RNG_MODULE_DISABLED)
	err = nrf_cc3xx_platform_init_no_rng();
#else
#if defined(PSA_WANT_ALG_CTR_DRBG)
	err = nrf_cc3xx_platform_init();
#elif defined (PSA_WANT_ALG_HMAC_DRBG)
	err = nrf_cc3xx_platform_init_hmac_drbg();
#else
	#error "Please enable either PSA_WANT_ALG_CTR_DRBG or PSA_WANT_ALG_HMAC_DRBG"
#endif
#endif

	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		return TFM_HAL_ERROR_BAD_STATE;
	}

#if !defined(TFM_CRYPTO_KEY_DERIVATION_MODULE_DISABLED) && \
    !defined(PLATFORM_DEFAULT_CRYPTO_KEYS)
	if (!hw_unique_key_are_any_written()) {
		SPMLOG_INFMSG("Writing random Hardware Unique Keys to the KMU.\r\n");
		hw_unique_key_write_random();
		SPMLOG_INFMSG("Success\r\n");
	}
#endif

	return TFM_HAL_SUCCESS;
}
#endif /* defined(TFM_PARTITION_CRYPTO) */


enum tfm_hal_status_t tfm_hal_platform_init(void)
{
	enum tfm_hal_status_t status;

	status = tfm_hal_platform_common_init();
	if (status != TFM_HAL_SUCCESS) {
		return status;
	}

#if defined(TFM_PARTITION_CRYPTO)
	status = crypto_platform_init();
	if (status != TFM_HAL_SUCCESS)
	{
		return status;
	}
#endif /* defined(TFM_PARTITION_CRYPTO) */

	return TFM_HAL_SUCCESS;
}
