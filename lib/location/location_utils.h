/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

#include <modem/location.h>

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
