/*
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * On-hardware test for the RAM power-down library, written against the public
 * API only so it runs on any supported family.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ram_pwrdn.h>

#define REBOOT_MARKER 0x5A11EDA7u
#define PROBE_PATTERN 0xCAFEBABEu
#define RESTORE_PATTERN 0xA5A5A5A5u

/* Lives inside the application image, so it is always powered. */
static volatile uint32_t work_buf[256];

/* Kept across the software reset (a warm reset does not clear RAM). */
static __noinit uint32_t reboot_marker;

static void exercise_ram(void)
{
	uint32_t sum = 0;

	for (size_t i = 0; i < ARRAY_SIZE(work_buf); ++i) {
		work_buf[i] = (uint32_t)(i * 7u + 1u);
	}

	for (size_t i = 0; i < ARRAY_SIZE(work_buf); ++i) {
		sum += work_buf[i];
	}

	printk("ram_pwrdn: RAM check sum=%u\n", sum);
}

/* Word in RAM that the library powers down (the image never reaches this far).
 */
static volatile uint32_t *unused_ram_word(void)
{
	uintptr_t ram_end = DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) +
			    DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
	uintptr_t probe = ram_end - sizeof(uint32_t);

#if defined(CONFIG_SOC_NRF7120)
	/* RAM03 cannot be fully powered down, so probe the top of RAM02. */
	probe = DT_REG_ADDR(DT_NODELABEL(ram02_sram)) +
		DT_REG_SIZE(DT_NODELABEL(ram02_sram)) - sizeof(uint32_t);
#endif

	return (volatile uint32_t *)probe;
}

/* Returns true if the unused RAM was confirmed to be powered down. */
static bool verify_power_down(void)
{
	volatile uint32_t *unused_ram = unused_ram_word();

	*unused_ram = PROBE_PATTERN;
	work_buf[0] = PROBE_PATTERN;

	power_down_unused_ram();
	printk("ram_pwrdn: powered down unused RAM\n");

	/* Some families gate the RAM only once the SoC enters System ON idle. */
	printk("ram_pwrdn: entering System ON idle\n");
	k_msleep(1000);
	printk("ram_pwrdn: woke from idle\n");

	power_up_unused_ram();
	printk("ram_pwrdn: powered up unused RAM\n");

	/* Read only after power-up: on some families accessing powered-down RAM
	 * faults. Losing the pattern proves the RAM lost power.
	 */
	if (*unused_ram == PROBE_PATTERN) {
		printk("ram_pwrdn: ERROR unused RAM kept its content (still powered)\n");
		return false;
	}
	printk("ram_pwrdn: unused RAM lost content (powered down)\n");

	if (work_buf[0] != PROBE_PATTERN) {
		printk("ram_pwrdn: ERROR used RAM lost its content\n");
		return false;
	}
	printk("ram_pwrdn: used RAM retained content\n");

	return true;
}

/* Returns true if unused RAM is accessible again (power restored on reboot). */
static bool verify_unused_ram_accessible(void)
{
	volatile uint32_t *unused_ram = unused_ram_word();

	*unused_ram = RESTORE_PATTERN;
	if (*unused_ram != RESTORE_PATTERN) {
		printk("ram_pwrdn: ERROR unused RAM not accessible after reboot\n");
		return false;
	}
	printk("ram_pwrdn: unused RAM accessible after reboot\n");

	return true;
}

int main(void)
{
	if (reboot_marker == REBOOT_MARKER) {
		reboot_marker = 0;
		printk("ram_pwrdn: boot after reboot\n");
		exercise_ram();
		if (!verify_unused_ram_accessible()) {
			return 0;
		}
		printk("ram_pwrdn: RAM access after reboot OK\n");
		printk("ram_pwrdn: TEST PASS\n");
		return 0;
	}

	printk("ram_pwrdn: initial boot\n");
	exercise_ram();

	if (!verify_power_down()) {
		return 0;
	}

	exercise_ram();
	printk("ram_pwrdn: RAM access after power cycle OK\n");

	/* Reboot with unused RAM powered down; it must be restored before reset. */
	reboot_marker = REBOOT_MARKER;
	power_down_unused_ram();
	printk("ram_pwrdn: rebooting with RAM powered down\n");
	k_msleep(100);
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}
