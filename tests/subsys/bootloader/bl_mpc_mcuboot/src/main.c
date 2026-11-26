/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <hal/nrf_mpc.h>
#include <hal/nrf_rramc.h>
#include <nrfx_rramc.h>
#include <bootutil/image.h>
#include <bl_mpc.h>

/* General-purpose overrides plus the access-blocking override. */
#define MPC_OVERRIDES_CHECKED (BL_MPC_REGION_COUNT_MAX + 1)

#define PART_START(label) DT_PARTITION_ADDR(DT_NODELABEL(label))
#define PART_END(label)	  (PART_START(label) + DT_REG_SIZE(DT_NODELABEL(label)))

#define PERM_R	(NRF_MPC_PERM_READ_MASK | NRF_MPC_PERM_SECURE_MASK)
#define PERM_RX (PERM_R | NRF_MPC_PERM_EXECUTE_MASK)
#define PERM_RW (PERM_R | NRF_MPC_PERM_WRITE_MASK)

struct expected_override {
	uint32_t start;
	uint32_t end;
	uint32_t perm;
};

static uint32_t slot0_code_end(void)
{
	const struct image_header *hdr = (const struct image_header *)PART_START(slot0_partition);

	uint32_t code_end;

	zassert_equal(hdr->ih_magic, IMAGE_MAGIC, "No image header at slot0");

	code_end = ROUND_UP(PART_START(slot0_partition) + hdr->ih_hdr_size + hdr->ih_img_size,
			    BL_MPC_GRANULARITY_BYTES);
#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD)
	code_end = MIN(code_end, PART_END(slot0_partition) - BL_MPC_GRANULARITY_BYTES);
#endif
	return code_end;
}

static void perm_print(uint32_t perm, char *str)
{
	str[0] = (perm & NRF_MPC_PERM_READ_MASK) ? 'R' : '-';
	str[1] = (perm & NRF_MPC_PERM_WRITE_MASK) ? 'W' : '-';
	str[2] = (perm & NRF_MPC_PERM_EXECUTE_MASK) ? 'X' : '-';
	str[3] = '\0';
}

static void rram_word_write_back(uint32_t address)
{
	nrf_rramc_word_write(address, *(volatile uint32_t *)address);
	__DSB();
}

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Expected fault caught, reason %u\n", reason);
	nrfx_rramc_write_enable_set(false, 0);
	ztest_test_pass();
}

ZTEST(bl_mpc_mcuboot, test_overrides_locked)
{
	uint32_t code_end = slot0_code_end();
	const struct expected_override expected[] = {
#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD)
		{PART_START(slot0_partition), code_end, PERM_R},
		{code_end, PART_END(slot1_partition), PERM_RW},
		{PART_START(storage_partition), PART_END(storage_partition), PERM_RW},
#else
		{PART_START(slot0_partition), code_end, PERM_RX},
		{code_end, PART_END(storage_partition), PERM_RW},
#endif
	};

	for (uint8_t i = 0; i < MPC_OVERRIDES_CHECKED; i++) {
		nrf_mpc_override_config_t config = nrf_mpc_override_config_get(NRF_MPC00, i);

		zassert_true(config.lock, "Override %u not locked", i);

		if (i >= ARRAY_SIZE(expected)) {
			zassert_false(config.enable, "Override %u enabled", i);
			continue;
		}

		uint32_t start = nrf_mpc_override_startaddr_get(NRF_MPC00, i);
		uint32_t end = nrf_mpc_override_endaddr_get(NRF_MPC00, i);
		uint32_t perm = nrf_mpc_override_perm_get(NRF_MPC00, i);
		char perm_str[4];

		perm_print(perm, perm_str);
		printk("MPC override %u start 0x%08x end 0x%08x perm %s locked\n", i, start, end,
		       perm_str);

		zassert_true(config.enable, "Override %u disabled", i);
		zassert_equal(start, expected[i].start, "Override %u start", i);
		zassert_equal(end, expected[i].end, "Override %u end", i);
		zassert_equal(perm, expected[i].perm, "Override %u perm", i);
	}

	printk("MPC overrides %u-%u locked disabled\n", (unsigned int)ARRAY_SIZE(expected),
	       MPC_OVERRIDES_CHECKED - 1);
}

ZTEST(bl_mpc_mcuboot, test_primary_slot_write_blocked)
{
	nrfx_rramc_write_enable_set(true, 0);
	nrf_mpc_event_clear(NRF_MPC00, NRF_MPC_EVENT_MEMACCERR);
	ztest_set_fault_valid(true);

	rram_word_write_back(PART_START(slot0_partition));

	ztest_set_fault_valid(false);
	nrfx_rramc_write_enable_set(false, 0);
	zassert_true(nrf_mpc_event_check(NRF_MPC00, NRF_MPC_EVENT_MEMACCERR),
		     "Write to the primary slot code was not blocked");
}

ZTEST(bl_mpc_mcuboot, test_rw_regions_writable)
{
	nrfx_rramc_write_enable_set(true, 0);

	rram_word_write_back(slot0_code_end());
	rram_word_write_back(PART_START(slot1_partition));
	rram_word_write_back(PART_START(storage_partition));

	nrfx_rramc_write_enable_set(false, 0);
}

ZTEST_SUITE(bl_mpc_mcuboot, NULL, NULL, NULL, NULL, NULL);
