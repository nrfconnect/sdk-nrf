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

	wifi_keys_type_t type = PEER_UCST_ENC;
	uint32_t db_id = 0;
	uint32_t key_index = 0;
	uint32_t key_length = wifi_keys_get_key_size_in_bytes(type);

	uint32_t dest_address = wifi_keys_get_key_start_addr(type, db_id, key_index);
	zassert_not_equal(dest_address, WIFI_KEYS_ADDR_INVALID);

	kmu_push_key_buffer_t kmu_push_buffer = {
		.usage_mask = KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY | KMU_PUSH_USAGE_MASK_DESTROY_AFTER,
		.dest_address = dest_address,
		.key_buffer = (uint8_t *)ccmp256_key,
		.buffer_size = key_length
	};

	psa_key_attributes_t attr = wifi_keys_key_attributes_init(key_length);
	psa_key_id_t key_id;
	psa_status_t status;

	status = psa_crypto_init();
	zassert_equal(status, PSA_SUCCESS);

	status = psa_import_key(&attr, (const uint8_t *)&kmu_push_buffer, sizeof kmu_push_buffer, &key_id);
	zassert_equal(status, PSA_SUCCESS);

	*(volatile uint32_t *)0x48086C04 = 1; /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO1OUT */
	while (*(volatile uint32_t *)0x48086C00 !=
	       2) { /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO0OUT */
	}
}

ZTEST_SUITE(wifi_crypto, NULL, NULL, NULL, NULL, NULL);
