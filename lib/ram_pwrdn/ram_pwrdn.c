/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <logging/log.h>


LOG_MODULE_REGISTER(ram_pwrdn, CONFIG_RAM_POWERDOWN_LOG_LEVEL);

#if defined CONFIG_BOARD_NRF52840DK_NRF52840 || \
	defined CONFIG_BOARD_NRF52833DK_NRF52833
#include <hal/nrf_power.h>

/* Start address of physical RAM. */
#define RAM_START_ADDR                0x20000000UL
/* Size of RAM section for banks 0-7. */
#define RAM_BANK_0_7_SECTION_SIZE     0x1000
/* Number of sections in banks 0-7. */
#define RAM_BANK_0_7_SECTIONS_NBR     2
/* Size of RAM section for bank 8. */
#define RAM_BANK_8_SECTION_SIZE       0x8000
/* Number of sections in bank 8. */
#define RAM_BANK_8_SECTIONS_NBR       6
/* Number of top RAM bank available at the SoC. */
#define RAM_BANKS_NBR                 8
/* End address of RAM used by the application. */
#define RAM_APP_END_ADDR              ((uint32_t)&_image_ram_end)
/* End of application RAM partition. */
#define RAM_APP_PARTITION_END_ADDR    (CONFIG_SRAM_SIZE * 1024UL + CONFIG_SRAM_BASE_ADDRESS)

extern char _image_ram_end;

#elif !defined(CONFIG_TRUSTED_EXECUTION_SECURE) && \
	defined(CONFIG_PARTITION_MANAGER_ENABLED) && \
	(defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP_NS))
#include <hal/nrf_vmc.h>
#include <pm_config.h>

/* Start address of physical RAM. */
#define RAM_START_ADDR                0x20000000UL
/* Size of RAM section for banks 0-7. */
#define RAM_BANK_0_7_SECTION_SIZE     0x1000
/* Number of sections in banks 0-7. */
#define RAM_BANK_0_7_SECTIONS_NBR     16
/* Size of RAM section for bank 8 - bank 8 not present at this SoC. */
#define RAM_BANK_8_SECTION_SIZE       0x0
/* Number of sections in bank 8 - bank 8 not present at this SoC. */
#define RAM_BANK_8_SECTIONS_NBR       0
/* Number of top RAM bank available at the SoC. */
#define RAM_BANKS_NBR                 7
/* End address of RAM used by the application. */
#define RAM_APP_END_ADDR              ((uint32_t)&_image_ram_end)

/* End of application RAM partition address.
 * This compares RAM  end addresses based on Partition Managern and DTS
 * to determine the correct RAM end address.
 * This is temporary solution - remove when RAM app partition end addr can be simply obtained.
 */
#define RAM_APP_END_ADDR_PM           PM_SRAM_PRIMARY_END_ADDRESS
#define RAM_APP_END_ADDR_DTS          (CONFIG_SRAM_SIZE * 1024UL + CONFIG_SRAM_BASE_ADDRESS)
#define RAM_APP_PARTITION_END_ADDR    ((RAM_APP_END_ADDR_PM > RAM_APP_END_ADDR_DTS) ? \
					(RAM_APP_END_ADDR_DTS) : (RAM_APP_END_ADDR_PM))

extern char _image_ram_end;

#else
#define UNUSED_RAM_POWER_OFF_UNSUPPORTED
#endif

#ifndef UNUSED_RAM_POWER_OFF_UNSUPPORTED

/**@brief Calculate bottom address of RAM bank with given ID.
 *
 * @param[in] bank_id  ID of RAM bank to get start address of.
 *
 * @return    Start address of RAM bank.
 */
static inline uint32_t ram_bank_bottom_addr(uint8_t bank_id)
{
	uint32_t bank_addr = RAM_START_ADDR + bank_id
			     * RAM_BANK_0_7_SECTION_SIZE
			     * RAM_BANK_0_7_SECTIONS_NBR;
	return bank_addr;
}

/**@brief Calculate top address of RAM bank with given ID.
 *
 * @param[in] bank_id  ID of RAM bank to get end address of.
 *
 * @return    End address of RAM bank.
 */
static inline uint32_t ram_bank_top_addr(uint8_t bank_id)
{
	uint32_t bank_addr;

	if (bank_id == 8) {
		bank_addr = ram_bank_bottom_addr(bank_id)
			    + RAM_BANK_8_SECTION_SIZE
			    * RAM_BANK_8_SECTIONS_NBR;
	} else {
		bank_addr = ram_bank_bottom_addr(bank_id + 1);
	}

	return bank_addr;
}

/**@brief Calculate bottom address of section of RAM bank with given bank
 *        and section IDs.
 *
 * @param[in] bank_id     ID of RAM bank section is placed in.
 * @param[in] section_id  ID of section of RAM bank to get start address of.
 *
 * @return    Start address of section of RAM bank.
 */
static uint32_t ram_sect_bank_bottom_addr(uint8_t bank_id, uint8_t section_id)
{
	/* Get base address of given RAM bank. */
	uint32_t section_addr = ram_bank_bottom_addr(bank_id);

	/* Calculate section address offset. */
	if (bank_id == 8) {
		section_addr += section_id * RAM_BANK_8_SECTION_SIZE;
	} else {
		section_addr += section_id * RAM_BANK_0_7_SECTION_SIZE;
	}

	return section_addr;
}

/**@brief Calculate top address of the RAM bank section with given bank
 *        and section IDs.
 *
 * @param[in] bank_id     ID of RAM bank section is placed in.
 * @param[in] section_id  ID of section of RAM bank to get end address of.
 *
 * @return    End address of the RAM bank section.
 */
static inline uint32_t ram_sect_bank_top_addr(uint8_t bank_id, uint8_t section_id)
{
	return ram_sect_bank_bottom_addr(bank_id, (section_id + 1));
}

/**@brief Power down selected RAM sections in given bank.
 *
 * @param[in] bank_id           ID of the RAM bank.
 * @param[in] power_off_mask    Mask of RAM sections to power down.
 */
static void power_down_ram_sections(uint8_t bank_id, uint32_t power_off_mask)
{
#if defined CONFIG_BOARD_NRF52840DK_NRF52840 || defined CONFIG_BOARD_NRF52833DK_NRF52833
	nrf_power_rampower_mask_off(NRF_POWER, bank_id,
				    (power_off_mask << NRF_POWER_RAMPOWER_S0POWER));
#elif defined CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP || \
	defined CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP_NS

	nrf_vmc_ram_block_power_clear(NRF_VMC, bank_id,
				      (power_off_mask << VMC_RAM_POWER_S0POWER_Pos));
#endif
}

/**@brief Power down RAM segments from given address range.
 *
 * @param[in] start_addr     Start address of RAM segments to power down.
 * @param[in] end_addr       End address of RAM segments to power down.
 */
static void power_down_ram_from_addr_range(uint32_t start_addr, uint32_t end_addr)
{
	uint8_t bank_id = RAM_BANKS_NBR + 1;
	uint8_t section_id = 0;
	uint32_t mask_off = 0;
	uint32_t ram_bank_power_off_mask = 0x0000FFFF;

	LOG_DBG("Powering off RAM from %#x to %#x", start_addr, end_addr);

	while (bank_id--) {
		/* Check if RAM bank region overlap with given address range, if not skip. */
		if ((ram_bank_top_addr(bank_id) <= start_addr) ||
		    (ram_bank_bottom_addr(bank_id) >= end_addr)) {
			LOG_DBG("Skipping bank: %d.", bank_id);
			continue;
		}
		/* Check if whole RAM bank can be powered down. */
		if ((ram_bank_bottom_addr(bank_id) >= start_addr) &&
		    (ram_bank_top_addr(bank_id) <= end_addr)) {
			LOG_DBG("Powering off bank: %d.", bank_id);
			power_down_ram_sections(bank_id, ram_bank_power_off_mask);
			continue;
		}

		/* Can't power down whole bank - find sections to power down.
		 * Start with top section of the current bank.
		 */
		if (bank_id == 8) {
			section_id = RAM_BANK_8_SECTIONS_NBR - 1;
		} else {
			section_id = RAM_BANK_0_7_SECTIONS_NBR - 1;
		}

		/* Find RAM sections to power down in the current bank - check if given section
		 * is within given address range.
		 */
		do {
			if ((ram_sect_bank_bottom_addr(bank_id, section_id) >= start_addr) &&
			    (ram_sect_bank_top_addr(bank_id, section_id) <= end_addr)) {
				LOG_DBG("Powering off section %d of bank %d.", section_id, bank_id);
				mask_off |= (1UL << section_id);
			} else {
				LOG_DBG("Skipping section %d of bank %d.", section_id, bank_id);
			}
		} while (section_id--);

		/* Power down selected sections. */
		power_down_ram_sections(bank_id, mask_off);
	}
}
#endif /* ifndef UNUSED_RAM_POWER_OFF_UNSUPPORTED */

void power_down_unused_ram(void)
{
#ifndef UNUSED_RAM_POWER_OFF_UNSUPPORTED
	/* Power off unused RAM only - from the end address of RAM used by the application
	 * to the end address of RAM available to application.
	 */
	power_down_ram_from_addr_range(RAM_APP_END_ADDR, RAM_APP_PARTITION_END_ADDR);
#else
	LOG_INF("RAM power off unsupported.");
#endif /* UNUSED_RAM_POWER_OFF_UNSUPPORTED */
}
