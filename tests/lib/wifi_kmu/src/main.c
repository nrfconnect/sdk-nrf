/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <nrfx_kmu.h>
#include <nrfx_mramc.h>
#include <nrf_security_mem_helpers.h>

#include "wifi_kmu.h"

LOG_MODULE_REGISTER(wifi_kmu_test, LOG_LEVEL_DBG);

/* SICR page for KMU on nRF71 series devices */
#define MRAM_CONFIGNVR_SICR_PAGE 3

/* Size of each key slot in bytes */
#define KEY_SLOT_SIZE_BYTES (KEY_SLOT_WORDS_COUNT * 4)

/* Max key size for this test */
#define MAX_KEY_SIZE (KEY_SLOT_SIZE_BYTES * CONFIG_NRF_WIFI_KMU_NUM_SLOTS)

#define SLOT_ID_FOR_TEST (CONFIG_NRF_WIFI_KMU_SLOT_MIN)

/** @brief Buffer to store the target key */
uint8_t target_buffer[MAX_KEY_SIZE] = {0};

/** @brief test pattern for a cryptographic key to push */
const uint8_t test_key[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

BUILD_ASSERT(sizeof(test_key) == KEY_SLOT_SIZE_BYTES, "Update the test_key to KMU size");

/** @brief Fill the key with the test pattern */
static void fill_key(uint8_t *key_buffer, size_t key_size)
{
	size_t num_slots = DIV_ROUND_UP(key_size, KEY_SLOT_SIZE_BYTES);
	size_t block_size = 0;
	size_t size_left = key_size;

	for (size_t i = 0; i < num_slots; i++) {
		block_size = size_left > KEY_SLOT_SIZE_BYTES ? KEY_SLOT_SIZE_BYTES : size_left;
		memcpy(key_buffer, test_key, block_size);
		key_buffer += block_size;
		size_left -= block_size;
	}
}

static void check_none_filled(void)
{
	int err = -EFAULT;
	bool is_empty = false;

	err = nrfx_kmu_key_slots_empty_check(CONFIG_NRF_WIFI_KMU_SLOT_MIN,
					     CONFIG_NRF_WIFI_KMU_NUM_SLOTS,
					     &is_empty);
	zassert_equal(err, 0, "nrfx_kmu_key_slots_empty_check returned err: %d", err);
	zassert_true(is_empty, "KMU was not empty (unexpected)");
}

static void check_key_value(const uint8_t *key_buffer, size_t key_size)
{
	bool is_equal = (constant_memcmp(target_buffer, key_buffer, key_size) == 0);

	zassert_true(is_equal, "Pushed KMU key not the same. (Size: %d)", key_size);
}

static void clear_wifi_kmu_keys(void)
{
	int err = wifi_kmu_erase_keys();

	zassert_equal(err, 0, "Failed to erase WIFI KMU keys. slot ID:%d, num_slots: %d",
		      CONFIG_NRF_WIFI_KMU_SLOT_MIN, CONFIG_NRF_WIFI_KMU_NUM_SLOTS);
}

ZTEST(wifi_kmu, test_key_writes)
{
	int err = -EINVAL;
	uint8_t key_buffer[MAX_KEY_SIZE] = {0};

	clear_wifi_kmu_keys();

	/* Ensure that KMU slots are empty */
	check_none_filled();

	for (size_t key_size = 8; key_size <= MAX_KEY_SIZE; key_size += 8) {
		LOG_DBG("Test key write: key_size: %d", key_size);
		safe_memzero(target_buffer, sizeof(target_buffer));
		safe_memzero(key_buffer, sizeof(key_buffer));
		fill_key(key_buffer, key_size);
		err = wifi_kmu_write_key(SLOT_ID_FOR_TEST, (uint32_t)(uintptr_t)target_buffer,
					 key_buffer, key_size);
		zassert_equal(err, 0, "Write key failed: err: %d, (size: %d)", err, key_size);

		check_key_value(key_buffer, key_size);
		check_none_filled();
	}
}

ZTEST(wifi_kmu, test_key_erase)
{
	int err = -EFAULT;
	nrfx_kmu_key_slot_data_t slot_data = {0};

	clear_wifi_kmu_keys();
	check_none_filled();

	slot_data.revoke_policy = NRFX_KMU_RPOLICY_ROTATING;
	memcpy(slot_data.keyslot_value, test_key, sizeof(test_key));

	nrfx_mramc_confignvr_perm_set(true, MRAM_CONFIGNVR_SICR_PAGE);
	err = nrfx_kmu_key_slot_provision(&slot_data, CONFIG_NRF_WIFI_KMU_SLOT_MIN);
	nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_SICR_PAGE);

	zassert_equal(err, 0, "Failed to provision dummy key for erase check");

	err = wifi_kmu_erase_keys();

	zassert_equal(err, 0, "wifi_kmu_erase_keys failed for lingering key: %d", err);
}

ZTEST_SUITE(wifi_kmu, NULL, NULL, NULL, NULL, NULL);
