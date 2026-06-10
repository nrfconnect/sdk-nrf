/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#if defined(CONFIG_NRFX_MRAMC)
#include <hal/nrf_mramc.h>
#include <nrfx_mramc.h>
#else
#include <hal/nrf_rramc.h>
#include <nrfx_rramc.h>
#endif

#if USE_PARTITION_MANAGER
#include <pm_config.h>

#if defined(CONFIG_TEST_MCUBOOT_ALONE)
	#define TEST_REGION_SIZE   PM_MCUBOOT_SIZE
	#define TEST_REGION_ADDRESS PM_MCUBOOT_ADDRESS
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_TEST_B0_LOCK_USE_S1)
	#define TEST_REGION_SIZE   PM_S1_IMAGE_SIZE
	#define TEST_REGION_ADDRESS PM_S1_IMAGE_ADDRESS
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_BOOTLOADER_MCUBOOT)
	#define TEST_REGION_SIZE   PM_MCUBOOT_SIZE
	#define TEST_REGION_ADDRESS PM_MCUBOOT_ADDRESS
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#else
	#define TEST_REGION_SIZE   PM_B0_SIZE
	#define TEST_REGION_ADDRESS PM_B0_ADDRESS
	#define TEST_REGION CONFIG_TEST_IMMUTABLE_REGION
#endif

#else
#include <zephyr/storage/flash_map.h>
#define RWX_SKIP_SIZE           CONFIG_SB_IMAGE_BOOT_OFFSET

#if defined(CONFIG_TEST_MCUBOOT_ALONE)
	#define TEST_REGION_SIZE   PARTITION_SIZE(boot_partition)
	#define TEST_REGION_ADDRESS PARTITION_OFFSET(boot_partition)
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_TEST_B0_LOCK_USE_S1)
	#define TEST_REGION_SIZE   (PARTITION_SIZE(s1_partition) - RWX_SKIP_SIZE)
	#define TEST_REGION_ADDRESS (PARTITION_OFFSET(s1_partition) + RWX_SKIP_SIZE)
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_BOOTLOADER_MCUBOOT)
	#define TEST_REGION_SIZE   (PARTITION_SIZE(s0_partition) - RWX_SKIP_SIZE)
	#define TEST_REGION_ADDRESS (PARTITION_OFFSET(s0_partition) + RWX_SKIP_SIZE)
	#define TEST_REGION CONFIG_TEST_ADDITIONAL_REGION
#else
	#define TEST_REGION_SIZE   PARTITION_SIZE(b0_partition)
	#define TEST_REGION_ADDRESS PARTITION_OFFSET(b0_partition)
	#define TEST_REGION CONFIG_TEST_IMMUTABLE_REGION
#endif
#endif

static uint32_t expected_fatal;
static uint32_t actual_fatal;
#if defined(CONFIG_NRFX_MRAMC)
/* The MRAMC region SIZE field caps at 31 KiB; a larger next-stage slot is
 * locked by chaining TEST_REGION and TEST_REGION + 1.
 */
#define NVM_REGION_MAX_SIZE_KB 31
static nrf_mramc_region_config_t config;
static uint32_t config_address;
#else
static nrf_rramc_region_config_t config;
#endif

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

#if defined(CONFIG_NRFX_MRAMC)
/* MRAMC exposes per-region R/W/X as booleans; address via a separate accessor. */
void *get_config(void)
{
	nrf_mramc_region_config_get(NRF_MRAMC, TEST_REGION, &config);
	config_address = nrf_mramc_region_address_get(NRF_MRAMC, TEST_REGION);
#if defined(CONFIG_TEST_B0_LOCK_READS)
	zassert_false(config.read || config.write || config.execute,
		"Read Write and eXecute permissions aren't cleared");
#else
	zassert_false(config.write, "Write permission isn't cleared");
#endif
	zassert_equal(config_address, TEST_REGION_ADDRESS,
		"Protected region start address doesn't match protected partition start address.");

	if (TEST_REGION_SIZE / 1024 > NVM_REGION_MAX_SIZE_KB) {
		/* Chained lock: TEST_REGION + 1 covers the rest of the slot. */
		nrf_mramc_region_config_t next = {0};
		uint32_t next_address;

		nrf_mramc_region_config_get(NRF_MRAMC, TEST_REGION + 1, &next);
		next_address = nrf_mramc_region_address_get(NRF_MRAMC, TEST_REGION + 1);
#if defined(CONFIG_TEST_B0_LOCK_READS)
		zassert_false(next.read || next.write || next.execute,
			"Chained region permissions aren't cleared");
#else
		zassert_false(next.write, "Chained region write isn't cleared");
#endif
		zassert_equal(next_address, config_address + config.size * 1024,
			"Chained region isn't contiguous with the first.");
		zassert_equal(config.size + next.size, TEST_REGION_SIZE / 1024,
			"Chained regions don't cover the protected slot.");
	} else {
		zassert_equal(config.size, TEST_REGION_SIZE / 1024,
			"Protected region size doesn't match protected partition size.");
	}
	return NULL;
}

/* Re-enabling R/W/X on a locked MRAMC region must take no effect. */
static void assert_region_locked(uint8_t region)
{
	nrf_mramc_region_config_t attempt;

	nrf_mramc_region_config_get(NRF_MRAMC, region, &attempt);
	attempt.read = true;
	attempt.write = true;
	attempt.execute = true;
	nrf_mramc_region_config_set(NRF_MRAMC, region, &attempt);
	nrf_mramc_region_config_get(NRF_MRAMC, region, &attempt);
	zassert_false(attempt.write, "Managed to update the locked config.");
}

ZTEST(b0_self_lock_test, test_rwx_locked_region)
{
	printk("Region %d\n", TEST_REGION);
	uint32_t protected_end_address = config_address + TEST_REGION_SIZE;
	volatile uint32_t *unprotected_word = (volatile uint32_t *)protected_end_address;
	volatile uint32_t *protected_word =
		(volatile uint32_t *)protected_end_address - sizeof(uint32_t);
	/* The locked region(s) must reject re-enabling R/W/X. */
	assert_region_locked(TEST_REGION);
	if (TEST_REGION_SIZE / 1024 > NVM_REGION_MAX_SIZE_KB) {
		assert_region_locked(TEST_REGION + 1);
	}

#if defined(CONFIG_TEST_B0_LOCK_READS)
	printk("Legal read\n");
	int val = *unprotected_word;

	printk("Illegal read\n");
	expected_fatal++;
	__DSB();
	val = *protected_word;
	(void)val;
#else
	uint32_t test_value = ~(*unprotected_word);
	nrfx_mramc_config_t mramc_config = NRFX_MRAMC_DEFAULT_CONFIG();

	/* Initialise the driver so DIRECT-mode writes commit. */
	(void)nrfx_mramc_init(&mramc_config, NULL);

	printk("Legal write\n");
	/* Next line corrupts application header.
	 * It is ok since we need to run test once per flashing.
	 */
	nrfx_mramc_word_write((uint32_t)unprotected_word, test_value);
	zassert_equal(test_value, *unprotected_word,
			"Legal write value doesn't match.");
	printk("Illegal write\n");
	expected_fatal++;
	__DSB();
	nrfx_mramc_word_write((uint32_t)protected_word, test_value);
#endif
}

#else /* RRAMC (nRF54L) */
void *get_config(void)
{
	nrf_rramc_region_config_get(NRF_RRAMC,
			TEST_REGION,
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
	zassert_equal(config.size_kb, TEST_REGION_SIZE / 1024,
		      "Protected region size doesn't match protected partition size.");
	zassert_equal(
		config.address, TEST_REGION_ADDRESS,
		"Protected region start address doesn't match protected partition start address.");
	return NULL;
}

ZTEST(b0_self_lock_test, test_rwx_locked_region)
{
	printk("Region %d\n", TEST_REGION);
	uint32_t protected_end_address = config.address + (1024 * config.size_kb);
	volatile uint32_t *unprotected_word = (volatile uint32_t *)protected_end_address;
	volatile uint32_t *protected_word =
		(volatile uint32_t *)protected_end_address - sizeof(uint32_t);
	uint32_t original_config = nrf_rramc_region_config_raw_get(NRF_RRAMC,
								   TEST_REGION);
	uint32_t updated_config = 0xFFFFFFFF;

	config.permissions = NRF_RRAMC_REGION_PERM_READ_MASK |
			     NRF_RRAMC_REGION_PERM_WRITE_MASK |
			     NRF_RRAMC_REGION_PERM_EXECUTE_MASK;
	/* Try unlocking. This should take no effect at this point */
	nrf_rramc_region_config_set(NRF_RRAMC, TEST_REGION, &config);

	updated_config = nrf_rramc_region_config_raw_get(NRF_RRAMC, TEST_REGION);
	zassert_equal(updated_config, original_config,
		"Managed to update the locked config.");

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
#endif /* CONFIG_NRFX_MRAMC */

ZTEST_SUITE(b0_self_lock_test, NULL, get_config, NULL, check_fatal, NULL);
