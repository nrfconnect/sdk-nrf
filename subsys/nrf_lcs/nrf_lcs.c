/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_lcs/nrf_lcs.h>
#include <nrf_lcs/nrf_lcs_mem.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_lcs, CONFIG_NRF_LCS_LOG_LEVEL);

/**
 * @brief Check if the LCS transition from @p current to @p new is legal.
 *
 * @param[in] current  Current LCS.
 * @param[in] new      New LCS.
 *
 * @retval true   if the transition is legal.
 * @retval false  if the transition is not legal.
 */
static bool transition_check(enum nrf_lcs current, enum nrf_lcs new)
{
	switch (current) {
	case NRF_LCS_PSA_PROT_DEBUG:
		/* fall-through */
	case NRF_LCS_PSA_NON_PROT_DEBUG:
		/* fall-through */
	case NRF_LCS_UNKNOWN:
		/* It is not possible to transition out of unknown state. */
		/* fall-through */
	case NRF_LCS_DECOMMISSIONED:
		/* It is not possible to transition out of decommissioned state. */
		/* fall-through */
	case NRF_LCS_MAX:
		/* It is not possible to transition out of max state. */
		return false;

	case NRF_LCS_ASSEMBLY_AND_TEST:
		return (new == NRF_LCS_PSA_ROT_PROVISIONING || new == NRF_LCS_SECURED ||
			new == NRF_LCS_DECOMMISSIONED);

	case NRF_LCS_PSA_ROT_PROVISIONING:
		return (new == NRF_LCS_SECURED || new == NRF_LCS_DECOMMISSIONED);

	case NRF_LCS_SECURED:
		return new == NRF_LCS_DECOMMISSIONED;

	default:
		/* If the current state is not recognized, do not allow any transition. */
		return false;
	}

	return false;
}

enum nrf_lcs nrf_lcs_get(void)
{
	/* Initialize all values with constants, different from the erased value. */
	uint32_t decommissioned_value = 0;
	uint32_t secured_value = 0;
	uint32_t provisioning_value = 0;
	int err = -1;

	/* Always query all of the LCS memory slots. */
	err = nrf_lcs_mem_decommissioned_read(&decommissioned_value);
	if (err != 0) {
		LOG_ERR("Error reading DECOMMISSIONED LCS value: %d", err);
		return NRF_LCS_UNKNOWN;
	}

	err = nrf_lcs_mem_secured_read(&secured_value);
	if (err != 0) {
		LOG_ERR("Error reading SECURED LCS value: %d", err);
		return NRF_LCS_UNKNOWN;
	}

	err = nrf_lcs_mem_provisioning_read(&provisioning_value);
	if (err != 0) {
		LOG_ERR("Error reading PSA_ROT_PROVISIONING LCS value: %d", err);
		return NRF_LCS_UNKNOWN;
	}

	/*
	 * Since the LCS_ASSEMBLY_AND_TEST allows to boot an unsigned firmware,
	 * make sure that all of the memory slots include the erased (not entered)
	 * value before returning this LCS.
	 */
	if ((provisioning_value == NRF_LCS_MEM_VALUE_NOT_ENTERED) &&
	    (secured_value == NRF_LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommissioned_value == NRF_LCS_MEM_VALUE_NOT_ENTERED)) {
		return NRF_LCS_ASSEMBLY_AND_TEST;
	}

	/*
	 * For all LCS, verify that the memory slot corresponding to further
	 * LCSes is not touched.
	 */
	if ((provisioning_value != NRF_LCS_MEM_VALUE_NOT_ENTERED) &&
	    (secured_value == NRF_LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommissioned_value == NRF_LCS_MEM_VALUE_NOT_ENTERED)) {
		return NRF_LCS_PSA_ROT_PROVISIONING;
	}

	/* The order of checks is not optimized, so a value of the further LCS
	 * cannot be deduced based on the previous checks, and it is
	 * enough to check each value only once.
	 */
	if ((secured_value != NRF_LCS_MEM_VALUE_NOT_ENTERED) &&
	    (decommissioned_value == NRF_LCS_MEM_VALUE_NOT_ENTERED)) {
		return NRF_LCS_SECURED;
	}

	/* It is enough to touch the DECOMMISSIONED slot to discard a device. */
	if (decommissioned_value != NRF_LCS_MEM_VALUE_NOT_ENTERED) {
		return NRF_LCS_DECOMMISSIONED;
	}

	/* If none of the above conditions are met, the device has been tampered with. */

	return NRF_LCS_UNKNOWN;
}

void nrf_lcs_set(enum nrf_lcs new)
{
	int err = 0;
	enum nrf_lcs stored = nrf_lcs_get();

	if (!transition_check(stored, new)) {
		LOG_ERR("Invalid LCS transition: 0x%x -> 0x%x", stored, new);
		return;
	}

	switch (new) {
	case NRF_LCS_PSA_ROT_PROVISIONING:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = nrf_lcs_mem_provisioning_write(NRF_LCS_PSA_ROT_PROVISIONING);
		if (err != 0) {
			LOG_ERR("Error setting PSA RoT provisioning LCS: %d", err);
		}
		break;

	case NRF_LCS_SECURED:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = nrf_lcs_mem_secured_write(NRF_LCS_SECURED);
		if (err != 0) {
			LOG_ERR("Error setting secured LCS: %d", err);
		}
		break;

	case NRF_LCS_DECOMMISSIONED:
		LOG_INF("Performing LCS transition 0x%x -> 0x%x", stored, new);
		err = nrf_lcs_mem_decommissioned_write(NRF_LCS_DECOMMISSIONED);
		if (err != 0) {
			LOG_ERR("Error setting decommissioned LCS: %d", err);
		}
		break;

	default:
		LOG_WRN("Invalid new LCS ignored: 0x%x", new);
		break;
	}
}
