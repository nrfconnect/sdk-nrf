/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#define UPDATE_MAGIC_VALUE_AVAILABLE_RAW  0x5555AAAA
#define UPDATE_MAGIC_VALUE_AVAILABLE_CBOR 0x55AA55AA
#define UPDATE_MAGIC_VALUE_EMPTY	  0xFFFFFFFF

LOG_MODULE_REGISTER(suit_storage_update, CONFIG_SUIT_LOG_LEVEL);

/** @brief Update the candidate info structure.
 *
 * @param[in]   info  Update candidate info to write.
 * @param[in]   addr  Address of erase-block aligned memory containing update candidate info.
 * @param[in]   size  Size of erase-block aligned memory, containing update candidate info.
 *
 * @retval SUIT_PLAT_SUCCESS           if update candidate successfully updated.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
static suit_plat_err_t save_update_info(const struct update_candidate_info *info, uint8_t *addr,
					size_t size)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(addr), size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(addr), info, sizeof(*info));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_init(void)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_get(const uint8_t *addr, size_t size,
					const suit_plat_mreg_t **regions, size_t *len)
{
	struct update_candidate_info *info = (struct update_candidate_info *)addr;

	if ((addr == NULL) || (regions == NULL) || (len == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (size < sizeof(struct update_candidate_info)) {
		LOG_ERR("Incorrect update candidate area size (%d != %d)", size,
			sizeof(struct update_candidate_info));
		return SUIT_PLAT_ERR_SIZE;
	}

	if ((info->update_magic_value != UPDATE_MAGIC_VALUE_AVAILABLE_CBOR) ||
	    (info->update_regions_len < 1) || (info->update_regions[0].mem == NULL) ||
	    (info->update_regions[0].size == 0)) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	*len = info->update_regions_len;
	*regions = (const suit_plat_mreg_t *)&(info->update_regions);

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_set(uint8_t *addr, size_t size, const suit_plat_mreg_t *regions,
					size_t len)
{
	struct update_candidate_info info;

	memset(&info, 0, sizeof(info));

	if ((addr == NULL) || ((len != 0) && (regions == NULL))) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (size < sizeof(struct update_candidate_info)) {
		LOG_ERR("Incorrect update candidate area size (%d != %d)", size,
			sizeof(struct update_candidate_info));
		return SUIT_PLAT_ERR_SIZE;
	}

	if ((regions != NULL) && ((regions[0].mem == NULL) || (regions[0].size == 0))) {
		LOG_ERR("Invalid update candidate regions provided (%p)", (void *)regions);
		return SUIT_PLAT_ERR_INVAL;
	}

	if (len > CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS) {
		LOG_ERR("Too many update regions (%d > %d)", len,
			CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS);
		return SUIT_PLAT_ERR_SIZE;
	}

	info.update_magic_value = UPDATE_MAGIC_VALUE_AVAILABLE_CBOR;
	info.update_regions_len = len;
	if (len != 0) {
		memcpy(&info.update_regions, regions, sizeof(regions[0]) * len);
	}

	return save_update_info(&info, addr, size);
}
