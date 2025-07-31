/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <hal/nrf_rramc.h>
#include <nrfx_rramc.h>

#define RRAMC_REGION_FOR_TEST CONFIG_TEST_B0_LOCK_REGION

static uint32_t expected_fatal;
static uint32_t actual_fatal;
static nrf_rramc_region_config_t config;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

void check_fatal(void *unused)
{
	zassert_equal(expected_fatal, actual_fatal,
		"Wrong number of fatal errors has occurred (e:%d != a:%d).",
		expected_fatal, actual_fatal);
}

void *get_config(void)
{
	nrf_rramc_region_config_get(NRF_RRAMC,
			RRAMC_REGION_FOR_TEST,
			&config);
#if defined(CONFIG_TEST_B0_LOCK_READS)
	zassert_equal(0, config.permissions &
			(NRF_RRAMC_REGION_PERM_READ_MASK |
			 NRF_RRAMC_REGION_PERM_WRITE_MASK |
			 NRF_RRAMC_REGION_PERM_EXECUTE_MASK),
			"Read Write and eXecute permissions aren't cleared");
#else
	zassert_equal(0, config.permissions &
			(NRF_RRAMC_REGION_PERM_WRITE_MASK),
			"Write permission isn't cleared");
#endif
	zassert_true(config.size_kb > 0, "Protected region has zero size.");
	return NULL;
}

ZTEST(b0_self_lock_test, test_reading_b0_image)
{
	printk("Region %d\n", RRAMC_REGION_FOR_TEST);
	uint32_t protected_end_address = 1024 * config.size_kb;
	volatile uint32_t *unprotected_word = (volatile uint32_t *)protected_end_address;
	volatile uint32_t *protected_word =
		(volatile uint32_t *)protected_end_address - sizeof(uint32_t);

	config.permissions = NRF_RRAMC_REGION_PERM_READ_MASK |
			     NRF_RRAMC_REGION_PERM_WRITE_MASK |
			     NRF_RRAMC_REGION_PERM_EXECUTE_MASK;
	/* Try unlocking. This should take no effect at this point */
	nrf_rramc_region_config_set(NRF_RRAMC, RRAMC_REGION_FOR_TEST, &config);

#if defined(CONFIG_TEST_B0_LOCK_READS)
	printk("Legal read\n");
	int val = *unprotected_word;

	printk("Illegal read\n");
	expected_fatal++;
	__DSB();
	val = *protected_word;
#else
	uint32_t test_value = ~(*unprotected_word);

	printk("Legal write\n");
	nrfx_rramc_write_enable_set(true, 0);
	/* Next line corrupts application header.
	 * It is ok since we need to run test once per flashing.
	 * Moreover after reboot slot will be invalidated and
	 * application will boot from second one.
	 */
	nrf_rramc_word_write((uint32_t)unprotected_word, test_value);
	zassert_equal(test_value, *unprotected_word,
			"Legal write value doesn't match.");
	printk("Illegal write\n");
	expected_fatal++;
	__DSB();
	nrf_rramc_word_write((uint32_t)protected_word, test_value);

#endif

}

ZTEST_SUITE(b0_self_lock_test, NULL, get_config, NULL, check_fatal, NULL);
