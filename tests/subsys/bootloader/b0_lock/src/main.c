/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <hal/nrf_rramc.h>

#define RRAMC_REGION_FOR_BOOTCONF 3
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
			RRAMC_REGION_FOR_BOOTCONF,
			&config);
	zassert_equal(0, config.permissions &
			(NRF_RRAMC_REGION_PERM_READ_MASK |
			 NRF_RRAMC_REGION_PERM_WRITE_MASK |
			 NRF_RRAMC_REGION_PERM_EXECUTE_MASK),
			"Read Write and eXecute permissions aren't cleared");
	zassert_true(config.size_kb > 0, "Protected region has zero size.");
	return NULL;
}

ZTEST(b0_self_lock_test, test_reading_b0_image)
{
	uint32_t protected_end_address = 1024 * config.size_kb;
	int val;

	printk("Legal read\n");
	val = *((volatile int*)protected_end_address);
	config.permissions = NRF_RRAMC_REGION_PERM_READ_MASK |
			     NRF_RRAMC_REGION_PERM_WRITE_MASK |
			     NRF_RRAMC_REGION_PERM_EXECUTE_MASK;
	/* Try unlocking. This should take no effect at this point */
	nrf_rramc_region_config_set(NRF_RRAMC, RRAMC_REGION_FOR_BOOTCONF, &config);
	printk("Illegal read\n");
	expected_fatal++;
	val = *((volatile int*)protected_end_address-1);
}

ZTEST_SUITE(b0_self_lock_test, NULL, get_config, NULL, check_fatal, NULL);
