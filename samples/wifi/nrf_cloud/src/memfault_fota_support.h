/* Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMFAULT_FOTA_SUPPORT_H
#define MEMFAULT_FOTA_SUPPORT_H

/**
 * @brief Initialize application FOTA support.
 *
 * Confirms the running image (if not already confirmed) so that MCUboot does not roll back
 * after a successful FOTA update. Must be called once at startup.
 */
void memfault_fota_support_init(void);

#endif /* MEMFAULT_FOTA_SUPPORT_H */
