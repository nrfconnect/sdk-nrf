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
	/* The last enum value is reserved for the size of the IPC buffer */
	IRONSIDE_SE_IPC_DATA_LEN
};

/* IronSide call identifiers with implicit versions.
 *
 * With the initial "version 0", the service ABI is allowed to break until the
 * first public release of IronSide SE.
 */
#define IRONSIDE_CALL_ID_PSA_CRYPTO_V0 0

/* We are adding the source files for the TF-M crypto partition to the build.
 *
 * The crypto partition will include the file psa_manifest/sid.h and
 * expect the below three symbols to be there.
 *
 * In a TF-M build, the TF-M build system will generate
 * psa_manifest/sid.h based on each partitions manifest.
 *
 * See https://trustedfirmware-m.readthedocs.io/
 * en/latest/integration_guide/services/tfm_secure_partition_addition.html
 *
 * for an example of a partition manifest.
 */
#define TFM_CRYPTO_SID	   (0x00000080U)
#define TFM_CRYPTO_VERSION (1U)
#define TFM_CRYPTO_HANDLE  (0x40000100U)

#endif /* __SDFW_PSA_IPC_SERVICE_H__ */
