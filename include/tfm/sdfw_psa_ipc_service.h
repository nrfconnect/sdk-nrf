/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/*
 * This header contains symbols that are used by both the SDFW client
 * and the SDFW service.
 */

/* Number of uint32_t words in the data sent over ipc_service */
#define SDFW_PSA_IPC_DATA_LEN 6

/* Usually provided by manifest/sid.h */
#define TFM_CRYPTO_SID                                             (0x00000080U)
#define TFM_CRYPTO_VERSION                                         (1U)
#define TFM_CRYPTO_HANDLE                                          (0x40000100U)

