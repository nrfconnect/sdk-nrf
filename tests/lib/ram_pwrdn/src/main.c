/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

#include <ram_pwrdn.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <inttypes.h>
#include <stdlib.h>
#include <malloc.h>

#if defined(CONFIG_SOC_NRF52840)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_NRF7120_ENGA_CPUAPP)
#if defined(CONFIG_SOC_SERIES_NRF71_TFM_RAM_CTRL_SERVICE)
#include "tfm_ioctl_core_api.h"
#else
#include <hal/nrf_memconf.h>
#endif
#endif

#if defined(CONFIG_SOC_NRF52840)
/* ===== Test helpers ===== */

struct bank_section {
	uint8_t bank_id;
	uint8_t sect_id;
};

static struct bank_section bank_section(uint8_t bank_id, uint8_t section_id)
{
	struct bank_section ret = {.bank_id = bank_id, .sect_id = section_id};

	return ret;
}

static bool check_section_up(uint8_t bank_id, uint8_t section_id)
{
	uint32_t mask = nrf_power_rampower_mask_get(NRF_POWER, bank_id);

	return mask & (NRF_POWER_RAMPOWER_S0POWER_MASK << section_id);
}

static bool check_section_range(struct bank_section first, struct bank_section last, bool up)
{
	for (uint8_t bank_id = first.bank_id; bank_id <= last.bank_id; ++bank_id) {
		uint8_t first_sect_id = bank_id == first.bank_id ? first.sect_id : 0;
		uint8_t last_sect_id = bank_id == last.bank_id ? last.sect_id : 1;

		for (uint8_t sect_id = first_sect_id; sect_id <= last_sect_id; ++sect_id) {
			if (check_section_up(bank_id, sect_id) != up) {
				return false;
			}
		}
	}

	return true;
}

static void teardown(void *f)
{
	const uintptr_t RAM_START_ADDR = 0x20000000UL;
	const uintptr_t RAM_END_ADDR = 0x20040000UL;

	power_up_ram(RAM_START_ADDR, RAM_END_ADDR);
}

/* ===== Test cases ===== */

ZTEST(ram_pwrdn, test_manual_power_control)
{
	const uintptr_t RAM_START_ADDR = 0x20000000UL;
	const uintptr_t RAM_END_ADDR = 0x20040000UL;
	const uintptr_t RAM_BANK_SECTION_SIZE = 0x1000UL;
	const uintptr_t RAM_BANK8_ADDR = 0x20010000UL;
	const uintptr_t RAM_BANK8_SECTION_SIZE = 0x8000UL;

	/* Power up all sections */
	power_up_ram(RAM_START_ADDR, RAM_END_ADDR);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 5), true),
		     "Enabling all RAM sections");

	/* Verify that powering down part of RAM section is not effective */
	power_down_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE + 1, RAM_END_ADDR);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 5), true),
		     "Disabling part of RAM section");

	/* Verify that powering down entire RAM section works */
	power_down_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE, RAM_END_ADDR);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 4), true),
		     "Disabling single RAM section (disabled too much)");
	zassert_true(check_section_range(bank_section(8, 5), bank_section(8, 5), false),
		     "Disabling single RAM section (failed)");

	/* Verify that powering down RAM section plus one byte has the same effect */
	power_down_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE, RAM_END_ADDR);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 4), true),
		     "Disabling more than RAM section (disabled too much)");
	zassert_true(check_section_range(bank_section(8, 5), bank_section(8, 5), false),
		     "Disabling more than RAM section (failed)");

	/* Power down last three sections */
	power_down_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE * 3, RAM_END_ADDR);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 2), true),
		     "Disabling three RAM sections (disabled too much)");
	zassert_true(check_section_range(bank_section(8, 3), bank_section(8, 5), false),
		     "Disabling three RAM sections (failed)");

	/* Verify that powering up one byte is enough to enable entire RAM section */
	power_up_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE * 3,
		     RAM_END_ADDR - RAM_BANK8_SECTION_SIZE * 3 + 1);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 3), true),
		     "Enabling one byte (failed)");
	zassert_true(check_section_range(bank_section(8, 4), bank_section(8, 5), false),
		     "Enabling one byte (enabled too much)");

	/* Verify that powering up entire RAM section has the same effect */
	power_up_ram(RAM_END_ADDR - RAM_BANK8_SECTION_SIZE * 3,
		     RAM_END_ADDR - RAM_BANK8_SECTION_SIZE * 2);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(8, 3), true),
		     "Enabling single RAM section (failed)");
	zassert_true(check_section_range(bank_section(8, 4), bank_section(8, 5), false),
		     "Enabling single RAM section (enabled too much)");

	/* Power down sections on the border between two banks */
	power_down_ram(RAM_BANK8_ADDR - RAM_BANK_SECTION_SIZE - 1,
		       RAM_BANK8_ADDR + RAM_BANK8_SECTION_SIZE);
	zassert_true(check_section_range(bank_section(0, 0), bank_section(7, 0), true),
		     "Disabling sections on two banks (disabled too much)");
	zassert_true(check_section_range(bank_section(7, 1), bank_section(8, 0), false),
		     "Disabling sections on two banks (failed)");
	zassert_true(check_section_range(bank_section(8, 1), bank_section(8, 3), true),
		     "Disabling sections on two banks (disabled too much)");
}

ZTEST_SUITE(ram_pwrdn, NULL, NULL, NULL, teardown, NULL);
#elif defined(CONFIG_SOC_NRF7120_ENGA_CPUAPP)

#define RAM_SECTION_SIZE  0x8000UL
#define TEST_SECTION_ID	  30U
#define TEST_SECTION_ADDR (0x20000000UL + TEST_SECTION_ID * RAM_SECTION_SIZE)

static uint32_t control_get(void)
{
#if defined(CONFIG_SOC_SERIES_NRF71_TFM_RAM_CTRL_SERVICE)
	uint32_t control;
	enum tfm_platform_err_t err;

	err = tfm_platform_ram_ctrl_read_status(&control, NULL, NULL);
	zassert_equal(err, TFM_PLATFORM_ERR_SUCCESS, "Reading MEMCONF CONTROL through TF-M");

	return control;
#else
	return NRF_MEMCONF->POWER[0].CONTROL;
#endif
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	power_up_ram(TEST_SECTION_ADDR, TEST_SECTION_ADDR + RAM_SECTION_SIZE);
}

ZTEST(ram_pwrdn, test_nrf7120_power_control)
{
	uint32_t control;
	uint32_t section31_before;

	power_up_ram(TEST_SECTION_ADDR, TEST_SECTION_ADDR + RAM_SECTION_SIZE);
	control = control_get();
	zassert_true(control & BIT(TEST_SECTION_ID), "Test section is not powered up");
	section31_before = control & BIT(31);

	/* A section is powered down only when the complete section is requested. */
	power_down_ram(TEST_SECTION_ADDR + 1U, TEST_SECTION_ADDR + RAM_SECTION_SIZE);
	control = control_get();
	zassert_true(control & BIT(TEST_SECTION_ID), "A partial section was powered down");
	zassert_equal(control & BIT(31), section31_before, "Section 31 changed");

	power_down_ram(TEST_SECTION_ADDR, TEST_SECTION_ADDR + RAM_SECTION_SIZE);
	control = control_get();
	zassert_false(control & BIT(TEST_SECTION_ID), "A complete section was not powered down");
	zassert_equal(control & BIT(31), section31_before, "Section 31 changed");

	/* Any overlap powers the complete section back up. */
	power_up_ram(TEST_SECTION_ADDR, TEST_SECTION_ADDR + 1U);
	control = control_get();
	zassert_true(control & BIT(TEST_SECTION_ID), "The section was not powered up");
	zassert_equal(control & BIT(31), section31_before, "Section 31 changed");
}

ZTEST_SUITE(ram_pwrdn, NULL, NULL, NULL, teardown, NULL);
#else
#error "RAM power-down test is not supported on the current platform"
#endif
