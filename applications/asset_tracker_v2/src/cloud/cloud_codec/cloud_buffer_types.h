/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_BUFFER_TYPES_H__
#define CLOUD_BUFFER_TYPES_H__

#include <zephyr/net/net_ip.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>
#include <stdint.h>

/**@file
 *
 * @defgroup cloud_buffer_types Cloud codec buffer types
 * @brief    Module that contains the different data types
 *           that are stored in the cloud buffers.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct cloud_data_sensors {
	/** Environmental sensors timestamp. UNIX milliseconds. */
	int64_t env_ts;
	/** Temperature in celcius. */
	double temperature;
	/** Humidity level in percentage. */
	double humidity;
	/** Atmospheric pressure in kilopascal. */
	double pressure;
	/** BSEC Air quality in Indoor-Air-Quality (IAQ) index.
	 *  If -1, the value is not provided.
	 */
	int bsec_air_quality;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_ui {
	/** Button number. */
	int btn;
	/** Button data timestamp. UNIX milliseconds. */
	int64_t btn_ts;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_accelerometer {
	/** Accelerometer readings timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Accelerometer readings. */
	double values[3];
	/** Flag signifying that the data entry is to be published. */
	bool queued : 1;
};

struct cloud_data_battery {
	/** Battery voltage level. */
	uint16_t bat;
	/** Battery data timestamp. UNIX milliseconds. */
	int64_t bat_ts;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_gnss_pvt {
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

enum cloud_data_gnss_format {
	CLOUD_CODEC_GNSS_FORMAT_INVALID,
	CLOUD_CODEC_GNSS_FORMAT_PVT,
	CLOUD_CODEC_GNSS_FORMAT_NMEA
};

struct cloud_data_gnss {
	/** GNSS data timestamp. UNIX milliseconds. */
	int64_t gnss_ts;

	union {
		/** Structure containing PVT data. */
		struct cloud_data_gnss_pvt pvt;

		/* Null terminated NMEA string. The maximum number of characters in an
		 * NMEA string is 83.
		 */
		char nmea[83];
	};

	/** Enum signifying the type of GNSS data that is valid. */
	enum cloud_data_gnss_format format;

	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_modem_dynamic {
	/** Dynamic modem data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Band number. */
	uint8_t band;
	/** Network mode. */
	enum lte_lc_lte_mode nw_mode;
	/* Mobile Country Code. */
	uint16_t mcc;
	/* Mobile Network Code. */
	uint16_t mnc;
	/** Area code. */
	uint16_t area;
	/** Cell id. */
	uint32_t cell;
	/** Reference Signal Received Power. */
	int16_t rsrp;
	/** Internet Protocol Address. */
	char ip[INET6_ADDRSTRLEN];
	/** Access Point Name. */
	char apn[CONFIG_CLOUD_CODEC_APN_LEN_MAX];
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
	bool band_fresh		: 1;
	bool nw_mode_fresh	: 1;
};

struct cloud_data_modem_static {
	/** Static modem data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Integrated Circuit Card Identifier. */
	char iccid[23];
	/** Application version and Mobile Network Code. */
	char appv[CONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN];
	/** Device board version. */
	char brdv[30];
	/** Modem firmware. */
	char fw[40];
	/** Device IMEI. */
	char imei[16];
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* CLOUD_BUFFER_TYPES_H__ */
