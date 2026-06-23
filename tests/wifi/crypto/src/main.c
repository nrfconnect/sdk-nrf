/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "wifi_kmu.h"

LOG_MODULE_REGISTER(wifi_crypto_test, LOG_LEVEL_DBG);

ZTEST(wifi_crypto, test_main)
{
	/* First, test slot index rotation */

	/* Starting slot is randomised so can't be tested easily.
	   Immediately request maximum number of slots, to force it to start
	   at the beginning. */
	uint32_t slot = wifi_kmu_get_next_slot(CONFIG_NRF_WIFI_KMU_NUM_SLOTS * 16);

	slot = wifi_kmu_get_next_slot(16);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN);

	slot = wifi_kmu_get_next_slot(32);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN + 1);

	slot = wifi_kmu_get_next_slot(32);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN + 3);

	/* Jump ahead to test wrap around behaviour */
	slot = wifi_kmu_get_next_slot((CONFIG_NRF_WIFI_KMU_NUM_SLOTS - 6) * 16);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN + 5);

	/* There should be one slot remaining now */
	slot = wifi_kmu_get_next_slot(16);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN + CONFIG_NRF_WIFI_KMU_NUM_SLOTS - 1);
	slot = wifi_kmu_get_next_slot(16);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN);

	/* Jump ahead again */
	slot = wifi_kmu_get_next_slot((CONFIG_NRF_WIFI_KMU_NUM_SLOTS - 2) * 16);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN + 1);

	/* Try to allocate 2 slots with only one slot remaining */
	slot = wifi_kmu_get_next_slot(32);
	zassert_equal(slot, CONFIG_NRF_WIFI_KMU_SLOT_MIN);

	/* Next, test actual encryption */

	uint32_t ccmp256_key[8] = {0x0C0D0E0F, 0x08090A0B, 0x04050607, 0x00010203,
				   0xF2BDD52F, 0x514A8A19, 0xCE371185, 0xC97C1F67};

	wifi_kmu_key_type_t type = PEER_UCST_ENC;
	uint32_t db_id = 0;
	uint32_t key_index = 0;
	uint32_t key_length = wifi_kmu_get_key_size_in_bytes(type);

	uint32_t dest_address = wifi_kmu_get_key_start_addr(type, db_id, key_index);
	zassert_not_equal(dest_address, WIFI_KMU_KEY_ADDR_INVALID);

	int err = wifi_kmu_write_key(slot, dest_address, (uint8_t *)ccmp256_key, key_length);
	zassert_equal(err, 0);

	*(volatile uint32_t *)0x48086C04 = 1; /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO1OUT */
	while (*(volatile uint32_t *)0x48086C00 !=
	       2) { /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO0OUT */
	}
}

ZTEST_SUITE(wifi_crypto, NULL, NULL, NULL, NULL, NULL);
