/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>

#include <suit.h>
#include <suit_platform.h>
#include <suit_storage.h>
#include <suit_mci.h>

#ifdef CONFIG_ARCH_POSIX
#include <zephyr/drivers/flash.h>
#define SOC_NV_FLASH_NODE	    DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_FLASH_SIZE  DT_REG_SIZE(SOC_NV_FLASH_NODE)
#endif /* CONFIG_ARCH_POSIX */

/* The SUIT envelope is defined inside the board-specific manfest_*.c file. */
extern uint8_t manifest_buf[];
extern const size_t manifest_len;

static void *test_suit_setup(void)
{
	int err = suit_mci_init();

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to initialize MCI module");

#ifdef CONFIG_ARCH_POSIX
	static const struct device *const flash_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	err = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET, FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, err, "Unable to erase flash area before test execution");
#endif /* CONFIG_ARCH_POSIX */

	return NULL;
}

ZTEST_SUITE(fetch_integrated_payload_flash_tests, NULL, test_suit_setup, NULL, NULL, NULL);

/** @brief Authenticate, validate and execute all command sequences from the sample SUIT envelope.
 *
 * @details The main purpose of this test is to give an entry point for SUIT processor platform
 * implementation. If the platform design requires it, please update the envelope payload, defined
 * at the beginning of this file.
 */
ZTEST(fetch_integrated_payload_flash_tests, test_suit_process)
{
	int err = SUIT_SUCCESS;

	err = suit_processor_init();
	zassert_equal(err, SUIT_SUCCESS, "Unable to initialise SUIT processor (err: %d)", err);

	err = suit_storage_init();
	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init suit storage (%d)", err);

	err = suit_process_sequence(manifest_buf, manifest_len, SUIT_SEQ_PARSE);
	zassert_equal(err, SUIT_SUCCESS, "Parsing SUIT envelope failed (err: %d)", err);

	for (enum suit_command_sequence i = SUIT_SEQ_PARSE + 1;
	     (i < SUIT_SEQ_MAX) && (err == SUIT_SUCCESS); i++) {
		err = suit_process_sequence(manifest_buf, manifest_len, i);
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			err = SUIT_SUCCESS;
		}
		zassert_equal(err, SUIT_SUCCESS, "Processing SUIT sequence %d failed with error %i",
			      i, err);
	}
}
