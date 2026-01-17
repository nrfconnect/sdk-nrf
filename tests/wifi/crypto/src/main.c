/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <psa/crypto.h>
#include "wifi_keys.h"

LOG_MODULE_REGISTER(wifi_crypto_test, LOG_LEVEL_DBG);

ZTEST(wifi_crypto, test_main)
{
	uint32_t ccmp256_key[8] = {0x0C0D0E0F, 0x08090A0B, 0x04050607, 0x00010203,
				   0xF2BDD52F, 0x514A8A19, 0xCE371185, 0xC97C1F67};

	wifi_keys_key_type_t type = PEER_UCST_ENC;
	psa_key_attributes_t attr = wifi_keys_key_attributes_init(type, 0, 0);
	uint32_t key_length = wifi_keys_get_key_size_in_bytes(type);
	psa_key_id_t key_id;

	psa_status_t status;

	status = psa_crypto_init();
	zassert_equal(status, PSA_SUCCESS);

	status = psa_import_key(&attr, (const uint8_t *)ccmp256_key, key_length, &key_id);
	zassert_equal(status, PSA_SUCCESS);

	*(volatile uint32_t *)0x48086C04 = 1; /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO1OUT */
	while (*(volatile uint32_t *)0x48086C00 !=
	       2) { /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO0OUT */
	}

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST(wifi_crypto, test_psa_import_key_failure_exact_params)
{
	/* Matching the trace with these params:
	 * alg 2
	 * key_idx 1
	 * set_tx 1
	 * key_len 32
	 * suite: 0x000fac02
	 * type: 32770
	 * Importing key to PSA, location: 8408663, persistence: 1, lifetime: -2142349567 key:
	 * 0x3F414201 Expect failure: wifi_install_key_to_crypto: Failed to import key to PSA: -135
	 */

	uint32_t ccmp256_key[8] = {0x0C0D0E0F, 0x08090A0B, 0x04050607, 0x00010203,
				   0xF2BDD52F, 0x514A8A19, 0xCE371185, 0xC97C1F67};

	const wifi_keys_key_type_t type = PEER_UCST_ENC;
	const uint32_t db_id = 1;
	const uint32_t key_idx = 1;

	psa_key_attributes_t attr = wifi_keys_key_attributes_init(type, db_id, key_idx);
	uint32_t key_length = wifi_keys_get_key_size_in_bytes(type);
	psa_key_id_t key_id;
	psa_status_t status;

	/* The attr struct does not have a .core member (per build error/log), so set the attributes
	 * using PSA setter APIs
	 */
	psa_set_key_type(&attr, 32770); /* 0x8002, PSA_KEY_TYPE_AES */
	psa_set_key_bits(&attr, 256);
	psa_set_key_algorithm(&attr, 2); /* alg 2 (normally PSA_ALG_CCM */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(
		&attr, (psa_key_lifetime_t)(0x80000001)); /* persistence: 1, location: 0x80000000 */

	/* PSA v1.0 standard: id only set via special setter if supported */
#if defined(PSA_KEY_ID_USER_MIN)
	psa_set_key_id(&attr, 0x3F414201);
#endif

	LOG_INF("Testing import failure with key params matching WPA supplicant/if trace");
	LOG_INF("alg: %d type: %u len: %u", psa_get_key_algorithm(&attr), psa_get_key_type(&attr),
		key_length);

	psa_key_lifetime_t lifetime = psa_get_key_lifetime(&attr);

	LOG_INF("lifetime: %u", (unsigned int)lifetime);

#if defined(PSA_KEY_ID_USER_MIN)
	LOG_INF("key_id: 0x%08x", (unsigned int)psa_get_key_id(&attr));
#endif

	status = psa_crypto_init();
	LOG_INF("PSA crypto initialized: %d", status);
	zassert_equal(status, PSA_SUCCESS, "Crypto init failed");

	status = psa_import_key(&attr, (const uint8_t *)ccmp256_key, key_length, &key_id);
	LOG_INF("psa_import_key returned: %d", status);

	/* Exactly as in the log trace, expect -135 (PSA_ERROR_INVALID_HANDLE or similar) or
	 * failure.
	 */
	zassert_not_equal(status, PSA_SUCCESS, "Key import should fail for WPA params");

	/* We do not expect key_id to be valid but if it is, destroy to be sure */
	if (status == PSA_SUCCESS) {
		(void)psa_destroy_key(key_id);
	}
}

ZTEST_SUITE(wifi_crypto, NULL, NULL, NULL, NULL, NULL);
