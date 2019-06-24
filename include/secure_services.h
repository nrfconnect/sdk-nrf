/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Secure Services header.
 */

#ifndef SECURE_SERVICES_H__
#define SECURE_SERVICES_H__

/**
 * @defgroup secure_services Secure Services
 * @{
 * @brief Secure services available to the Non-Secure Firmware.
 *
 * The Secure Services provide access to functionality controlled by the
 * Secure Firmware.
 */

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Request a system reboot from the Secure Firmware.
 *
 * Rebooting is not available from the Non-Secure Firmware.
 */
void spm_request_system_reboot(void);


/** Request a random number from the Secure Firmware.
 *
 * This provides a True Random Number from the on-board random number generator.
 *
 * @note Currently, the RNG hardware is run each time this is called. This
 *       spends significant time and power.
 *
 * @param[out] output  The random number. Must be at least @c len long.
 * @param[in]  len     The length of the output array. Currently, @c len must be
 *                     144.
 * @param[out] olen    The length of the random number provided.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If @c len is invalid. Currently, @c len must be 144.
 */
int spm_request_random_number(u8_t *output, size_t len, size_t *olen);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SECURE_SERVICES_H__ */
