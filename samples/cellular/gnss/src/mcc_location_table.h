/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MCC_LOCATION_TABLE_H_
#define MCC_LOCATION_TABLE_H_

struct __attribute__ ((__packed__)) mcc_table {
	uint8_t confidence;    /* percentage, 0-100 */
	uint8_t unc_semiminor; /* scaled, see GNSS interface for details */
	uint8_t unc_semimajor; /* scaled, see GNSS interface for details */
	uint8_t orientation;   /* orientation angle between the major axis and north */
	float lat;
	float lon;
	uint16_t mcc;
};

/**
 * @brief Finds location information for a given Mobile Country Code (MCC).
 *
 * @param mcc[in] MCC to look for.
 *
 * @return Location information for the matching MCC, NULL if no entry was found.
 */
const struct mcc_table *mcc_lookup(uint16_t mcc);

/**
 * @brief Converts a latitude in degrees to the integer representation used by GNSS.
 *
 * @param lat[in] Latitude in degrees.
 *
 * @return Latitude in scaled integer representation.
 */
int32_t lat_convert(float lat);

/**
 * @brief Converts a longitude in degrees to the integer representation used by GNSS.
 *
 * @param lon[in] Longitude in degrees.
 *
 * @return Longitude in scaled integer representation.
 */
int32_t lon_convert(float lon);

#endif /* MCC_LOCATION_TABLE_H_ */
