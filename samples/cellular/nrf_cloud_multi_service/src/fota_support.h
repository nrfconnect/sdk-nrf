/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FOTA_SUPPORT_H_
#define _FOTA_SUPPORT_H_

#include <zephyr/kernel.h>

/* Time in seconds to wait before rebooting after a FOTA update. */
#define FOTA_REBOOT_DELAY_S	5
#define PENDING_REBOOT_S	10
#define ERROR_REBOOT_S		30
#define FOTA_REBOOT_S		10

/**
 * @brief Notify fota_support that a FOTA download has finished.
 *
 * Besides updating the device shadow (handled in connection.c), this is the only additional
 * code needed to get FOTA working properly, and its sole function is to reboot the microcontroller
 * after FOTA download completes.
 *
 */
void on_fota_downloaded(void);

/**
 * @brief Get the external flash device used for full modem FOTA updates.
 *
 * This function returns NULL if CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE is not
 * enabled.
 *
 */
struct dfu_target_fmfu_fdev * get_full_modem_fota_fdev(void);

#endif /* _FOTA_SUPPORT_H_ */
