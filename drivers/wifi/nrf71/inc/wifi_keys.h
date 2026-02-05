/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_KEYS_H__
#define WIFI_KEYS_H__

#include <stdint.h>
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PSA key ID range for nRF71 Wi-Fi driver PTK/GTK crypto keys.
 * Must not overlap with ZEPHYR_PSA_WIFI_CREDENTIALS (0x20010000-0x200100ff).
 */
#define NRF71_PSA_WIFI_CRYPTO_KEY_ID_BASE ((psa_key_id_t)0x20020100U)

/** Wi-Fi key role: unicast/broadcast, encryption/MIC */
typedef enum {
	PEER_UCST_ENC,
	PEER_BCST_ENC,
	PEER_UCST_MIC,
	PEER_BCST_MIC,
} wifi_keys_key_type_t;

/**
 * @brief Build PSA key attributes for a Wi-Fi PTK/GTK key.
 *
 * @param type   Key type (unicast/broadcast, enc/MIC).
 * @param db_id  Connection/context identifier.
 * @param key_index Key index (0..WIFI_CRYPTO_MAX_KEYS-1).
 * @return Initialized psa_key_attributes_t with deterministic key_id for
 *         import and later destroy.
 */
static inline psa_key_attributes_t wifi_keys_key_attributes_init(
	wifi_keys_key_type_t type, uint32_t db_id, uint32_t key_index)
{
	psa_key_attributes_t attr = psa_key_attributes_init();
	psa_key_id_t key_id = NRF71_PSA_WIFI_CRYPTO_KEY_ID_BASE
		+ (key_index & 0xfU)
		+ ((uint32_t)(type & 0x3U) << 4U)
		+ ((db_id & 0xffU) << 8U);

	psa_set_key_id(&attr, key_id);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	if (type == PEER_UCST_MIC || type == PEER_BCST_MIC) {
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
		psa_set_key_algorithm(&attr, PSA_ALG_CMAC);
	} else {
		psa_set_key_usage_flags(&attr,
					PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
		psa_set_key_algorithm(&attr, PSA_ALG_CCM);
	}
	return attr;
}

#ifdef __cplusplus
}
#endif

#endif /* WIFI_KEYS_H__ */
