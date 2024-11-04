/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <suit_storage_flags_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_flags, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_FLAG_RECOVERY_IDX	       0
#define SUIT_FLAG_CLEARED	       0xFFFFFFFF
#define SUIT_FLAG_RECOVERY_MAGIC       0x2a17644c
#define SUIT_FLAG_FOREGROUND_DFU_MAGIC 0x713cf9c6

suit_plat_err_t suit_storage_flags_internal_init(void)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_flags_internal_clear(uint8_t *area_addr, size_t area_size,
						  suit_storage_flag_t flag)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	uint8_t flags_buf[sizeof(suit_storage_flags_t)];
	suit_storage_flags_t *flags = (suit_storage_flags_t *)&flags_buf;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_flags_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	memcpy(flags_buf, area_addr, sizeof(flags_buf));

	switch (flag) {
	case SUIT_FLAG_RECOVERY:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] != SUIT_FLAG_RECOVERY_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}

		(*flags)[SUIT_FLAG_RECOVERY_IDX] = SUIT_FLAG_CLEARED;
		break;

	case SUIT_FLAG_FOREGROUND_DFU:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] != SUIT_FLAG_FOREGROUND_DFU_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}

		(*flags)[SUIT_FLAG_RECOVERY_IDX] = SUIT_FLAG_CLEARED;
		break;

	default:
		return SUIT_PLAT_ERR_INVAL;
	}

	/* Override regular entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(area_addr), flags_buf,
			  sizeof(flags_buf));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_flags_internal_set(uint8_t *area_addr, size_t area_size,
						suit_storage_flag_t flag)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	uint8_t flags_buf[sizeof(suit_storage_flags_t)];
	suit_storage_flags_t *flags = (suit_storage_flags_t *)&flags_buf;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_flags_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	memcpy(flags_buf, area_addr, sizeof(flags_buf));

	switch (flag) {
	case SUIT_FLAG_RECOVERY:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] == SUIT_FLAG_RECOVERY_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}

		(*flags)[SUIT_FLAG_RECOVERY_IDX] = SUIT_FLAG_RECOVERY_MAGIC;
		break;

	case SUIT_FLAG_FOREGROUND_DFU:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] == SUIT_FLAG_FOREGROUND_DFU_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}

		(*flags)[SUIT_FLAG_RECOVERY_IDX] = SUIT_FLAG_FOREGROUND_DFU_MAGIC;
		break;

	default:
		return SUIT_PLAT_ERR_INVAL;
	}

	/* Override regular entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(area_addr), flags_buf,
			  sizeof(flags_buf));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_flags_internal_check(const uint8_t *area_addr, size_t area_size,
						  suit_storage_flag_t flag)
{
	suit_storage_flags_t *flags = (suit_storage_flags_t *)area_addr;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (EB_SIZE(suit_storage_flags_t) > area_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	switch (flag) {
	case SUIT_FLAG_RECOVERY:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] == SUIT_FLAG_RECOVERY_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}
		return SUIT_PLAT_ERR_NOT_FOUND;

	case SUIT_FLAG_FOREGROUND_DFU:
		if ((*flags)[SUIT_FLAG_RECOVERY_IDX] == SUIT_FLAG_FOREGROUND_DFU_MAGIC) {
			return SUIT_PLAT_SUCCESS;
		}
		return SUIT_PLAT_ERR_NOT_FOUND;

	default:
		break;
	}

	return SUIT_PLAT_ERR_INVAL;
}
