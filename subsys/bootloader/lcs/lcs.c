/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bootloader/lcs.h>
#include <bootloader/lcs_mem.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lcs, CONFIG_LCS_LOG_LEVEL);

/**
 * @brief Check if the LCS transition from @p current to @p new is legal.
 *
 * @param[in] current  Current LCS.
 * @param[in] new      New LCS.
 *
 * @retval true   if the transition is legal.
 * @retval false  if the transition is not legal.
 */
static bool transition_check(enum lcs current, enum lcs new)
{
	switch (current) {
	case LCS_INVALID:
		/* It is not possible to transition out of invalid state. */
		/* fall-through */
	case LCS_UNKNOWN:
		/* It is not possible to transition out of unknown state. */
		/* fall-through */
	case LCS_DECOMMISSIONED:
		/* It is not possible to transition out of decommissioned state. */
		/* fall-through */
	case LCS_MAX:
		/* It is not possible to transition out of max state. */
		return false;

	case LCS_ASSEMBLY_AND_TEST:
		return (new == LCS_PSA_ROT_PROVISIONING || new == LCS_SECURED ||
			new == LCS_DECOMMISSIONED);

	case LCS_PSA_ROT_PROVISIONING:
		return (new == LCS_SECURED || new == LCS_DECOMMISSIONED);

	case LCS_SECURED:
		return new == LCS_DECOMMISSIONED;
	}

	return false;
}

enum lcs lcs_get(void)
{
	/* Initialize all values with constants, different from the erased value. */
	uint16_t decommisioned_value = 0;
	uint16_t secured_value = 0;
	uint16_t provisioning_value = 0;
	int err = -1;

	/* Always query all of the LCS memory slots. */
	err = lcs_mem_decommissioned_read(&decommisioned_value);
	if (err != 0) {
		LOG_ERR("Error reading DECOMMISSIONED LCS value: %d", err);
		return LCS_UNKNOWN;
	}

	err = lcs_mem_secured_read(&secured_value);
	if (err != 0) {
		LOG_ERR("Error reading SECURED LCS value: %d", err);
		return LCS_UNKNOWN;
	}

	err = lcs_mem_provisioning_read(&provisioning_value);
	if (err != 0) {
		LOG_ERR("Error reading PSA_ROT_PROVISIONING LCS value: %d", err);
		return LCS_UNKNOWN;
	}

	/*
	 * Since the LCS_ASSEMBLY_AND_TEST allows to boot an unsigned firmware,
	 * make sure that all of the memory slots include the erased (not entered)
	 * value before returning this LCS.
	 */
	if ((provisioning_value == LCS_MEM_VALUE_NOT_ENTERED) &&
	    (secured_value == LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommisioned_value == LCS_MEM_VALUE_NOT_ENTERED)) {
		return LCS_ASSEMBLY_AND_TEST;
	}

	/*
	 * For all LCS, verify that the memory slot corresponding to further
	 * LCS is not touched.
	 */
	if ((provisioning_value != LCS_MEM_VALUE_NOT_ENTERED) &&
	    (secured_value == LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommisioned_value == LCS_MEM_VALUE_NOT_ENTERED)) {
		return LCS_PSA_ROT_PROVISIONING;
	}

	/* The order of checks is not optimized, so the optimization cannot
	 * assume that a value of the further LCS was checked before and it is
	 * enough to check each field only once.
	 */
	if ((secured_value != LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommisioned_value == LCS_MEM_VALUE_NOT_ENTERED)) {
		return LCS_SECURED;
	}

	/* It is enough to touch the DECOMMISSIONED slot to discard a device. */
	if (decommisioned_value != LCS_MEM_VALUE_NOT_ENTERED) {
		return LCS_DECOMMISSIONED;
	}

	/* If none of the above conditions are met, the device has been tampered with. */

	return LCS_UNKNOWN;
}

void lcs_set(enum lcs new)
{
	int err = 0;
	enum lcs stored = lcs_get();

	if (!transition_check(stored, new)) {
		LOG_ERR("Invalid LCS transition: 0x%x -> 0x%x", stored, new);
		return;
	}

	switch (new) {
	case LCS_PSA_ROT_PROVISIONING:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = lcs_mem_provisioning_write((uint16_t)LCS_PSA_ROT_PROVISIONING);
		if (err != 0) {
			LOG_ERR("Error setting new LCS: %d", err);
		}
		break;

	case LCS_SECURED:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = lcs_mem_secured_write((uint16_t)LCS_SECURED);
		if (err != 0) {
			LOG_ERR("Error setting new LCS: %d", err);
		}
		break;

	case LCS_DECOMMISSIONED:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = lcs_mem_decommissioned_write((uint16_t)LCS_DECOMMISSIONED);
		if (err != 0) {
			LOG_ERR("Error setting new LCS: %d", err);
		}
		break;

	default:
		LOG_WRN("Invalid new LCS ignored: 0x%x", new);
		break;
	}
}
