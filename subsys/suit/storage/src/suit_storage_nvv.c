/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_nvv.h>
#include <suit_storage_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_nvv, CONFIG_SUIT_LOG_LEVEL);

typedef struct {
	uint8_t *addr;
	size_t size;
} suit_storage_nvv_entry_t;

suit_plat_err_t suit_storage_nvv_init(void)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_nvv_erase(uint8_t *area_addr, size_t area_size)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	uint8_t nvv_buf[EB_SIZE(suit_storage_nvv_t)];

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_nvv_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	/* Override regular entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	memset(nvv_buf, 0xFF, sizeof(nvv_buf));

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(area_addr), nvv_buf, sizeof(nvv_buf));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_nvv_get(const uint8_t *area_addr, size_t area_size, size_t index,
				     uint32_t *value)
{
	suit_storage_nvv_t *vars = (suit_storage_nvv_t *)area_addr;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_nvv_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (value == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ARRAY_SIZE(*vars) <= index) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	*value = (*vars)[index];

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_nvv_set(uint8_t *area_addr, size_t area_size, size_t index,
				     uint32_t value)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	uint8_t nvv_buf[EB_SIZE(suit_storage_nvv_t)];
	suit_storage_nvv_t *vars = (suit_storage_nvv_t *)&nvv_buf;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_nvv_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if (ARRAY_SIZE(*vars) <= index) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	memcpy(nvv_buf, area_addr, sizeof(nvv_buf));
	(*vars)[index] = value;

	/* Override regular entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(area_addr), nvv_buf, sizeof(nvv_buf));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}
