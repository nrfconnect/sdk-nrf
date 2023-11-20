/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CARRIER_CFG_
#define SLM_AT_CARRIER_CFG_

#include <modem/at_cmd_parser.h>

/**@file slm_at_carrier_cfg.h
 *
 * @brief Custom AT command to configure the LwM2M Carrier library.
 * @{
 */
/**
 * @brief AT#XCARRIERCFG command handler.
 */
int handle_at_carrier_cfg(enum at_cmd_type cmd_type);

/** @} */

#endif /* SLM_AT_CARRIER_CFG_ */
