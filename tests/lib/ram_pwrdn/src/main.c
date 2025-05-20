/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_power.h>
#include <ram_pwrdn.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <inttypes.h>
#include <stdlib.h>
#include <malloc.h>

/* ===== Test helpers ===== */

struct bank_section {
	uint8_t bank_id;
	uint8_t sect_id;
};

static struct bank_section bank_section(uint8_t bank_id, uint8_t section_id)
{
	struct bank_section ret = { .bank_id = bank_id, .sect_id = section_id };

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

static bool get_first_down_section(struct bank_section *out)
{
	bool down_section_found = false;

	for (uint8_t bank_id = 0; bank_id <= 8; ++bank_id) {
		for (uint8_t sect_id = 0; sect_id < (bank_id == 8 ? 6 : 2); ++sect_id) {
			bool is_up = check_section_up(bank_id, sect_id);

			if (is_up && down_section_found) {
				return false;
			}

			if (!is_up && !down_section_found) {
				out->bank_id = bank_id;
				out->sect_id = sect_id;
				down_section_found = true;
			}
		}
	}

	return down_section_found;
}

/* ===== Test cases ===== */

/*
 * Use a global extern pointer to hold the allocated buffer to prevent the
 * compiler from optimizing away malloc()/free().
 */
void *allocated_buffer;

ZTEST(ram_pwrdn, test_heap_resize)
{
	const size_t buffer_size = 20480; // 20KiB

	struct bank_section limit;
	struct bank_section limit_saved;

	/* Save the current limit between powered up and down RAM areas */
	zassert_true(get_first_down_section(&limit_saved), "Enabled RAM limit not found");

	/* Allocate some memory and verify that the limit has moved forward */
	allocated_buffer = malloc(buffer_size);
	memset(allocated_buffer, 0, buffer_size);

	zassert_true(get_first_down_section(&limit), "Enabled RAM limit not found after malloc");
	zassert_true(limit.bank_id > limit_saved.bank_id || (limit.bank_id == limit_saved.bank_id &&
							     limit.sect_id > limit_saved.sect_id),
		     "Enabled RAM limit not moved forward after malloc");

	/* Trim memory and verify that the limit has moved backward */
	free(allocated_buffer);
	malloc_trim(0);
	limit_saved.bank_id = limit.bank_id;
	limit_saved.sect_id = limit.sect_id;

	zassert_true(get_first_down_section(&limit), "Enabled RAM limit not found after malloc");
	zassert_true(limit.bank_id < limit_saved.bank_id || (limit.bank_id == limit_saved.bank_id &&
							     limit.sect_id < limit_saved.sect_id),
		     "Enabled RAM limit not moved backward after malloc");
}

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

ZTEST_SUITE(ram_pwrdn, NULL, NULL, NULL, NULL, NULL);
