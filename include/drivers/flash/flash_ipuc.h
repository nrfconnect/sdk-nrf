/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SUIT IPUC pseudo flash driver.
 *
 * @details This module implements flash API that allows to modify memory declared as
 *          IPUC through SUIT SSF services.
 */

#ifndef FLASH_IPUC_H__
#define FLASH_IPUC_H__

#include <zephyr/drivers/flash.h>
#include <stdbool.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Release one of the flash IPUC driver context.
 *
 * @param[in]  dev  Reference to the IPUC device to release.
 */
void flash_ipuc_release(struct device *dev);

/**
 * @brief Create a new IPUC device, based on component ID.
 *
 * @param[in]  component_id      Component ID, exactly matching the component, declared as IPUC.
 * @param[in]  encryption_info   Pointer to the binary defining the decryption parameters.
 * @param[in]  compression_info  Pointer to the binary defining the decompression parameters.
 *
 * @return Pointer to the device on success, NULL otherwise.
 */
struct device *flash_component_ipuc_create(struct zcbor_string *component_id,
					   struct zcbor_string *encryption_info,
					   struct zcbor_string *compression_info);

/**
 * @brief Check if it is possible to create a new IPUC device, based on component ID.
 *
 * @param[in]  component_id  Component ID, exactly matching the component, declared as IPUC.
 *
 * @retval true   If it is possible to create the IPUC, based on the list of available IPUCs
 * @retval false  If it is not possible to create such IPUC.
 */
bool flash_component_ipuc_check(struct zcbor_string *component_id);

/**
 * @brief Create a new IPUC device, dedicated for the SUIT DFU cache usage.
 *
 * @param[in]   min_address   The minimal start address of the new SUIT DFU cache area.
 * @param[out]  ipuc_address  The start address of the area managed by the found IPUC device.
 * @param[out]  ipuc_size     The size of the area managed by the found IPUC device.
 *
 * @return Pointer to the device on success, NULL otherwise.
 */
struct device *flash_cache_ipuc_create(uintptr_t min_address, uintptr_t *ipuc_address,
				       size_t *ipuc_size);

/**
 * @brief Check if it is possible to create a new IPUC device, dedicated for the SUIT DFU cache.
 *
 * @param[in]   min_address   The minimal start address of the new SUIT DFU cache area.
 * @param[out]  ipuc_address  The start address of the area managed by the found IPUC device.
 * @param[out]  ipuc_size     The size of the area managed by the found IPUC device.
 *
 * @retval true   If it is possible to create the IPUC, based on the list of available IPUCs
 * @retval false  If it is not possible to create such IPUC.
 */
bool flash_cache_ipuc_check(uintptr_t min_address, uintptr_t *ipuc_address, size_t *ipuc_size);

/**
 * @brief Find an existing IPUC driver, capable of modifying the specified address range.
 *
 * @param[in]   address       The start of an address range to be modified through IPUC.
 * @param[in]   size          The size of the address range.
 * @param[out]  ipuc_address  The start address of the area managed by the found IPUC device.
 * @param[out]  ipuc_size     The size of the area managed by the found IPUC device.
 *
 * @return Pointer to the device on success, NULL otherwise.
 */
struct device *flash_ipuc_find(uintptr_t address, size_t size, uintptr_t *ipuc_address,
			       size_t *ipuc_size);

/**
 * @brief Check if IPUC device was not initialized over SSF.
 *
 * @param[in]  dev  Reference to the IPUC device to release.
 */
bool flash_ipuc_setup_pending(const struct device *dev);

/**
 * @brief Create a new IPUC device, based on image ID.
 *
 * @param[in]   id                Image ID, defined inside DTS.
 * @param[in]   encryption_info   Pointer to the binary defining the decryption parameters.
 * @param[in]   compression_info  Pointer to the binary defining the decompression parameters.
 * @param[out]  ipuc_address      The start address of the area managed by the found IPUC device.
 * @param[out]  ipuc_size         The size of the area managed by the found IPUC device.
 *
 * @return Pointer to the device on success, NULL otherwise.
 */
struct device *flash_image_ipuc_create(size_t id, struct zcbor_string *encryption_info,
				       struct zcbor_string *compression_info,
				       uintptr_t *ipuc_address, size_t *ipuc_size);

/**
 * @brief Release one of the flash image IPUC driver context.
 *
 * @param[in]  id  Image ID, defined inside DTS.
 */
void flash_image_ipuc_release(size_t id);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_IPUC_H__ */
