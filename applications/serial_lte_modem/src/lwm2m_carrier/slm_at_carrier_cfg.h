/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CARRIER_CFG_
#define SLM_AT_CARRIER_CFG_

/**@file slm_at_carrier_cfg.h
 *
 * @brief Vendor-specific AT command for configuring the LwM2M Carrier service.
 * @{
 */

/**
 * @brief Initialize Carrier Settings AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_carrier_cfg_init(void);

/** @} */

#endif /* SLM_AT_CARRIER_CFG_ */
