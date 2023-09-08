/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CARRIER_
#define SLM_AT_CARRIER_

#include <stdint.h>

/**@file slm_at_carrier.h
 *
 * @brief Vendor-specific AT command for LwM2M Carrier service.
 * @{
 */

/* Whether to auto-connect to the network at startup or after a FOTA update. */
extern int32_t slm_carrier_auto_connect;

/**
 * @brief Initialize Carrier AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_carrier_init(void);

/**
 * @brief Uninitialize Carrier AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_carrier_uninit(void);

/** @} */

#endif /* SLM_AT_CARRIER_ */
