/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SDFW_PSA_IPC_SERVICE_H__
#define __SDFW_PSA_IPC_SERVICE_H__

/*
 * This header contains symbols that are used by both the SDFW client
 * and the SDFW service.
 */
enum {
	INDEX_HANDLE,
	INDEX_IN_VEC,
	INDEX_IN_LEN,
	INDEX_OUT_VEC,
	INDEX_OUT_LEN,
	INDEX_STATUS_PTR,
	/* Not the name of an index per se, but the number of uint32_t
	 * words in the data sent over ipc_service.
	 */
	SDFW_PSA_IPC_DATA_LEN
};

/* Usually provided by manifest/sid.h */
#define TFM_CRYPTO_SID	   (0x00000080U)
#define TFM_CRYPTO_VERSION (1U)
#define TFM_CRYPTO_HANDLE  (0x40000100U)

#endif /* __SDFW_PSA_IPC_SERVICE_H__ */
