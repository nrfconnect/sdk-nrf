/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_ICMP_
#define SLM_AT_ICMP_

/**@file slm_at_icmp.h
 *
 * @brief Vendor-specific AT command for ICMP service.
 * @{
 */

/**
 * @brief Initialize ICMP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_icmp_init(void);

/**
 * @brief Uninitialize ICMP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_icmp_uninit(void);
/** @} */

#endif /* SLM_AT_ICMP_ */
