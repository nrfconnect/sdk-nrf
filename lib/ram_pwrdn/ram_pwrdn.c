/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <stdint.h>

#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52833)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <hal/nrf_vmc.h>
#else
#error "RAM power-down library is not supported on the current platform"
#endif

#if CONFIG_RAM_POWER_ADJUST_ON_HEAP_RESIZE
#include <zephyr/init.h>
#include <zephyr/sys/heap_listener.h>
#endif

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#include <pm_config.h>
#endif

#define RAM_IMAGE_END_ADDR ((uintptr_t)_image_ram_end)
#define RAM_BANK_COUNT ARRAY_SIZE(banks)

LOG_MODULE_REGISTER(ram_pwrdn, CONFIG_RAM_POWERDOWN_LOG_LEVEL);

struct ram_bank {
	uintptr_t start;
	uint8_t section_count;
	uint16_t section_size;
};

extern char _image_ram_end[];

static const struct ram_bank banks[] = {
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52833)
	{ .start = 0x20000000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x20002000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x20004000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x20006000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x20008000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x2000A000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x2000C000UL, .section_count = 2, .section_size = 0x1000 },
	{ .start = 0x2000E000UL, .section_count = 2, .section_size = 0x1000 },
#if CONFIG_SOC_NRF52833
	{ .start = 0x20010000UL, .section_count = 2, .section_size = 0x8000 },
#else
	{ .start = 0x20010000UL, .section_count = 6, .section_size = 0x8000 },
#endif
#elif defined(CONFIG_SOC_NRF5340_CPUAPP)
	{ .start = 0x20000000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20010000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20020000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20030000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20040000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20050000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20060000UL, .section_count = 16, .section_size = 0x1000 },
	{ .start = 0x20070000UL, .section_count = 16, .section_size = 0x1000 },
#endif
};

/*
 * Power down selected RAM sections of the given bank.
 */
static void ram_bank_power_down(uint8_t bank_id, uint8_t first_section_id, uint8_t last_section_id)
{
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52833)
	uint32_t mask = GENMASK(NRF_POWER_RAMPOWER_S0POWER + last_section_id,
				NRF_POWER_RAMPOWER_S0POWER + first_section_id);
	nrf_power_rampower_mask_off(NRF_POWER, bank_id, mask);
#elif defined(CONFIG_SOC_NRF5340_CPUAPP)
	uint32_t mask = GENMASK(VMC_RAM_POWER_S0POWER_Pos + last_section_id,
				VMC_RAM_POWER_S0POWER_Pos + first_section_id);
	nrf_vmc_ram_block_power_clear(NRF_VMC, bank_id, mask);
#endif
}

/*
 * Power up selected RAM sections of the given bank.
 */
static void ram_bank_power_up(uint8_t bank_id, uint8_t first_section_id, uint8_t last_section_id)
{
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52833)
	uint32_t mask = GENMASK(NRF_POWER_RAMPOWER_S0POWER + last_section_id,
				NRF_POWER_RAMPOWER_S0POWER + first_section_id);
	nrf_power_rampower_mask_on(NRF_POWER, bank_id, mask);
#elif defined(CONFIG_SOC_NRF5340_CPUAPP)
	uint32_t mask = GENMASK(VMC_RAM_POWER_S0POWER_Pos + last_section_id,
				VMC_RAM_POWER_S0POWER_Pos + first_section_id);
	nrf_vmc_ram_block_power_set(NRF_VMC, bank_id, mask);
#endif
}

/*
 * Calculate size of the RAM bank.
 */
static uintptr_t ram_bank_size(const struct ram_bank *bank)
{
	return (uintptr_t)bank->section_count * (uintptr_t)bank->section_size;
}

/*
 * Return ID of the nearest RAM section with start address less or equal to the given address.
 *
 * If the address points before or after the RAM bank then 0 or the number of bank sections
 * is returned, respectively.
 */
static uint8_t ram_bank_section_id_floor(uintptr_t address, const struct ram_bank *bank)
{
	if (address < bank->start) {
		return 0;
	}

	if (address >= bank->start + ram_bank_size(bank)) {
		return bank->section_count;
	}

	return (uint8_t)((address - bank->start) / bank->section_size);
}

/*
 * Returns ID of the nearest RAM section with start address greater or equal to the given address.
 *
 * If the address points before or after the RAM bank then 0 or the number of bank sections
 * is returned, respectively.
 */
static uint8_t ram_bank_section_id_ceil(uintptr_t address, const struct ram_bank *bank)
{
	if (address < bank->start) {
		return 0;
	}

	if (address >= bank->start + ram_bank_size(bank)) {
		return bank->section_count;
	}

	return (uint8_t)(ROUND_UP(address - bank->start, bank->section_size) / bank->section_size);
}

/*
 * Returns end address of RAM managed by the Power Down library
 */
static uintptr_t ram_end_addr(void)
{
#if !defined(CONFIG_PARTITION_MANAGER_ENABLED)
	const struct ram_bank *last_bank = &banks[RAM_BANK_COUNT - 1];

	return last_bank->start + ram_bank_size(last_bank);
#else
	return PM_SRAM_PRIMARY_END_ADDRESS;
#endif
}

void power_down_ram(uintptr_t start_address, uintptr_t end_address)
{
	for (uint8_t bank_id = 0; bank_id < RAM_BANK_COUNT; ++bank_id) {
		const struct ram_bank *bank = &banks[bank_id];

		/* Determine bank sections which fully fall within the input address range */
		uint8_t section_begin = ram_bank_section_id_ceil(start_address, bank);
		uint8_t section_end = ram_bank_section_id_floor(end_address, bank);

		if (section_begin < section_end) {
			LOG_DBG("Powering down sections %u-%u of bank %u", section_begin,
				section_end - 1, bank_id);
			ram_bank_power_down(bank_id, section_begin, section_end - 1);
		}
	}
}

void power_up_ram(uintptr_t start_address, uintptr_t end_address)
{
	for (uint8_t bank_id = 0; bank_id < RAM_BANK_COUNT; ++bank_id) {
		const struct ram_bank *bank = &banks[bank_id];

		/* Determine bank sections which overlap with the input address range */
		uint8_t section_begin = ram_bank_section_id_floor(start_address, bank);
		uint8_t section_end = ram_bank_section_id_ceil(end_address, bank);

		if (section_begin < section_end) {
			LOG_DBG("Powering up sections %u-%u of bank %u", section_begin,
				section_end - 1, bank_id);
			ram_bank_power_up(bank_id, section_begin, section_end - 1);
		}
	}
}

void power_down_unused_ram(void)
{
	power_down_ram(RAM_IMAGE_END_ADDR, ram_end_addr());
}

void power_up_unused_ram(void)
{
	power_up_ram(RAM_IMAGE_END_ADDR, ram_end_addr());
}

#if CONFIG_RAM_POWER_ADJUST_ON_HEAP_RESIZE

static void libc_heap_resize_cb(uintptr_t heap_id, void *old_heap_end, void *new_heap_end)
{
	ARG_UNUSED(heap_id);

	if (new_heap_end > old_heap_end) {
		power_up_ram(RAM_IMAGE_END_ADDR, (uintptr_t)new_heap_end);
	} else if (new_heap_end < old_heap_end) {
		power_down_ram((uintptr_t)new_heap_end, ram_end_addr());
	}
}

static HEAP_LISTENER_RESIZE_DEFINE(heap_listener, HEAP_ID_LIBC, libc_heap_resize_cb);

static int ram_power_init(void)
{

	power_down_unused_ram();
	heap_listener_register(&heap_listener);

	return 0;
}

SYS_INIT(ram_power_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
