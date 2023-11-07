/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_SMS_
#define SLM_AT_SMS_

/**@file slm_at_sms.h
 *
 * @brief Vendor-specific AT command for SMS service.
 * @{
 */

/**
 * @brief Initialize SMS AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_sms_init(void);

/**
 * @brief Uninitialize SMS AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_sms_uninit(void);
/** @} */

#endif /* SLM_AT_SMS_ */
