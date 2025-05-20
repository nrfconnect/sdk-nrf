/* Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMP_RESET_H_
#define _SMP_RESET_H_

#if defined(CONFIG_NRF_CLOUD_FOTA_SMP) && defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
#define SMP_FOTA_ENABLED 1
#else
#define SMP_FOTA_ENABLED 0
#endif

/**
 * @brief Reset the nRF52840 for SMP FOTA.
 */
int nrf52840_reset_api(void);

#endif /* _SMP_RESET_H_ */
