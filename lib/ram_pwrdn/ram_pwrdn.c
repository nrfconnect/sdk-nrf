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

/* Start address of RAM. */
#define RAM_START_ADDR              0x20000000UL
/* Size of RAM section for banks 0-7. */
#define RAM_BANK_0_7_SECTION_SIZE   0x1000
/* Number of sections in banks 0-7. */
#define RAM_BANK_0_7_SECTIONS_NBR   2
/* Size of RAM section for bank 8. */
#define RAM_BANK_8_SECTION_SIZE     0x8000
/* Number of sections in bank 8. */
#define RAM_BANK_8_SECTIONS_NBR     6
/* Number of RAM banks available at the SoC. */
#define RAM_BANKS_NBR               8
/* End of RAM used by the application. */
#define RAM_END_ADDR                ((uint32_t)&_image_ram_end)
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
#endif /* ifndef UNUSED_RAM_POWER_OFF_UNSUPPORTED */

void power_down_unused_ram(void)
{
#ifndef UNUSED_RAM_POWER_OFF_UNSUPPORTED
	/* ID of top RAM bank. Depends of amount of RAM available at SoC. */
	uint8_t     bank_id                 = RAM_BANKS_NBR;
	uint8_t     section_id              = 5;
	uint32_t    section_size            = 0;
	/* Mask to power down whole RAM bank. */
	uint32_t    ram_bank_power_off_mask = 0x0000FFFF;
	/* Mask to select sections of RAM bank to power off. */
	uint32_t    mask_off;

	/* Power off banks with unused RAM only. */
	while (ram_bank_bottom_addr(bank_id) >= RAM_END_ADDR) {
		LOG_DBG("Powering off bank: %d.", bank_id);
		nrf_power_rampower_mask_off(NRF_POWER,
					    bank_id,
					    ram_bank_power_off_mask);
		bank_id--;
	}

	/* Set ID of top section and section size for given bank. */
	if (bank_id == 8) {
		section_id = RAM_BANK_8_SECTIONS_NBR - 1;
		section_size = RAM_BANK_8_SECTION_SIZE;
	} else {
		section_id = RAM_BANK_0_7_SECTIONS_NBR - 1;
		section_size = RAM_BANK_0_7_SECTION_SIZE;
	}

	/* Power off remaining sections of unused RAM. */
	while (ram_sect_bank_bottom_addr(bank_id, section_id) >= RAM_END_ADDR) {
		LOG_DBG("Powering off section %d of bank %d.",
			section_id,
			bank_id);

		mask_off = ((1 << section_id) << NRF_POWER_RAMPOWER_S0POWER);

		nrf_power_rampower_mask_off(NRF_POWER, bank_id, mask_off);
		section_id--;
	}
#else
	LOG_INF("RAM power off unsupported.");
#endif /* UNUSED_RAM_POWER_OFF_UNSUPPORTED */
}
