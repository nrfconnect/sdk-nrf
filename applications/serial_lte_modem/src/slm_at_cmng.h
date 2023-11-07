/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CMNG_
#define SLM_AT_CMNG_

/**@file slm_at_cmng.h
 *
 * @brief Vendor-specific AT command for CMNG service.
 * @{
 */

/**
 * @brief Initialize CMNG AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_cmng_init(void);

/**
 * @brief Uninitialize CMNG AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_cmng_uninit(void);
/** @} */

#endif /* SLM_AT_CMNG_ */
