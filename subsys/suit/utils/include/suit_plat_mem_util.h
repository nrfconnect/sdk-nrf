/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_MEM_UTIL_H__
#define SUIT_PLAT_MEM_UTIL_H__

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_AREA_ERASE_BLOCK_SIZE(label)                                                         \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)
#define FLASH_AREA_WRITE_BLOCK_SIZE(label)                                                         \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#ifdef CONFIG_SDFW_MRAM_ERASE_VALUE
#define EMPTY_STORAGE_VALUE                                                                        \
	((((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 24) |                                      \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 16) |                                      \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 8) |                                       \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE))))
#else
#define EMPTY_STORAGE_VALUE 0xFFFFFFFF
#endif

#if IS_ENABLED(CONFIG_FLASH)
#if (DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash_controller)))
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller))
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash))
#endif
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV NULL
#endif

/** @brief Convert address from SUIT component ID to memory pointer.
 *
 * @note This function is used to hide the fact that on POSIX platform the flash
 *       memory is mapped to a different address than specified by the component ID
 *
 * @param[in]  address  Address from the SUIT component ID
 *
 * @returns Pointer to the memory.
 */
uint8_t *suit_plat_mem_ptr_get(uintptr_t address);

/** @brief Convert pointer to the memory to address compatible with SUIT component ID.
 *
 * @note This function is used to hide the fact that on POSIX platform the flash
 *       memory is mapped to a different address than specified by the component ID
 *
 * @param[in]  ptr  Pointer to the memory
 *
 * @returns Address compatible with the SUIT component ID.
 */
uintptr_t suit_plat_mem_address_get(uint8_t *ptr);

/** @brief Convert pointer to the memory to the offset in the area, managed by the default flash
 *         controller.
 *
 * @note The flash controller API requires to pass the offset inside the memory, not just
 *       the address.
 *
 * @param[in]  ptr  Pointer to the memory
 *
 * @returns Offset in the NVM area, managed by the flash controller.
 */
uintptr_t suit_plat_mem_nvm_offset_get(uint8_t *ptr);

/** @brief Convert the offset in the area, managed by the default flash controller to pointer to the
 *         memory.
 *
 * @note The flash controller API requires to pass the offset inside the memory, not just
 *       the address.
 *
 * @param[in]  offset  Offset in the NVM area, managed by the flash controller
 *
 * @returns Pointer to the memory.
 */
uint8_t *suit_plat_mem_nvm_ptr_get(uintptr_t offset);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_MEM_UTIL_H__ */
