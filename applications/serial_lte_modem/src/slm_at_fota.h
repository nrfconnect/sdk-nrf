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

/** @} */

#endif /* SLM_AT_FOTA_ */
