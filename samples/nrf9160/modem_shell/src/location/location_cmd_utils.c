/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zephyr/shell/shell.h>
#include <date_time.h>
#include <cJSON.h>
#include <math.h>
#include <getopt.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <modem/location.h>

#include "mosh_print.h"
#include "location_cmd_utils.h"

/******************************************************************************/

/**
 * @brief Compute checksum for NMEA string.
 *
 * @param datastring - The NMEA string to compute the checksum for.
 * @return uint8_t - The resultant checksum.
 */
static uint8_t location_cmd_utils_nmea_checksum(const char *const datastring)
{
	uint8_t checksum = 0;
	int len = strlen(datastring);

	for (int i = 0; i < len; i++) {
		checksum ^= datastring[i];
	}
	return checksum;
}

static int location_cmd_utils_generate_nmea(const struct location_data *location_data,
					    char *nmea_buf, int nmea_buf_len)
{
	__ASSERT_NO_MSG(location_data != NULL);
	__ASSERT_NO_MSG(nmea_buf != NULL);

	/* Generate a NMEA message from the provided location lib data */
	int latdeg = floor(fabs(location_data->latitude));
	int londeg = floor(fabs(location_data->longitude));
	double latmin = (fabs(location_data->latitude) - latdeg) * 60;
	double lonmin = (fabs(location_data->longitude) - londeg) * 60;
	int payloadlength;

	snprintf(nmea_buf, nmea_buf_len, "$GPGGA,,%02d%08.5f,%c,%02d%08.5f,%c,,,,,,,,,", latdeg,
		 latmin, location_data->latitude > 0 ? 'N' : 'S', londeg, lonmin,
		 location_data->longitude > 0 ? 'E' : 'W');
	payloadlength = strlen(nmea_buf);
	snprintf(nmea_buf + payloadlength, nmea_buf_len - payloadlength, "*%02X\n",
		 location_cmd_utils_nmea_checksum(nmea_buf + 1));
	return payloadlength;
}

/**************************************************************************************************/

int location_cmd_utils_gnss_loc_to_cloud_payload_json_encode(
	enum nrf_cloud_gnss_type format, const struct location_data *location_data,
	char **json_str_out)
{
	__ASSERT_NO_MSG(location_data != NULL);
	__ASSERT_NO_MSG(json_str_out != NULL);

	struct nrf_cloud_gnss_data gnss_data = {
		.type = format,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.pvt = {
			.lon		= location_data->longitude,
			.lat		= location_data->latitude,
			.accuracy	= location_data->accuracy,
			.has_alt	= 0,
			.has_speed	= 0,
			.has_heading	= 0
		}
	};
	char nmea_buf[64];
	int err = 0;
	cJSON *gnss_data_obj = cJSON_CreateObject();

	if (gnss_data_obj == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

#if defined(CONFIG_DATE_TIME)
	/* Add the timestamp */
	err = date_time_now(&gnss_data.ts_ms);
	if (err) {
		mosh_error("Failed to create timestamp");
		goto cleanup;
	}
#endif
	if (format == NRF_CLOUD_GNSS_TYPE_NMEA) {
		(void)location_cmd_utils_generate_nmea(location_data, nmea_buf, sizeof(nmea_buf));
		gnss_data.nmea.sentence = nmea_buf;
	}

	/* Encode the GNSS location data */
	err = nrf_cloud_gnss_msg_json_encode(&gnss_data, gnss_data_obj);
	if (err) {
		mosh_error("Failed to encode GNSS data to json");
		goto cleanup;
	}

	*json_str_out = cJSON_PrintUnformatted(gnss_data_obj);
	if (*json_str_out == NULL) {
		err = -ENOMEM;
	}
cleanup:
	if (gnss_data_obj) {
		cJSON_Delete(gnss_data_obj);
	}
	return err;
}
