/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <errno.h>
#include <fw_info.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/devicetree.h>

/* The NVM is accessed through the same nrfx driver that fw_info uses to
 * invalidate metadata, so that the test does not depend on the write block
 * size and erase semantics the Zephyr flash driver exposes on top of it.
 */
#if defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#elif defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#else
#error Test needs alignment to the platform
#endif

#define TEST_PARTITION	s1_partition
#define TEST_ADDRESS	PARTITION_ADDRESS(TEST_PARTITION)
#define TEST_SIZE	PARTITION_SIZE(TEST_PARTITION)

/* Assumptions for test supported environment */
#define ERASE_PAGE_SIZE 0x1000

/* Address is used as offset with NVM writes and to access
 * structures via processor address space. Assumption was that they
 * are the same.
 */
BUILD_ASSERT(TEST_ADDRESS == PARTITION_OFFSET(TEST_PARTITION));

/* The nrfx word write functions take a word count and require a word aligned
 * source. struct fw_info is packed, so the instances below have to ask for the
 * alignment explicitly.
 */
BUILD_ASSERT(sizeof(struct fw_info) % sizeof(uint32_t) == 0);

/* signature check of this will fail, causing .valid to be overwritten. */
const struct fw_info dummy_s1 __aligned(sizeof(uint32_t)) = {
	.magic = {FIRMWARE_INFO_MAGIC},
	.address = TEST_ADDRESS,
	.size = TEST_SIZE,
	.version = 0xFF,
	.valid = CONFIG_FW_INFO_VALID_VAL,
};

/* Written over a dummy fw_info once a test is done with it. The zeroed magic
 * makes fw_info_check() reject it, so the next run of the test enters the
 * fw_info setup path again. Zeroing works on both NVMC and RRAMC, as neither
 * needs an erase to clear bits.
 */
static const struct fw_info discarded_s1 __aligned(sizeof(uint32_t)) = {0};

static uint8_t fw_info_find_buf[0x1000 + sizeof(dummy_s1)];

/* On NVMC the page has to be erased before a new fw_info can be written to it.
 * RRAM can be overwritten in place.
 */
static void nvm_erase_page(uint32_t address)
{
#if defined(CONFIG_NRFX_NVMC)
	int ret = nrfx_nvmc_page_erase(address);

	zassert_equal(0, ret, "nrfx_nvmc_page_erase failed %d", ret);
#else
	ARG_UNUSED(address);
#endif
}

static void nvm_write_fw_info(uint32_t address, const struct fw_info *src)
{
#if defined(CONFIG_NRFX_RRAMC)
	nrfx_rramc_words_write(address, src, sizeof(*src) / sizeof(uint32_t));
#else
	nrfx_nvmc_words_write(address, src, sizeof(*src) / sizeof(uint32_t));
#endif
}

ZTEST(test_fw_info, test_fw_info_invalidated)
{
	const struct fw_info *target = (const struct fw_info *)(TEST_ADDRESS);

	/* Write a dummy upgrade to S1 */
	if (!fw_info_check((uint32_t)target)) {
		TC_PRINT("#### Writing dummy fw_info of s1 ####\n");

		nvm_erase_page((uint32_t)target);
		nvm_write_fw_info((uint32_t)target, &dummy_s1);

		zassert_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
				"wrong valid value");

		TC_PRINT("#### REBOOTING ####\n\n");
		sys_reboot(0);
	}

	/* The s1 fw_info has been faked and boot should detect this, prior to
	 * booting the test, and invalidate the fw_info, which means t should
	 * be set to value different from CONFIG_FW_INFO_VALID_VAL.
	 */
	zassert_not_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
		"expected target to be invalidated 0x%x", target->valid);

	/* Reset NVM so test will enter the fw_info setup path on next run */
	nvm_write_fw_info((uint32_t)target, &discarded_s1);
}

ZTEST(test_fw_info, test_fw_info_invalidate)
{
	/* We will write fw_info, valid, two pages from the beginning of partitions;
	 * next fw_info_invalidate is called and we will check if invalidation was
	 * done ok.
	 */
	const struct fw_info *target = (const struct fw_info *)(TEST_ADDRESS + ERASE_PAGE_SIZE * 2);
	struct fw_info re_read_fw_info;

	/* Write a dummy upgrade to S1 */
	nvm_erase_page((uint32_t)target);
	nvm_write_fw_info((uint32_t)target, &dummy_s1);
	zassert_equal(CONFIG_FW_INFO_VALID_VAL, target->valid, "wrong valid value");

	/* Now we should have the fw_info invalidated */
	fw_info_invalidate(target);
	/* Check the valid field against the valid literal */
	zassert_not_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
		"expected target to be invalidated 0x%x", target->valid);

	/* We will read the entire fw_info to re_read_fw_info for comparison,
	 * then .valid field will be set to CONFIG_FW_INFO_VALID_VAL, and the
	 * re-read struct will be compared against dummy_s1. The comparison
	 * should indicate no difference, as the only field that was supposed to
	 * be touched by invalidation has been reset in the re-read struct.
	 */
	memcpy(&re_read_fw_info, target, sizeof(struct fw_info));

	re_read_fw_info.valid = CONFIG_FW_INFO_VALID_VAL;

	zassert_ok(memcmp(&dummy_s1, &re_read_fw_info, sizeof(struct fw_info)),
		"modification spilled out of .valid");

	/* Reset NVM so test will enter the fw_info setup path on next run */
	nvm_write_fw_info((uint32_t)target, &discarded_s1);
}


ZTEST(test_fw_info, test_fw_info_find)
{
	for (uint32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		const struct fw_info *fwinfo_res;

		memset(fw_info_find_buf, 0xFF, sizeof(fw_info_find_buf));
		memcpy(&fw_info_find_buf[fw_info_allowed_offsets[i]], &dummy_s1,
			sizeof(dummy_s1));
		fwinfo_res = fw_info_find((uint32_t)fw_info_find_buf);
		zassert_equal_ptr(&fw_info_find_buf[fw_info_allowed_offsets[i]],
			fwinfo_res,
			"fw_info_find returned 0x%x", (uint32_t)fwinfo_res);
	}
}

static void *test_fw_info_setup(void)
{
#if defined(CONFIG_NRFX_RRAMC)
	/* Disable write buffering and the preload timeout, so that writes reach
	 * RRAM before the test reads them back through the processor address
	 * space. fw_info_invalidate() sets up RRAMC the same way, and may have
	 * gotten there first, hence the reconfiguration on -EALREADY.
	 */
	nrfx_rramc_config_t config = NRFX_RRAMC_DEFAULT_CONFIG(0);

	config.preload_timeout_enable = false;
	config.preload_timeout = 0;

	int ret = nrfx_rramc_init(&config, NULL);

	if (ret == -EALREADY) {
		ret = nrfx_rramc_reconfigure(&config);
	}
	zassert_equal(0, ret, "RRAMC setup failed %d", ret);
#else
	/* Check if erase-block-size is supported */
	uint32_t page_size = nrfx_nvmc_flash_page_size_get();

	zassert_equal(ERASE_PAGE_SIZE, page_size, "unsupported erase page size %u", page_size);
#endif

	return NULL;
}

ZTEST_SUITE(test_fw_info, NULL, test_fw_info_setup, NULL, NULL, NULL);
