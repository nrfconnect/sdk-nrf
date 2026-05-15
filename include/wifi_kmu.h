/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_KMU_H__
#define WIFI_KMU_H__

#include <stdint.h>
#include <stddef.h>

/**
 * @file
 * @defgroup WI-FI KMU key loading library
 * @{
 *
 * @brief API for loading WI-FI keys via KMU (Key Management Unit)
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Function to write WI-FI key via KMU
 *
 * This function allows writing keys to an external WI-FI core via KMU
 * for nRF71 series devices. The KMU is the only peripheral that has access
 * to do this operation
 *
 * @param[in]   slot_id         The ID of the slot to write the key to.
 * @param[in]   target_addr     The Address to write the key to in external core.
 * @param[in]   key_buffer      The buffer containing the key.
 * @param[in]   key_size        The size of the key buffer in bytes.
 *
 * @return 0 on success, otherwise a negative error code
 */
int wifi_kmu_write_key(uint32_t slot_id, uint32_t target_addr, const uint8_t *key_buffer,
		       size_t key_size);

/**
 * @brief Function to erase KMU slots with WI-FI keys
 *
 * This function will erase all keys in relevant key slots used for WI-FI key support.
 * This is intended to be run on initialization, to ensure that the KMU slots are
 * revoked (emptied), ready for the next WI-FI key write.
 *
 * @return 0 on success, otherwise a negative error code.
 */
int wifi_kmu_erase_keys(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

 #endif /* WIFI_KMU_H__ */
