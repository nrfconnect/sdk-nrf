/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_mem_util.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */

uint8_t *suit_plat_mem_ptr_get(uintptr_t address)
{
#ifdef CONFIG_FLASH_SIMULATOR
	static uint8_t *f_base_address;

	if (f_base_address == NULL) {
		size_t f_size = 0;
		static const struct device *fdev =
			DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

		f_base_address = flash_simulator_get_memory(fdev, &f_size);
	}

	/* Address is validated against range defined in dts for flash0 and for that range
	 * it is translated to the emulated flash address range.
	 * Address in the range defined for sram0 is not translated.
	 * Address that do not fit in any of the mentioned ranges is treated as invalid.
	 */
#if DT_NODE_EXISTS(DT_NODELABEL(flash0))
	if ((address >= DT_REG_ADDR(DT_NODELABEL(flash0))) &&
	    (address < (DT_REG_ADDR(DT_NODELABEL(flash0)) + DT_REG_SIZE(DT_NODELABEL(flash0))))) {
		return (uint8_t *)(address + (uintptr_t)f_base_address);
	}
#endif /* DT_NODE_EXISTS(DT_NODELABEL(flash0)) */

#if DT_NODE_EXISTS(DT_NODELABEL(sram0))
	if ((address >= (DT_REG_ADDR(DT_NODELABEL(sram0)))) &&
	    (address < (DT_REG_ADDR(DT_NODELABEL(sram0)) + DT_REG_SIZE(DT_NODELABEL(sram0))))) {
		return (uint8_t *)address;
	}
#endif /* DT_NODE_EXISTS(DT_NODELABEL(sram0)) */

	return NULL;
#else  /* CONFIG_FLASH_SIMULATOR */
	return (uint8_t *)address;
#endif /* CONFIG_FLASH_SIMULATOR */
}

uintptr_t suit_plat_mem_address_get(uint8_t *ptr)
{
#ifdef CONFIG_FLASH_SIMULATOR
	static uint8_t *f_base_address;

	if (f_base_address == NULL) {
		size_t f_size;
		static const struct device *fdev =
			DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

		f_base_address = flash_simulator_get_memory(fdev, &f_size);
	}

	return (uintptr_t)(ptr - f_base_address);
#else  /* CONFIG_FLASH_SIMULATOR */
	return (uintptr_t)ptr;
#endif /* CONFIG_FLASH_SIMULATOR */
}

uintptr_t suit_plat_mem_nvm_offset_get(uint8_t *ptr)
{
	uintptr_t address = suit_plat_mem_address_get(ptr);

#if (DT_NODE_EXISTS(DT_NODELABEL(mram1x)))
	address = (((address)&0xEFFFFFFFUL) - (DT_REG_ADDR(DT_NODELABEL(mram1x)) & 0xEFFFFFFFUL));
#elif (DT_NODE_EXISTS(DT_NODELABEL(mram10)))
	address = (((address)&0xEFFFFFFFUL) - (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL));
#endif

	return address;
}

uint8_t *suit_plat_mem_nvm_ptr_get(uintptr_t offset)
{
	uintptr_t address;

#if (DT_NODE_EXISTS(DT_NODELABEL(mram1x)))
	address = (((offset)&0xEFFFFFFFUL) + (DT_REG_ADDR(DT_NODELABEL(mram1x)) & 0xEFFFFFFFUL));
#elif (DT_NODE_EXISTS(DT_NODELABEL(mram10)))
	address = (((offset)&0xEFFFFFFFUL) + (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL));
#else
	address = offset;
#endif

	return suit_plat_mem_ptr_get(address);
}
