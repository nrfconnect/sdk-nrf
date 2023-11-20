/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CARRIER_
#define SLM_AT_CARRIER_

#include <stdint.h>
#include <modem/at_cmd_parser.h>

/**@file slm_at_carrier.h
 *
 * @brief Vendor-specific AT command for LwM2M Carrier service.
 * @{
 */
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

/**
 * @brief AT#XCARRIER command handler.
 */
int handle_at_carrier(enum at_cmd_type cmd_type);

/** @} */

#endif /* SLM_AT_CARRIER_ */
