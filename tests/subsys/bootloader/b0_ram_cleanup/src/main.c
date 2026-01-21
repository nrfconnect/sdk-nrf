/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <hal/nrf_timer.h>
#include <zephyr/drivers/retained_mem.h>

/* As peripheral registers are not cleared during RAM cleanup, we can
 * use a register of a peripheral unused by NSIB to save data which can then
 * be read by the application.
 * We use this trick to save the start and end addresses of the RAMFUNC region.
 * This region is not cleared during the RAM cleanup, as it contains the code responsible
 * for performing the RAM cleanup, so it must be skipped when checking if the RAM cleanup
 * has been successfully performed.
 * The TIMER00 is not used by NSIB, and is therefore its CC[0] and CC[1] registers
 * are used for this purpose.
 */
#define RAMFUNC_START_SAVE_REGISTER NRF_TIMER00->CC[0]
#define RAMFUNC_END_SAVE_REGISTER NRF_TIMER00->CC[1]

/* Using 2-byte magic values allows to load the whole value with movw */
#define RAM_CLEANUP_SUCCESS_MAGIC 0x5678
#define RAM_CLEANUP_FAILURE_MAGIC 0x4321

static uint32_t __noinit ram_cleanup_result;
static uint32_t __noinit uncleared_address;
static uint32_t __noinit uncleared_value;

#define EXPECTED_RETAINED_DATA "RETAINED"

/**
 * This hook needs to have the attribute __attribute__((naked)),
 * as otherwise the stack would be modified when calling it, leading to
 * test failure, as part of the RAM in which the stack resides wouldn't
 * be zeroed.
 */
__attribute__((naked)) void soc_early_reset_hook(void)
{
	__asm__ volatile (
		/* Load zero (value used to clear memory) into r0 */
		"   mov  r0, #0\n"
		/*  Explicitly clear the ram_cleanup_result variable */
		"   mov  r2, %6\n"
		"   str  r0, [r2]\n"
		/* Load the location of the saved ram func start address to r1 */
		"   mov  r1, %0\n"
		/* Load the ram func start address to r2 */
		"   ldr  r2, [r1]\n"
		/* Load the location of the saved ram func end address to r1 */
		"   mov  r1, %1\n"
		/* Load the ram func end address to r3*/
		"   ldr  r3, [r1]\n"
		/* Clear memory from ram func start to ram func end.
		 * The area is skipped during the RAM cleanup,
		 * so it is not cleared.
		 * Simply cleaning it up here is quicker
		 * than verifying if the current address is within the gap
		 * in each iteration of the loop.
		 */
		"ram_func_zero_loop:\n"
		"   cmp  r2, r3\n"
		"   bge  ram_func_zero_loop_done\n"
		"   str  r0, [r2]\n"
		/* Increment the address by 4 (word size) */
		"   add  r2, r2, #4\n"
		"   b    ram_func_zero_loop\n"
		"ram_func_zero_loop_done:\n"
		/* Verify that all of the RAM memory has been cleared */
		/* Load SRAM base address to r1 */
		"   mov  r1, %2\n"
		/* Load SRAM size to r2 */
		"   mov  r2, %3\n"
		/* Calculate SRAM end address (base + size) */
		"   add  r2, r1, r2\n"
		"verify_ram_loop:\n"
		"   cmp  r1, r2\n"
		"   bge  verify_ram_done\n"
		/* Load value at current address and verify if it equals 0 */
		"   ldr  r3, [r1]\n"
		"   cmp  r3, #0\n"
		"   bne  ram_cleanup_failed\n"
		/* Increment the address by 4 (word size) */
		"   add  r1, r1, #4\n"
		"   b    verify_ram_loop\n"
		"ram_cleanup_failed:\n"
		/* Load RAM_CLEANUP_FAILURE_MAGIC into r0 */
		"   movw r0, %4\n"
		/* Save the uncleared address in uncleared_address variable */
		"   mov  r2, %7\n"
		"   str  r1, [r2]\n"
		/* Save the uncleared value in uncleared_value variable */
		"   mov  r2, %8\n"
		"   str  r3, [r2]\n"
		"   b    verification_done\n"
		"verify_ram_done:\n"
		/* Load RAM_CLEANUP_SUCCESS_MAGIC */
		"   movw r0, %5\n"
		"verification_done:\n"
		/* Store result in ram_cleanup_result variable */
		"   mov  r2, %6\n"
		"   str  r0, [r2]\n"
		/* __attribute__((naked)) requires manual branching  */
		"   bx   lr\n"
		:
		: "r" (&RAMFUNC_START_SAVE_REGISTER),
		  "r" (&RAMFUNC_END_SAVE_REGISTER),
		  "r" (CONFIG_SRAM_BASE_ADDRESS),
		  "r" (CONFIG_SRAM_SIZE * 1024),
		  "i" (RAM_CLEANUP_FAILURE_MAGIC),
		  "i" (RAM_CLEANUP_SUCCESS_MAGIC),
		  "r" (&ram_cleanup_result),
		  "r" (&uncleared_address),
		  "r" (&uncleared_value)
		: "r0", "r1", "r2", "r3", "lr", "memory"
	);
}

ZTEST(b0_ram_cleanup, test_ram_cleanup)
{
	zassert_true((ram_cleanup_result == RAM_CLEANUP_SUCCESS_MAGIC) ||
		     (ram_cleanup_result == RAM_CLEANUP_FAILURE_MAGIC),
		     "RAM cleanup result should be either success or failure");
	zassert_equal(ram_cleanup_result, RAM_CLEANUP_SUCCESS_MAGIC,
		      "Uncleared word detected at address %p, value 0x%x",
		      (void *) uncleared_address, uncleared_value);

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_retained_ram)
	const struct device *retained_mem_dev =
		DEVICE_DT_GET(DT_INST(0, zephyr_retained_ram));
	size_t retained_size;
	uint8_t buffer[strlen(EXPECTED_RETAINED_DATA)+1];

	zassert_true(device_is_ready(retained_mem_dev), "Retained memory device is not ready");

	retained_size = retained_mem_size(retained_mem_dev);

	zassert_true(retained_size >= strlen(EXPECTED_RETAINED_DATA),
		     "Retained memory size is too small");

	retained_mem_read(retained_mem_dev, 0, buffer, strlen(EXPECTED_RETAINED_DATA));
	zassert_mem_equal(buffer, EXPECTED_RETAINED_DATA, strlen(EXPECTED_RETAINED_DATA),
			  "Retained data is not correct");
#endif
}

ZTEST_SUITE(b0_ram_cleanup, NULL, NULL, NULL, NULL, NULL);
