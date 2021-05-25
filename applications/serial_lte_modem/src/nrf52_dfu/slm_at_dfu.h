/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_DFU_
#define SLM_AT_DFU_

/**@file slm_at_dfu.h
 *
 * @brief Vendor-specific AT command for nRF52 DFU service.
 * @{
 */
/**
 * @brief Initialize DFU AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_dfu_init(void);

/**
 * @brief Uninitialize DFU AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_dfu_uninit(void);

/** @} */

#endif /* SLM_AT_DFU_ */
