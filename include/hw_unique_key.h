/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HW_UNIQUE_KEY_H_
#define HW_UNIQUE_KEY_H_

/**
 * @file
 * @defgroup hw_unique_key Hardware Unique Key (HUK) loading
 * @{
 *
 * @brief API for loading the Hardware Unique Key (HUK) in the CryptoCell
 *        KDR registers.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function loads the Hardware Unique Key (HUK) into the KDR
 *        registers of the Cryptocell. It also locks the flash page
 *        which contains the key material from reading and writing
 *        until the next reboot.
 *
 * @return non-negative on success, negative errno code on fail
 */
int hw_unique_key_load(const struct device *unused);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HW_UNIQUE_KEY_H_ */
