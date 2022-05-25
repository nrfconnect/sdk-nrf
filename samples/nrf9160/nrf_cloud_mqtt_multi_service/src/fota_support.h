/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FOTA_SUPPORT_H_
#define _FOTA_SUPPORT_H_

#include <zephyr/kernel.h>

/**
 * @brief Check whether we are capable of Firmware Over The Air (FOTA) application or modem update.
 *
 * @return bool - Whether we are capable of application or modem FOTA.
 */
static inline bool fota_capable(void)
{
	return IS_ENABLED(CONFIG_NRF_CLOUD_FOTA) && IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT);
}

/**
 * @brief Check whether we are capable of Firmware Over The Air (FOTA) bootloader update.
 *
 * @return bool - Whether we are capable of bootloader FOTA.
 */
static inline bool boot_fota_capable(void)
{
	return IS_ENABLED(CONFIG_BUILD_S1_VARIANT) && IS_ENABLED(CONFIG_SECURE_BOOT);
}

/**
 * @brief Notify fota_support that a FOTA download has finished.
 *
 * Besides updating the device shadow (handled in connection.c), this is the only additional
 * code needed to get FOTA working properly, and its sole function is to reboot the microcontroller
 * after FOTA download completes.
 *
 */
void on_fota_downloaded(void);

#endif /* _FOTA_SUPPORT_H_ */
