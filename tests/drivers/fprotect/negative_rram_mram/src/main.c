/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Negative fprotect tests for nRF54L (RRAM) and nRF7120 (MRAM) platforms.
 *
 * Complements drivers.fprotect.negative, which targets flash-only SoCs with
 * secure boot. This test uses a dedicated storage partition (see board
 * overlays) rather than the application image region.
 *
 * Memory layout within storage_partition:
 *   ZONE_A - unprotected (writable)
 *   ZONE_B - protected via fprotect_area() in suite setup
 *   ZONE_C - unprotected (writable)
 *
 * RRAM/MRAM fprotect allows only one fprotect_area() call per boot, so zone B
 * is locked once in suite_setup() and remains protected for all tests in a run.
 *
 * Tests are split into three Twister scenarios (see tests.yaml and Kconfig):
 *   basic       - misaligned fprotect_area(), unprotected zone writability
 *   boundary    - faulting direct stores across protection edges
 *   flash_write - faulting flash_write() on the protected zone
 *
 * Each scenario is a separate image/run so a flash_write() fault cannot leave
 * the driver lock held for later tests. Within a scenario, ztest shuffle is
 * enabled and tests must not depend on execution order.
 *
 * Boundary tests use direct CPU stores (not the flash driver) because a fault
 * during flash_write() aborts the test thread while holding the driver lock.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <fprotect.h>
#include <string.h>

#if defined(CONFIG_SOC_FLASH_NRF_RRAM)
#include <hal/nrf_rramc.h>
#elif defined(CONFIG_SOC_FLASH_NRF_MRAMC)
#include <nrfx_mramc.h>
#endif

#define STORAGE_PARTITION storage_partition
#define STORAGE_DEV       PARTITION_DEVICE(STORAGE_PARTITION)
#define STORAGE_ADDR      PARTITION_OFFSET(STORAGE_PARTITION)
#define STORAGE_SIZE      PARTITION_SIZE(STORAGE_PARTITION)
#define FPROTECT_BLOCK    CONFIG_FPROTECT_BLOCK_SIZE

/* Unprotected zone */
#define ZONE_A            STORAGE_ADDR
/* Protected zone */
#define ZONE_B            (STORAGE_ADDR + FPROTECT_BLOCK)
/* Unprotected zone */
#define ZONE_C            (STORAGE_ADDR + (2 * FPROTECT_BLOCK))

/* Middle of ZONE_A */
#define ZONE_A_MIDDLE (ZONE_A + (FPROTECT_BLOCK / 2))
/* Middle of ZONE_B */
#define ZONE_B_MIDDLE (ZONE_B + (FPROTECT_BLOCK / 2))

#define ZONE_A_OVERWRITE_VALUE 0xA5
#define ZONE_B_OVERWRITE_VALUE 0xB5
#define ZONE_C_OVERWRITE_VALUE 0xC5

BUILD_ASSERT(STORAGE_ADDR % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "storage area must be FPROTECT-block aligned");
BUILD_ASSERT(STORAGE_SIZE >= (3 * FPROTECT_BLOCK),
	     "storage partition must fit three fprotect blocks");

static volatile uint32_t expected_fatal;
static uint32_t actual_fatal;

static uint8_t initial_zone_b[FPROTECT_BLOCK];
static uint8_t post_lock_a[FPROTECT_BLOCK];
static uint8_t post_lock_c[FPROTECT_BLOCK];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

static void write_pattern(uint32_t addr, uint8_t pattern)
{
	uint8_t buf[FPROTECT_BLOCK];

	memset(buf, pattern, sizeof(buf));
	zassert_equal(0, flash_write(STORAGE_DEV, addr, buf, sizeof(buf)),
		      "flash_write failed @0x%x", addr);
}

static void read_pattern(uint32_t addr, uint8_t *dest)
{
	zassert_equal(0, flash_read(STORAGE_DEV, addr, dest, FPROTECT_BLOCK),
		      "flash_read failed @0x%x", addr);
}

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BASIC || \
	CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY
static void expect_patterns_equal(uint32_t addr, const uint8_t *expected)
{
	uint8_t buf[FPROTECT_BLOCK];

	read_pattern(addr, buf);
	zassert_mem_equal(buf, expected, FPROTECT_BLOCK,
			  "unexpected content @0x%x", addr);
}
#endif

static void *suite_setup(void)
{
	zassert_true(device_is_ready(STORAGE_DEV), "flash device not ready");

	write_pattern(ZONE_B, 0xBB);
	read_pattern(ZONE_B, initial_zone_b);

	zassert_equal(0, fprotect_area(ZONE_B, FPROTECT_BLOCK),
		      "fprotect_area failed");

	return NULL;
}

static void reset_zones_baseline(void)
{
	write_pattern(ZONE_A, ZONE_A_OVERWRITE_VALUE);
	write_pattern(ZONE_C, ZONE_C_OVERWRITE_VALUE);

	memset(post_lock_a, ZONE_A_OVERWRITE_VALUE, sizeof(post_lock_a));
	memset(post_lock_c, ZONE_C_OVERWRITE_VALUE, sizeof(post_lock_c));
}

static void before_test(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_zones_baseline();
}

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BASIC || \
	CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY
static void verify_all_zones(void)
{
	expect_patterns_equal(ZONE_A, post_lock_a);
	expect_patterns_equal(ZONE_B, initial_zone_b);
	expect_patterns_equal(ZONE_C, post_lock_c);
}
#endif

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY
/*
 * Trigger a CPU bus fault by writing to a protected region. The flash driver
 * must not be used here: a fault during flash_write() aborts the test thread
 * while holding the driver lock, which hangs subsequent tests.
 */
static void direct_store_expecting_fault(uint32_t addr, const void *data, size_t len)
{
#if defined(CONFIG_SOC_FLASH_NRF_RRAM)
	nrf_rramc_config_t config = {
		.mode_write = true,
		.write_buff_size = CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE,
	};

	nrf_rramc_config_set(NRF_RRAMC, &config);
	memcpy((void *)(uintptr_t)addr, data, len);
	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#elif defined(CONFIG_SOC_FLASH_NRF_MRAMC)
	const uint8_t *src = data;

	/* MRAM stores in 16-byte words (write-block-size). Callers pass
	 * FPROTECT_BLOCK-aligned lengths, so no partial tail is needed.
	 */
	for (size_t off = 0; off < len; off += 16) {
		uint32_t mapped = addr + off;

		nrfx_mramc_config_write_mode_set(1);
		while (!nrfx_mramc_ready_check()) {
		}

		uint32_t *dst = (uint32_t *)mapped;
		const uint32_t *words = (const uint32_t *)(src + off);

		for (int i = 0; i < 4; i++) {
			dst[i] = words[i];
		}

		nrfx_mramc_config_write_mode_set(0);
		while (!nrfx_mramc_ready_check()) {
		}
	}
#else
	BUILD_ASSERT(false, "unsupported fprotect storage backend");
#endif
}

static void expect_faulting_direct_store(uint32_t addr, size_t len)
{
	uint8_t buf[FPROTECT_BLOCK];

	zassert_true(len <= sizeof(buf), "unexpected write length");
	memset(buf, ZONE_B_OVERWRITE_VALUE, len);

	TC_PRINT("NOTE: expect bus fault on write @0x%x len %u\n", addr, len);
	zassert_equal(expected_fatal, actual_fatal,
		      "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	direct_store_expecting_fault(addr, buf, len);
}
#endif /* CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY */

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BASIC
ZTEST(fprotect_negative_rram_mram, test_misaligned_rejected)
{
	zassert_equal(-EINVAL,
		      fprotect_area(ZONE_B + 1, CONFIG_FPROTECT_BLOCK_SIZE));
}

ZTEST(fprotect_negative_rram_mram, test_unprotected_zones_writable)
{
	TC_PRINT("Verify unprotected zones A and C are writable; zone B stays locked\n");
	verify_all_zones();
}
#endif /* CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BASIC */

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY
ZTEST(fprotect_negative_rram_mram, test_whole_zone_b)
{
	TC_PRINT("Overwrite whole protected zone B; expect bus fault\n");
	expect_faulting_direct_store(ZONE_B, FPROTECT_BLOCK);
	TC_PRINT("Verify zones after whole-B write fault\n");
	verify_all_zones();
}

ZTEST(fprotect_negative_rram_mram, test_cross_a_to_b)
{
	TC_PRINT("Cross-boundary write A->B; expect bus fault at zone B\n");
	expect_faulting_direct_store(ZONE_A_MIDDLE, FPROTECT_BLOCK);
	TC_PRINT("Verify zones after A->B cross write (upper A may change)\n");
	/*
	 * The A->B cross write stores to the unprotected upper half of zone A
	 * before faulting at the start of zone B.
	 */
	memset(&post_lock_a[FPROTECT_BLOCK / 2], ZONE_B_OVERWRITE_VALUE, FPROTECT_BLOCK / 2);
	verify_all_zones();
}

ZTEST(fprotect_negative_rram_mram, test_cross_b_to_c)
{
	TC_PRINT("Cross-boundary write B->C; expect bus fault in zone B\n");
	expect_faulting_direct_store(ZONE_B_MIDDLE, FPROTECT_BLOCK);
	TC_PRINT("Verify zones after B->C cross write fault\n");
	verify_all_zones();
}
#endif /* CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_BOUNDARY */

#if CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_FLASH_WRITE
ZTEST(fprotect_negative_rram_mram, test_flash_write_zone_b)
{
	uint8_t buf[FPROTECT_BLOCK];

	memset(buf, ZONE_B_OVERWRITE_VALUE, sizeof(buf));
	TC_PRINT("NOTE: expect bus fault on flash_write to protected zone B\n");
	zassert_equal(expected_fatal, actual_fatal,
		      "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	zassert_equal(0, flash_write(STORAGE_DEV, ZONE_B, buf, sizeof(buf)),
		      "flash_write should not return");
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}
#endif /* CONFIG_TEST_FPROTECT_NEGATIVE_RRAM_MRAM_FLASH_WRITE */

static void check_fatal(void *unused)
{
	ARG_UNUSED(unused);

	zassert_equal(expected_fatal, actual_fatal,
		      "The wrong number of fatal error has occurred (e:%d != a:%d).\n",
		      expected_fatal, actual_fatal);
}

ZTEST_SUITE(fprotect_negative_rram_mram, NULL, suite_setup,
	    before_test, check_fatal, NULL);
