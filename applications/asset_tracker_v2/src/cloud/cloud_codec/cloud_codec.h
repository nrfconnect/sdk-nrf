/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_CODEC_H__
#define CLOUD_CODEC_H__

#include <zephyr.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON_os.h"
#include <net/net_ip.h>

/**@file
 *
 * @defgroup cloud_codec Cloud codec
 * @brief    Module that encodes and decodes cloud communication.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure containing battery data published to cloud. */
struct cloud_data_battery {
	/** Battery voltage level. */
	uint16_t bat;
	/** Battery data timestamp. UNIX milliseconds. */
	int64_t bat_ts;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

/** @brief Structure containing GPS data published to cloud. */
struct cloud_data_gps {
	/** GPS data timestamp. UNIX milliseconds. */
	int64_t gps_ts;
	/** Longitude */
	double longi;
	/** Latitude */
	double lat;
	/** Altitude above WGS-84 ellipsoid in meters. */
	float alt;
	/** Accuracy in (2D 1-sigma) in meters. */
	float acc;
	/** Horizontal speed in meters. */
	float spd;
	/** Heading of movement in degrees. */
	float hdg;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_cfg {
	/** Device mode. */
	bool active_mode;
	/** GPS search timeout. */
	int gps_timeout;
	/** Time between cloud publications in Active mode. */
	int active_wait_timeout;
	/** Time between cloud publications in Passive mode. */
	int movement_resolution;
	/** Time between cloud publications regardless of movement
	 *  in Passive mode.
	 */
	int movement_timeout;
	/** Accelerometer trigger threshold value in m/s2. */
	double accelerometer_threshold;

	/** Flags to signify if the corresponding data value is fresh and can be used. */
	bool active_mode_fresh		   : 1;
	bool gps_timeout_fresh		   : 1;
	bool active_wait_timeout_fresh	   : 1;
	bool movement_resolution_fresh	   : 1;
	bool movement_timeout_fresh	   : 1;
	bool accelerometer_threshold_fresh : 1;
};

struct cloud_data_accelerometer {
	/** Accelerometer readings timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Accelerometer readings. */
	double values[3];
	/** Flag signifying that the data entry is to be published. */
	bool queued : 1;
};

struct cloud_data_sensors {
	/** Environmental sensors timestamp. UNIX milliseconds. */
	int64_t env_ts;
	/** Temperature in celcius */
	double temp;
	/** Humidity level in percentage */
	double hum;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_modem_static {
	/** Static modem data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Band number. */
	uint16_t bnd;
	/** Network mode GPS. */
	uint16_t nw_gps;
	/** Network mode LTE-M. */
	uint16_t nw_lte_m;
	/** Network mode NB-IoT. */
	uint16_t nw_nb_iot;
	/** Integrated Circuit Card Identifier. */
	char iccid[23];
	/** Application version and Mobile Network Code. */
	char appv[CONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN];
	/** Device board version. */
	char brdv[30];
	/** Modem firmware. */
	char fw[40];
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_modem_dynamic {
	/** Dynamic modem data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Area code. */
	uint16_t area;
	/** Cell id. */
	uint32_t cell;
	/** Reference Signal Received Power. */
	uint16_t rsrp;
	/** Internet Protocol Address. */
	char ip[INET6_ADDRSTRLEN];
	/* Mobile Country Code*/
	char mccmnc[7];
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;

	/** Flags to signify if the corresponding data value is fresh and can be used. */
	bool area_code_fresh	: 1;
	bool cell_id_fresh	: 1;
	bool rsrp_fresh		: 1;
	bool ip_address_fresh	: 1;
	bool mccmnc_fresh	: 1;
};

struct cloud_data_ui {
	/** Button number. */
	int btn;
	/** Button data timestamp. UNIX milliseconds. */
	int64_t btn_ts;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_codec_data {
	/** Encoded output. */
	char *buf;
	/** Length of encoded output. */
	size_t len;
};

static inline void cloud_codec_init(void)
{
	cJSON_Init();
}

int cloud_codec_decode_config(char *input, struct cloud_data_cfg *cfg);

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *cfg);

int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_data_gps *gps_buf,
			    struct cloud_data_sensors *sensor_buf,
			    struct cloud_data_modem_static *modem_stat_buf,
			    struct cloud_data_modem_dynamic *modem_dyn_buf,
			    struct cloud_data_ui *ui_buf,
			    struct cloud_data_accelerometer *accel_buf,
			    struct cloud_data_battery *bat_buf);

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf);

int cloud_codec_encode_batch_data(
				struct cloud_codec_data *output,
				struct cloud_data_gps *gps_buf,
				struct cloud_data_sensors *sensor_buf,
				struct cloud_data_modem_dynamic *modem_dyn_buf,
				struct cloud_data_ui *ui_buf,
				struct cloud_data_accelerometer *accel_buf,
				struct cloud_data_battery *bat_buf,
				size_t gps_buf_count,
				size_t sensor_buf_count,
				size_t modem_dyn_buf_count,
				size_t ui_buf_count,
				size_t accel_buf_count,
				size_t bat_buf_count);

void cloud_codec_populate_sensor_buffer(
				struct cloud_data_sensors *sensor_buffer,
				struct cloud_data_sensors *new_sensor_data,
				int *head_sensor_buf,
				size_t buffer_count);

void cloud_codec_populate_ui_buffer(struct cloud_data_ui *ui_buffer,
				    struct cloud_data_ui *new_ui_data,
				    int *head_ui_buf,
				    size_t buffer_count);

void cloud_codec_populate_accel_buffer(
				struct cloud_data_accelerometer *accel_buf,
				struct cloud_data_accelerometer *new_accel_data,
				int *head_accel_buf,
				size_t buffer_count);

void cloud_codec_populate_bat_buffer(struct cloud_data_battery *bat_buffer,
				     struct cloud_data_battery *new_bat_data,
				     int *head_bat_buf,
				     size_t buffer_count);

void cloud_codec_populate_gps_buffer(struct cloud_data_gps *gps_buffer,
				     struct cloud_data_gps *new_gps_data,
				     int *head_gps_buf,
				     size_t buffer_count);

void cloud_codec_populate_modem_dynamic_buffer(
				struct cloud_data_modem_dynamic *modem_buffer,
				struct cloud_data_modem_dynamic *new_modem_data,
				int *head_modem_buf,
				size_t buffer_count);

static inline void cloud_codec_release_data(struct cloud_codec_data *output)
{
	cJSON_FreeString(output->buf);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif
