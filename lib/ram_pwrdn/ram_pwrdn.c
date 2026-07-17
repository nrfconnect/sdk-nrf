/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#include <helpers/nrfx_ram_ctrl.h>

#if defined(CONFIG_SOC_SERIES_NRF71_TFM_RAM_CTRL_SERVICE)
#include "tfm_ioctl_core_api.h"
#endif

#if !defined(NRF_MEMORY_RAM_BASE) && defined(NRF_MEMORY_RAM0_BASE)
#define NRF_MEMORY_RAM_BASE NRF_MEMORY_RAM0_BASE
#endif

#define RAM_IMAGE_END_ADDR ((uintptr_t)_image_ram_end)

#define RAM_UNIFORM_REGION_SIZE                                                                    \
	((uintptr_t)RAM_UNIFORM_SECTIONS_TOTAL * (uintptr_t)RAM_SECTION_UNIT_SIZE)

LOG_MODULE_REGISTER(ram_pwrdn, CONFIG_RAM_POWERDOWN_LOG_LEVEL);

extern char _image_ram_end[];

/* Contiguous RAM range in which every power-controllable section has the same size. */
struct ram_region {
	uintptr_t start;
	uintptr_t size;
	uintptr_t section_size;
};

static const struct ram_region ram_regions[] = {
	{
		.start = NRF_MEMORY_RAM_BASE,
		.size = RAM_UNIFORM_REGION_SIZE,
		.section_size = RAM_SECTION_UNIT_SIZE,
	},
#if defined(RAM_NON_UNIFORM_SECTIONS)
	{
		.start = NRF_MEMORY_RAM_BASE + RAM_UNIFORM_REGION_SIZE,
		.size = (uintptr_t)NRF_MEMORY_RAM_SIZE - RAM_UNIFORM_REGION_SIZE,
		.section_size =
			(uintptr_t)RAM_NON_UNIFORM_BLOCK_UNITS * (uintptr_t)RAM_SECTION_UNIT_SIZE,
	},
#endif
};

/*
 * When powering down, only sections that fall entirely within the range are affected, so that
 * a section still holding part of the application image is never turned off. When powering up,
 * every section that overlaps the range is affected.
 */
static void ram_sections_power_set(uintptr_t start_address, uintptr_t end_address, bool power_up)
{
	for (size_t i = 0; i < ARRAY_SIZE(ram_regions); ++i) {
		const struct ram_region *region = &ram_regions[i];
		uintptr_t region_end = region->start + region->size;
		uintptr_t range_start = MAX(start_address, region->start);
		uintptr_t range_end = MIN(end_address, region_end);
		uintptr_t section_start;
		uintptr_t section_end;

		if (range_start >= range_end) {
			continue;
		}

		if (power_up) {
			section_start = ROUND_DOWN(range_start, region->section_size);
			section_end = ROUND_UP(range_end, region->section_size);
		} else {
			section_start = ROUND_UP(range_start, region->section_size);
			section_end = ROUND_DOWN(range_end, region->section_size);
		}

		if (section_start < section_end) {
			LOG_DBG("%s RAM 0x%08lx-0x%08lx",
				power_up ? "Powering up" : "Powering down",
				(unsigned long)section_start, (unsigned long)section_end);
#if defined(CONFIG_SOC_SERIES_NRF71_TFM_RAM_CTRL_SERVICE)
			if (tfm_platform_ram_ctrl_power_set((uint32_t)section_start,
							    (uint32_t)(section_end - section_start),
							    power_up) != TFM_PLATFORM_ERR_SUCCESS) {
				LOG_ERR("TF-M rejected RAM power-%s range 0x%08lx-0x%08lx",
					power_up ? "up" : "down", (unsigned long)section_start,
					(unsigned long)section_end);
			}
#else
			nrfx_ram_ctrl_power_enable_set((void const *)section_start,
						       section_end - section_start, power_up);
#endif
		}
	}
}

static uintptr_t ram_end_addr(void)
{
	return DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
}

void power_down_ram(uintptr_t start_address, uintptr_t end_address)
{
	ram_sections_power_set(start_address, end_address, false);
}

void power_up_ram(uintptr_t start_address, uintptr_t end_address)
{
	ram_sections_power_set(start_address, end_address, true);
}

void power_down_unused_ram(void)
{
	power_down_ram(RAM_IMAGE_END_ADDR, ram_end_addr());
}

void power_up_unused_ram(void)
{
	power_up_ram(RAM_IMAGE_END_ADDR, ram_end_addr());
}
