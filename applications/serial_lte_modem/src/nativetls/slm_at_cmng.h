/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CMNG_
#define SLM_AT_CMNG_

/**@file slm_at_cmng.h
 *
 * @brief SLM-specific AT command for credential storing.
 * @{
 */

enum slm_cmng_type {
	SLM_AT_CMNG_TYPE_CA_CERT,
	SLM_AT_CMNG_TYPE_CLIENT_CERT,
	SLM_AT_CMNG_TYPE_CLIENT_KEY,
	SLM_AT_CMNG_TYPE_PSK,
	SLM_AT_CMNG_TYPE_PSK_ID,
	SLM_AT_CMNG_TYPE_COUNT
};

/** @} */

#endif /* SLM_AT_CMNG_ */
