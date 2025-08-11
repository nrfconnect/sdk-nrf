/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SDFW_PSA_IPC_SERVICE_H__
#define __SDFW_PSA_IPC_SERVICE_H__

/*
 * This header contains symbols that are used by both the IronSide SE client
 * and the IronSide SE service.
 */
enum {
	IRONSIDE_SE_IPC_INDEX_HANDLE,
	IRONSIDE_SE_IPC_INDEX_IN_VEC,
	IRONSIDE_SE_IPC_INDEX_IN_LEN,
	IRONSIDE_SE_IPC_INDEX_OUT_VEC,
	IRONSIDE_SE_IPC_INDEX_OUT_LEN,
	IRONSIDE_SE_IPC_INDEX_STATUS,
	IRONSIDE_SE_IPC_INDEX_TYPE,
	/* The last enum value is reserved for the size of the IPC buffer */
	IRONSIDE_SE_IPC_DATA_LEN
};

/* IronSide call identifiers with implicit versions */
#define IRONSIDE_CALL_ID_PSA_V1 0

/* We are adding the source files for the TF-M Crypto partition
 * and the TF-M Internal Trusted Storage partition to the build.
 *
 * These partitions will include the file psa_manifest/sid.h and
 * expect the below triplets of symbols to be there.
 *
 * In a TF-M build, the TF-M build system will generate
 * psa_manifest/sid.h based on each partition's manifest.
 *
 * See https://trustedfirmware-m.readthedocs.io/
 * en/latest/integration_guide/services/tfm_secure_partition_addition.html
 *
 * for an example of a partition manifest.
 */
#define TFM_CRYPTO_SID	   (0x00000080U)
#define TFM_CRYPTO_VERSION (1U)
#define TFM_CRYPTO_HANDLE  (0x40000100U)

#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID     (0x00000070U)
#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_VERSION (1U)
#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_HANDLE  (0x40000102U)

#endif /* __SDFW_PSA_IPC_SERVICE_H__ */
