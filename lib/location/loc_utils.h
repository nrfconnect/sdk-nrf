/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOC_UTILS_H
#define LOC_UTILS_H

struct loc_utils_modem_params_info {
	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID */
	uint32_t cell_id;

	/** Tracking area code. */
	uint32_t tac;

	/** Physical cell ID. */
	uint16_t phys_cell_id;
};

/**
 * @brief Function to check if default PDN context is active.
 *
 * @retval true      If default PDN context is active.
 * @retval true      If default PDN context is not active.
 */
bool loc_utils_is_default_pdn_active(void);

/**
 * @brief Function to read modem parameters.
 *
 *  @param[out] modem_params Context where parameters are filled.
 *
 * @retval true      If default PDN context is active.
 * @retval true      If default PDN context is not active.
 */
int loc_utils_modem_params_read(struct loc_utils_modem_params_info *modem_params);

#endif /* LOC_UTILS_H */
