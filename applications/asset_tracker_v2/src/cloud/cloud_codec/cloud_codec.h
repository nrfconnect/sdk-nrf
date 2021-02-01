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
};

struct cloud_data_accelerometer {
	/** Accelerometer readings timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Accelerometer readings. */
	double values[3];
};

struct cloud_data_sensors {
	/** Environmental sensors timestamp. UNIX milliseconds. */
	int64_t env_ts;
	/** Temperature in celcius */
	double temp;
	/** Humidity level in percentage */
	double hum;
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
	char *iccid;
	/** Application version and Mobile Network Code. */
	char *appv;
	/** Device board version. */
	const char *brdv;
	/** Modem firmware. */
	char *fw;
};

struct cloud_data_modem_dynamic {
	/** Dynamic modem data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Area code. */
	uint16_t area;
	/** Cell id. */
	uint16_t cell;
	/** Reference Signal Received Power. */
	uint16_t rsrp;
	/** Internet Protocol Address. */
	char *ip;
	/* Mobile Country Code*/
	char *mccmnc;
};

struct cloud_data_ui {
	/** Button number. */
	int btn;
	/** Button data timestamp. UNIX milliseconds. */
	int64_t btn_ts;
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

int cloud_codec_encode_data(struct cloud_codec_data *output);

int cloud_codec_encode_ui_data(struct cloud_codec_data *output);

int cloud_codec_encode_batch_data(struct cloud_codec_data *output);

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
