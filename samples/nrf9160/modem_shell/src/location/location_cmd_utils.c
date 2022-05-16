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

#define MSG_TYPE      "messageType"
#define MSG_APP_ID    "appId"
#define MSG_DATA      "data"
#define MSG_TIMESTAMP "ts"

#define MSG_APP_ID_GNSS_FORMAT_NMEA "GPS"
#define MSG_APP_ID_GNSS_FORMAT_PVT  "GNSS"
#define MSG_TYPE_DATA               "DATA"

#define MSG_TYPE_DATA_GNSS_LATITUDE  "lat"
#define MSG_TYPE_DATA_GNSS_LONGITUDE "lon"
#define MSG_TYPE_DATA_GNSS_ACCURACY  "acc"

int location_cmd_utils_gnss_loc_to_cloud_payload_json_encode(
	enum location_cmd_cloud_data_gnss_format format, const struct location_data *location_data,
	char **json_str_out)
{
	__ASSERT_NO_MSG(location_data != NULL);
	__ASSERT_NO_MSG(json_str_out != NULL);

	/* This structure corresponds to the General Message Schema described in the
	 * application-protocols repo:
	 * https://github.com/nRFCloud/application-protocols
	 */

	int err = 0;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *gnss_data_obj;
	int64_t timestamp = 0;
	char *app_id = MSG_APP_ID_GNSS_FORMAT_NMEA;

	if (root_obj == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

#if defined(CONFIG_DATE_TIME)
	err = date_time_now(&timestamp);
	if (err) {
		mosh_error("Failed to create timestamp");
		goto cleanup;
	}
#endif
	/* GNSS data based on used format */
	if (format == CLOUD_GNSS_FORMAT_NMEA) {
		int payloadlength;
		char nmea_buf[64];

		payloadlength =
			location_cmd_utils_generate_nmea(location_data, nmea_buf, sizeof(nmea_buf));
		if (!cJSON_AddStringToObjectCS(root_obj, MSG_DATA, nmea_buf)) {
			err = -ENOMEM;
			goto cleanup;
		}
	} else {
		assert(format == CLOUD_GNSS_FORMAT_PVT);

		app_id = MSG_APP_ID_GNSS_FORMAT_PVT;
		gnss_data_obj = cJSON_CreateObject();
		if (gnss_data_obj == NULL) {
			err = -ENOMEM;
			goto cleanup;
		}
		if (!cJSON_AddNumberToObjectCS(gnss_data_obj, MSG_TYPE_DATA_GNSS_LATITUDE,
					       location_data->latitude) ||
		    !cJSON_AddNumberToObjectCS(gnss_data_obj, MSG_TYPE_DATA_GNSS_LONGITUDE,
					       location_data->longitude) ||
		    !cJSON_AddNumberToObjectCS(gnss_data_obj, MSG_TYPE_DATA_GNSS_ACCURACY,
					       location_data->accuracy) ||
		    !cJSON_AddItemToObject(root_obj, MSG_DATA, gnss_data_obj)) {
			cJSON_Delete(gnss_data_obj);
			err = -ENOMEM;
			goto cleanup;
		}
	}

	/* Common metadata */
	if (!cJSON_AddStringToObjectCS(root_obj, MSG_APP_ID, app_id) ||
	    !cJSON_AddStringToObjectCS(root_obj, MSG_TYPE, MSG_TYPE_DATA) ||
	    !cJSON_AddNumberToObjectCS(root_obj, MSG_TIMESTAMP, timestamp)) {
		err = -ENOMEM;
		goto cleanup;
	}

	*json_str_out = cJSON_PrintUnformatted(root_obj);
	if (*json_str_out == NULL) {
		err = -ENOMEM;
	}
cleanup:
	if (root_obj) {
		cJSON_Delete(root_obj);
	}
	return err;
}
