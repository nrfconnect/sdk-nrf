/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include "common.h"
#include "nrf_cc3xx_platform_ctr_drbg.h"
#include "nrf_cc3xx_platform_hmac_drbg.h"
#include "psa/crypto.h"
#include "psa/crypto_platform.h"

 psa_status_t mbedtls_psa_external_get_random(
    mbedtls_psa_external_random_context_t *context,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int ret;

#if defined(CONFIG_PSA_WANT_ALG_CTR_DRBG)
    ret = nrf_cc3xx_platform_ctr_drbg_get(
        NULL,
        output,
        output_size,
        output_length);
#elif defined(CONFIG_PSA_WANT_ALG_HMAC_DRBG)
    ret = nrf_cc3xx_platform_hmac_drbg_get(
        NULL,
        output,
        output_size,
        output_length);
#else
    #error "Enable CONFIG_PSA_WANT_ALG_CTR_DRBG or CONFIG_PSA_WANT_ALG_HMAC_DRBG"
#endif

    if (ret != NRF_CC3XX_PLATFORM_SUCCESS)
    {
        return PSA_ERROR_HARDWARE_FAILURE;
    }

    if(output_size != *output_length)
    {
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }

    return PSA_SUCCESS;
}
