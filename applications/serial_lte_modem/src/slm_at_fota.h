/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_FOTA_
#define SLM_AT_FOTA_

/**@file slm_at_fota.h
 *
 * @brief Vendor-specific AT command for FOTA service.
 * @{
 */
enum fota_stages {
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
 *
 */
void slm_fota_post_process(void);
/** @} */

#endif /* SLM_AT_FOTA_ */
