/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_FOTA_
#define SLM_AT_FOTA_

/** @file slm_at_fota.h
 *
 * @brief Vendor-specific AT command for FOTA service.
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

enum fota_stage {
	FOTA_STAGE_INIT,
	FOTA_STAGE_DOWNLOAD,
	FOTA_STAGE_DOWNLOAD_ERASE_PENDING,
	FOTA_STAGE_DOWNLOAD_ERASED,
	FOTA_STAGE_ACTIVATE,
	FOTA_STAGE_COMPLETE
};

enum fota_status {
	FOTA_STATUS_OK,
	FOTA_STATUS_ERROR,
	FOTA_STATUS_CANCELLED
};

extern uint8_t slm_fota_type; /* FOTA image type. */
extern enum fota_stage slm_fota_stage; /* Current stage of FOTA process. */
extern enum fota_status slm_fota_status; /* FOTA process status. */
extern int32_t slm_fota_info; /* FOTA download percentage or failure cause in case of error. */

/**
 * @brief Initialize FOTA AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_fota_init(void);

/**
 * @brief Uninitialize FOTA AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_fota_uninit(void);

/**
 * @brief FOTA post-process after reboot.
 */
void slm_fota_post_process(void);

/**
 * @brief Finishes the full modem firmware update.
 *
 * This is to be called after the application or modem
 * has been rebooted and a full modem firmware update is ongoing.
 */
#if defined(CONFIG_SLM_FULL_FOTA)
void slm_finish_modem_full_dfu(void);
#endif

/** @} */
#endif /* SLM_AT_FOTA_ */
