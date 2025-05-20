/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_manifest_variables.h>
#include <suit_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_mfst_vars, CONFIG_SUIT_LOG_LEVEL);

static uint32_t plat_volatile_values[CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT];
static uint32_t mfst_volatile_values[CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT];

suit_plat_err_t suit_mfst_var_get_access_mask(uint32_t id, uint32_t *access_mask)
{
	if (access_mask == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID &&
	    id < CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID +
			    CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT) {

		*access_mask = CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_ACCESS_MASK;
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT) {

		*access_mask = CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_ACCESS_MASK;
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT) {

		*access_mask = CONFIG_SUIT_MANIFEST_VARIABLES_NVM_ACCESS_MASK;
		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Variable does not exist, id: %d", id);
	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_mfst_var_set(uint32_t id, uint32_t val)
{
	if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID &&
	    id < CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID +
			    CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT) {

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID;

		plat_volatile_values[idx] = val;
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT) {

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID;

		mfst_volatile_values[idx] = val;
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT) {

		if (val > 0xFF) {
			/* size of NVM based variable is limited to 8 bits
			 */
			return SUIT_PLAT_ERR_SIZE;
		}

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID;

		if (suit_storage_var_set(idx, val) == SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Cannot set persistent variable, idx: %d", idx);

		return SUIT_PLAT_ERR_IO;
	}

	LOG_ERR("Variable does not exist, id: %d", id);
	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_mfst_var_get(uint32_t id, uint32_t *val)
{
	if (val == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID &&
	    id < CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID +
			    CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT) {

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID;

		*val = plat_volatile_values[idx];
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT) {

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID;

		*val = mfst_volatile_values[idx];
		return SUIT_PLAT_SUCCESS;

	} else if (id >= CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID &&
		   id < CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID +
				   CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT) {

		uint32_t idx = id - CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID;
		uint8_t nvm_val = 0;

		if (suit_storage_var_get(idx, &nvm_val) == SUIT_PLAT_SUCCESS) {
			*val = nvm_val;
			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Cannot get persistent variable, idx: %d", idx);
		return SUIT_PLAT_ERR_IO;
	}

	LOG_ERR("Variable does not exist, id: %d", id);
	return SUIT_PLAT_ERR_NOT_FOUND;
}
