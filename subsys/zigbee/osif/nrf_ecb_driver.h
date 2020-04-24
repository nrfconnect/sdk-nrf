/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief   ECB Driver ported from nRF5 SDK.
 */

#ifndef NRF_ECB_DRIVER_H__
#define NRF_ECB_DRIVER_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for initializing and powering on the ECB peripheral.
 *
 * This function allocates memory for the ECBDATAPTR.
 *
 * @retval 0  The initialization was successful.
 */
int nrf_ecb_driver_init(void);

/**
 * @brief Function for encrypting 16-byte data using current key.
 *
 * This function avoids unnecessary copying of data if the parameters
 * point to the correct locations in the ECB data structure.
 *
 * @param dst Result of encryption, 16 bytes will be written.
 * @param src Source with 16-byte data to be encrypted.
 *
 * @retval 0  The encryption operation completed.
 */
int nrf_ecb_driver_crypt(u8_t *dst, const u8_t *src);

/**
 * @brief Function for setting the key to be used for encryption.
 *
 * @param key Pointer to the key. 16 bytes will be read.
 */
void nrf_ecb_driver_set_key(const u8_t *key);

#ifdef __cplusplus
}
#endif

#endif  /* NRF_ECB_DRIVER_H__ */
