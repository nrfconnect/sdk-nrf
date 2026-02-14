/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DESH_AT_CMNG_
#define DESH_AT_CMNG_

/**@file desh_at_cmng.h
 *
 * @brief DESH-specific AT command for credential storing.
 * @{
 */

enum desh_cmng_type {
	DESH_AT_CMNG_TYPE_CA_CERT,
	DESH_AT_CMNG_TYPE_CLIENT_CERT,
	DESH_AT_CMNG_TYPE_CLIENT_KEY,
	DESH_AT_CMNG_TYPE_PSK,
	DESH_AT_CMNG_TYPE_PSK_ID,
	DESH_AT_CMNG_TYPE_COUNT
};

/** @} */

#endif /* DESH_AT_CMNG_ */
