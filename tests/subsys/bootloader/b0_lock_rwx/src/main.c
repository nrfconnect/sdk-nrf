/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#if defined(CONFIG_HAS_HW_NRF_MRAMC)
#include <hal/nrf_mramc.h>
#include <nrfx_mramc.h>
#else
#include <hal/nrf_rramc.h>
#include <nrfx_rramc.h>
#endif

#include <zephyr/storage/flash_map.h>
#define RWX_SKIP_SIZE           CONFIG_SB_IMAGE_BOOT_OFFSET

#if defined(CONFIG_TEST_MCUBOOT_ALONE)
	#define NVM_REGION_FOR_TEST_SIZE   PARTITION_SIZE(boot_partition)
	#define NVM_REGION_FOR_TEST_ADDRESS PARTITION_OFFSET(boot_partition)
	#define NVM_REGION_FOR_TEST CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_TEST_B0_LOCK_USE_S1)
	#define NVM_REGION_FOR_TEST_SIZE   (PARTITION_SIZE(s1_partition) - RWX_SKIP_SIZE)
	#define NVM_REGION_FOR_TEST_ADDRESS (PARTITION_OFFSET(s1_partition) + RWX_SKIP_SIZE)
	#define NVM_REGION_FOR_TEST CONFIG_TEST_ADDITIONAL_REGION
#elif defined(CONFIG_BOOTLOADER_MCUBOOT)
	#define NVM_REGION_FOR_TEST_SIZE   (PARTITION_SIZE(s0_partition) - RWX_SKIP_SIZE)
	#define NVM_REGION_FOR_TEST_ADDRESS (PARTITION_OFFSET(s0_partition) + RWX_SKIP_SIZE)
	#define NVM_REGION_FOR_TEST CONFIG_TEST_ADDITIONAL_REGION
#else
	#define NVM_REGION_FOR_TEST_SIZE   PARTITION_SIZE(b0_partition)
	#define NVM_REGION_FOR_TEST_ADDRESS PARTITION_OFFSET(b0_partition)
	#define NVM_REGION_FOR_TEST CONFIG_TEST_IMMUTABLE_REGION
#endif

static uint32_t expected_fatal;
static uint32_t actual_fatal;
#if defined(CONFIG_HAS_HW_NRF_MRAMC)
/* A single MRAMC region covers at most 31 KiB, larger areas continue in the next region. */
#define NVM_REGION_MAX_SIZE_KB 31
#define NVM_REGION_FOR_TEST_SIZE_KB (NVM_REGION_FOR_TEST_SIZE / 1024)
#define NVM_REGION_FOR_TEST_CHAINED (NVM_REGION_FOR_TEST_SIZE_KB > NVM_REGION_MAX_SIZE_KB)
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

#if defined(CONFIG_HAS_HW_NRF_MRAMC)
static void check_region(uint8_t region, uint32_t address, uint32_t size_kb)
{
	nrf_mramc_region_config_t cfg;

	nrf_mramc_region_config_get(NRF_MRAMC, region, &cfg);
#if defined(CONFIG_TEST_B0_LOCK_READS)
	zassert_false(cfg.read || cfg.write || cfg.execute,
		      "Read Write and eXecute permissions aren't cleared");
#else
	zassert_false(cfg.write, "Write permission isn't cleared");
#endif
	zassert_equal(cfg.size, size_kb,
		      "Protected region size doesn't match protected partition size.");
	zassert_equal(nrf_mramc_region_address_get(NRF_MRAMC, region), address,
		      "Protected region start address doesn't match protected partition start address.");
}

static void check_region_locked(uint8_t region)
{
	nrf_mramc_region_config_t cfg;
	uint32_t original_config = NRF_MRAMC->REGION[region].CONFIG;

	nrf_mramc_region_config_get(NRF_MRAMC, region, &cfg);
	cfg.read = true;
	cfg.write = true;
	cfg.execute = true;
	/* Try unlocking. This should take no effect at this point */
	nrf_mramc_region_config_set(NRF_MRAMC, region, &cfg);
	zassert_equal(NRF_MRAMC->REGION[region].CONFIG, original_config,
		      "Managed to update the locked config.");
}

void *get_config(void)
{
	uint32_t first_kb = MIN(NVM_REGION_FOR_TEST_SIZE_KB, NVM_REGION_MAX_SIZE_KB);

	check_region(NVM_REGION_FOR_TEST, NVM_REGION_FOR_TEST_ADDRESS, first_kb);
	if (NVM_REGION_FOR_TEST_CHAINED) {
		check_region(NVM_REGION_FOR_TEST + 1, NVM_REGION_FOR_TEST_ADDRESS + first_kb * 1024,
			     NVM_REGION_FOR_TEST_SIZE_KB - first_kb);
	}
	return NULL;
}

ZTEST(b0_self_lock_test, test_rwx_locked_region)
{
	printk("Region %d\n", NVM_REGION_FOR_TEST);
	uint32_t protected_end_address = NVM_REGION_FOR_TEST_ADDRESS + NVM_REGION_FOR_TEST_SIZE;
	volatile uint32_t *unprotected_word = (volatile uint32_t *)protected_end_address;
	volatile uint32_t *protected_word =
		(volatile uint32_t *)protected_end_address - sizeof(uint32_t);

	check_region_locked(NVM_REGION_FOR_TEST);
	if (NVM_REGION_FOR_TEST_CHAINED) {
		check_region_locked(NVM_REGION_FOR_TEST + 1);
	}

#if defined(CONFIG_TEST_B0_LOCK_READS)
	printk("Legal read\n");
	int val = *unprotected_word;

	printk("Illegal read\n");
	expected_fatal++;
	__DSB();
	val = *protected_word;
#else
	uint32_t test_value = ~(*unprotected_word);
	nrfx_mramc_config_t mramc_config = NRFX_MRAMC_DEFAULT_CONFIG();

	/* The driver programs the READYNEXT timeout required for writes to commit. */
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
#else
void *get_config(void)
{
	nrf_rramc_region_config_get(NRF_RRAMC,
			NVM_REGION_FOR_TEST,
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
	zassert_equal(config.size_kb, NVM_REGION_FOR_TEST_SIZE / 1024,
		      "Protected region size doesn't match protected partition size.");
	zassert_equal(
		config.address, NVM_REGION_FOR_TEST_ADDRESS,
		"Protected region start address doesn't match protected partition start address.");
	return NULL;
}

ZTEST(b0_self_lock_test, test_rwx_locked_region)
{
	printk("Region %d\n", NVM_REGION_FOR_TEST);
	uint32_t protected_end_address = config.address + (1024 * config.size_kb);
	volatile uint32_t *unprotected_word = (volatile uint32_t *)protected_end_address;
	volatile uint32_t *protected_word =
		(volatile uint32_t *)protected_end_address - sizeof(uint32_t);
	uint32_t original_config = nrf_rramc_region_config_raw_get(NRF_RRAMC,
								   NVM_REGION_FOR_TEST);
	uint32_t updated_config = 0xFFFFFFFF;

	config.permissions = NRF_RRAMC_REGION_PERM_READ_MASK |
			     NRF_RRAMC_REGION_PERM_WRITE_MASK |
			     NRF_RRAMC_REGION_PERM_EXECUTE_MASK;
	/* Try unlocking. This should take no effect at this point */
	nrf_rramc_region_config_set(NRF_RRAMC, NVM_REGION_FOR_TEST, &config);

	updated_config = nrf_rramc_region_config_raw_get(NRF_RRAMC, NVM_REGION_FOR_TEST);
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

#endif /* CONFIG_HAS_HW_NRF_MRAMC */

ZTEST_SUITE(b0_self_lock_test, NULL, get_config, NULL, check_fatal, NULL);
