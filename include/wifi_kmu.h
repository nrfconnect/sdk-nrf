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

/* Utility functions to prepare keys before passing to KMU */

/** @brief Value to indicate an invalid key address */
static const uint32_t WIFI_KMU_KEY_ADDR_INVALID = ~0;

/**
 * @brief Different types of keys used by Wi-Fi
 */
typedef enum {
	/** Peer unicast encryption key */
	PEER_UCST_ENC,
	/** Peer unicast message integrity code key */
	PEER_UCST_MIC,
	/** Peer broadcast encryption key */
	PEER_BCST_ENC,
	/** Peer broadcast message integrity code key */
	PEER_BCST_MIC,
	/** Virtual interface encryption key */
	VIF_ENC,
	/** Virtual interface message integrity code key */
	VIF_MIC
} wifi_kmu_key_type_t;

/**
 * @brief Calculate target address of the key
 *
 * This function calculates the target address based on the key type and index.
 * This address can then be passed into wifi_kmu_write_key.
 *
 * @param[in]   type         Type of the key.
 * @param[in]   db_id        Database entry in Wi-Fi secure RAM.
 * @param[in]   key_index    Key index within the database entry.
 *
 * @return A valid address on success, otherwise WIFI_KMU_KEY_ADDR_INVALID.
 */
uint32_t wifi_kmu_get_key_start_addr(wifi_kmu_key_type_t type, uint32_t db_id, uint32_t key_index);

/**
 * @brief Calculate size of the key in bytes.
 *
 * This will be either 16 bytes (128 bits) or 32 bytes (256 bits).
 *
 * @param[in]   type         Type of the key.
 *
 * @return Key size in bytes.
 */
uint32_t wifi_kmu_get_key_size_in_bytes(wifi_kmu_key_type_t type);

/**
 * @brief Calculate size of the key in bits.
 *
 * This will be either 128 bits (16 bytes) or 256 bits (32 bytes).
 *
 * @param[in]   type         Type of the key.
 *
 * @return Key size in bits.
 */
uint32_t wifi_kmu_get_key_size_in_bits(wifi_kmu_key_type_t type);

/**
 * @brief Reverse key byte order before passing into KMU.
 *
 * Keys from wpa_supplicant have a different byte order
 * than expected by nrf71 Wi-Fi hardware.
 * Reverse key byte order from src and copy to dst.
 * dst and src must not overlap.
 *
 * @param[in]   dst               Destination buffer to copy to.
 * @param[in]   src               Source buffer to read from.
 * @param[in]   key_length_bytes  Key length in bytes.
 *
 * @return 0 on success, -1 on failure.
 */
int wifi_kmu_key_reverse_byte_order(void *restrict dst, const void *restrict src,
				    uint32_t key_length_bytes);

/**
 * @brief Reverse key byte order before passing into KMU.
 *
 * Keys from wpa_supplicant have a different byte order
 * than expected by nrf71 Wi-Fi hardware.
 * Reverse key byte order in place in a buffer.
 *
 * @param[in]   buf               Key buffer to modify in place.
 * @param[in]   key_length_bytes  Key length in bytes.
 *
 * @return 0 on success, -1 on failure.
 */
int wifi_kmu_key_reverse_byte_order_in_place(void *buf, uint32_t key_length_bytes);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* WIFI_KMU_H__ */
