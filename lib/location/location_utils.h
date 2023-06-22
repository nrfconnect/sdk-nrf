/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

#include <modem/location.h>

/** Modem parameters. */
struct location_utils_modem_params_info {
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
 * @brief Check if LTE networking is available.
 *
 * Checks for an active default PDN bearer, but does not check cell registration status,
 * so may give false positive, but not false negative.
 *
 * @retval true      LTE networking is available.
 * @retval false     LTE networking is not available.
 */
bool location_utils_is_lte_available(void);

/**
 * @brief Read modem parameters.
 *
 * @param[out] modem_params Context where parameters are filled.
 *
 * @retval true      If modem parameters were received successfully.
 * @retval false     If modem parameters were not received successfully.
 */
int location_utils_modem_params_read(struct location_utils_modem_params_info *modem_params);

/**
 * @brief Generate JWT buffer to be used for nRF Cloud REST API use.
 *
 * @details Return JWT buffer memory shouldn't be freed by the caller and it's valid until
 * the next call to this function.
 *
 * @return JWT buffer if generation was successful. NULL if generation failed.
 */
const char *location_utils_nrf_cloud_jwt_generate(void);

/**
 * @brief Get current system time and fill that into a given location_datetime.
 *
 */
void location_utils_systime_to_location_datetime(struct location_datetime *datetime);

#endif /* LOCATION_UTILS_H */
