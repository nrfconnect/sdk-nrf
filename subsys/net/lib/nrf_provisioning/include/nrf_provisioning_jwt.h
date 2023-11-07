/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_provisioning_jwt.h
 *
 * @brief nRF Provisioning JWT API.
 */
#ifndef NRF_PROVISIONING_JWT_H__
#define NRF_PROVISIONING_JWT_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup nrf_provisioning_jwt nRF Device Provisioning JWT API
 *  @{
 */

/**
 * @brief Function to generate a JWT to be used with nRF Device provisioning service's REST API.
 *        This library's configured values for client id and sec tag (NRF_PROVISIONING_SEC_TAG)
 *        will be used for generating the JWT.
 *
 * @param[in] time_valid_s How long (seconds) the JWT will be valid.
 *                         If zero, @ref NRF_PROVISIONING_JWT_MAX_VALID_TIME_S will be used.
 * @param[in,out] jwt_buf Buffer to hold the JWT.
 * @param[in] jwt_buf_sz  Size of the buffer (recommended size >= 600 bytes).
 *
 * @retval 0      JWT generated successfully.
 * @retval -ETIME Modem does not have valid date/time, JWT not generated.
 * @return A negative value indicates an error.
 */
int nrf_provisioning_jwt_generate(uint32_t time_valid_s, char *const jwt_buf, size_t jwt_buf_sz);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_JWT_H__ */
