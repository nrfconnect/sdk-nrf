/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MEMORY_LAYOUT_H__
#define SUIT_MEMORY_LAYOUT_H__

#include <zephyr/sys/util_macro.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NVM address consisting of flash device handle and offset. */
struct nvm_address {
	const struct device *fdev;
	uintptr_t offset;
};

/**
 * @brief Convert global address to NVM address.
 *
 * @param address Global address pointer.
 * @param result Non-volatile memory address.
 *
 * @return True if global address pointer belongs to NVM region, false otherwise.
 */
bool suit_memory_global_address_to_nvm_address(uintptr_t address, struct nvm_address *result);

/**
 * @brief Convert NVM address to global address pointer.
 *
 * @param address Non-volatile memory address.
 * @param result Global address pointer.
 *
 * @return True if conversion was successful, false otherwise.
 */
bool suit_memory_nvm_address_to_global_address(struct nvm_address *address, uintptr_t *result);

/**
 * @brief Check if global address pointer is in NVM region.
 *
 * @param address Global address pointer.
 *
 * @return True if global address pointer belongs to NVM region, false otherwise.
 */
bool suit_memory_global_address_is_in_nvm(uintptr_t address);

/**
 * @brief Check if global address range is in the same NVM region.
 *
 * @param address Global address pointer.
 * @param size Region size.
 *
 * @return True if global address pointer belongs to NVM region, false otherwise.
 */
bool suit_memory_global_address_range_is_in_nvm(uintptr_t address, size_t size);

/**
 * @brief Check if global address pointer is in RAM region.
 *
 * @param address Global address pointer.
 *
 * @return True if global address pointer belongs to RAM region, false otherwise.
 */
bool suit_memory_global_address_is_in_ram(uintptr_t address);

/**
 * @brief Convert global address to RAM address.
 *
 * This function is relevant only on simulated platforms. Otherwise its an identity.
 *
 * @param address Global address pointer.
 *
 * @return RAM address.
 */
uintptr_t suit_memory_global_address_to_ram_address(uintptr_t address);

/**
 * @brief Check if global address range is in the same RAM region.
 *
 * @param address Global address pointer.
 * @param size Region size.
 *
 * @return True if global address pointer belongs to RAM region, false otherwise.
 */
bool suit_memory_global_address_range_is_in_ram(uintptr_t address, size_t size);

/**
 * @brief Check if global address is in external memory region.
 *
 * @param address Global address pointer.
 *
 * @return True if address is in external memory region, false otherwise.
 */
bool suit_memory_global_address_is_in_external_memory(uintptr_t address);

/**
 * @brief Check if global address range is in external memory region.
 *
 * @param address Global address pointer.
 *
 * @return True if address range is in external memory region, false otherwise.
 */
bool suit_memory_global_address_range_is_in_external_memory(uintptr_t address, size_t size);

/**
 * @brief Check if address points to a region readable through regular bus transactions.
 *
 * Directly readable regions can be read directly by the CPU with regular bus transactions.
 * Such regions are the RAM and NVM memory integrated into the SoC.
 *
 * @param address Global address pointer.
 *
 * @return True if address points to directly readable region, false otherwise.
 */
bool suit_memory_global_address_is_directly_readable(uintptr_t address);

/**
 * @brief Convert global address to an offset within an external storage.
 *
 * @param address Global address pointer.
 *
 * @return Offset within external storage.
 */
bool suit_memory_global_address_to_external_memory_offset(uintptr_t address, uintptr_t *offset);

/**
 * @brief Get external memory controller flash_api instance.
 *
 * @return External memory controller flash_api instance.
 */
const struct device *suit_memory_external_memory_device_get(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MEMORY_LAYOUT_H__ */
