/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_CODEC_H__
#define CLOUD_CODEC_H__

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cJSON_os.h>
#include <zephyr/net/net_ip.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>

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

/** @brief Structure containing GNSS data published to cloud. */
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

/** Structure containing boolean variables used to enable/disable inclusion of the corresponding
 *  data type in sample requests sent out by the application module.
 */
struct cloud_data_no_data {
	/** If this flag is set GNSS data is not included in sample requests. */
	bool gnss;

	/** If this flag is set neighbor cell data is not included sample requests. */
	bool neighbor_cell;
};

struct cloud_data_cfg {
	/** Device mode. */
	bool active_mode;
	/** GNSS search timeout. */
	int gnss_timeout;
	/** Time between cloud publications in Active mode. */
	int active_wait_timeout;
	/** Time between cloud publications in Passive mode. */
	int movement_resolution;
	/** Time between cloud publications regardless of movement
	 *  in Passive mode.
	 */
	int movement_timeout;
	/** Accelerometer activity-trigger threshold value in m/s2 */
	double accelerometer_activity_threshold;
	/** Accelerometer inactivity-trigger threshold value in m/s2 */
	double accelerometer_inactivity_threshold;
	/** Accelerometer inactivity-trigger timeout value in seconds */
	double accelerometer_inactivity_timeout;
	/** Variable used to govern what data types are requested by the application. */
	struct cloud_data_no_data no_data;
};

/** Structure containing the magnitude of an impact event detected by the high-G Accelerometer. */
struct cloud_data_impact {
	/** Impact timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Impact magnitude in G. */
	double magnitude;
	/** Flag signifying that the data entry is to be published. */
	bool queued : 1;
};

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
	/** LwM2M object paths. */
	char paths[CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX]
		  [CONFIG_CLOUD_CODEC_LWM2M_PATH_ENTRY_SIZE_MAX];
	/* Number of valid paths in the paths variable. */
	uint8_t valid_object_paths;
};

struct cloud_data_neighbor_cells {
	/** Contains current cell and number of neighbor cells. */
	struct lte_lc_cells_info cell_data;
	/** Contains neighborhood cells. */
	struct lte_lc_ncell neighbor_cells[17];
	/** Neighbor cells data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

struct cloud_data_agps_request {
	/** Mobile Country Code */
	int mcc;
	/** Mobile Network Code */
	int mnc;
	/** Cell ID */
	uint32_t cell;
	/** Area Code */
	uint32_t area;
	/** AGPS request types */
	struct nrf_modem_gnss_agps_data_frame request;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
	/** Flag indicating that the ephemerides will only include visible satellites */
	bool filtered : 1;
	/** Angle above the horizon to constrain the filtered set to. */
	uint8_t mask_angle;

};
struct cloud_data_pgps_request {
	/** Number of requested predictions. */
	uint16_t count;
	/** Time in between predictions, in minutes. */
	uint16_t interval;
	/** The day to start the prediction from. Days since GPS epoch. */
	uint16_t day;
	/** The start time of the prediction in seconds in a day. */
	uint32_t time;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

enum cloud_codec_event_type {
	/** Only used in LwM2M codec. This event carries a config update. */
	CLOUD_CODEC_EVT_CONFIG_UPDATE = 1,
};

struct cloud_codec_evt {
	/** Cloud codec event type */
	enum cloud_codec_event_type type;
	/** New config data */
	struct cloud_data_cfg config_update;
};

/**
 * @brief Event handler prototype.
 *
 * @param[in] evt event type
 */
typedef void (*cloud_codec_evt_handler_t)(const struct cloud_codec_evt *evt);

/**
 * @brief Initialize cloud codec.
 * @note currently only used for config updates in LwM2M
 *
 * @param[in] cfg initial config data
 * @param[in] event_handler handler for events coming from the codec
 *
 * @retval 0 on success
 * @retval -ENOMEM if LwM2M engine couldn't allocate its objects
 */
int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler);

/**
 * @brief Encode cloud codec neighbor cells data.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] neighbor_cells pointer to neighbor cells data
 *
 * @retval 0 on success
 * @retval -ENODATA if data object is not marked valid
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -EINVAL if the data is invalid
 * @retval -ENOTSUP if compiled without support for neighbor cell encoding
 */
int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells);

/**
 * @brief Encode cloud codec A-GPS request.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] agps_request pointer to A-GPS request data
 *
 * @retval 0 on success
 * @retval -ENODATA if data object is not marked valid
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ENOTSUP if nrf cloud codec is used
 */
int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request);

/**
 * @brief Encode cloud codec P-GPS request.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] pgps_request pointer to P-GPS request data
 *
 * @retval 0 on success
 * @retval -ENODATA if data object is not marked valid
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ENOTSUP if nRF or LwM2M cloud codec is used
 */
int cloud_codec_encode_pgps_request(struct cloud_codec_data *output,
				    struct cloud_data_pgps_request *pgps_request);

/**
 * @brief Decode received configuration.
 *
 * @param[in] input string buffer with encoded config
 * @param[in] input_len length of input
 * @param[out] cfg where to store the decoded config
 *
 * @retval 0 on success
 * @retval -ENODATA if string doesn't contain required JSON objects
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ECANCELED if data was already processed
 * @retval -ENOTSUP if LwM2M cloud codec is used
 */
int cloud_codec_decode_config(char *input, size_t input_len,
			      struct cloud_data_cfg *cfg);

/**
 * @brief Encode current configuration.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] cfg pointer to current configuration
 *
 * @retval 0 on success
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ENOTSUP if LwM2M cloud codec is used
 */
int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *cfg);

/**
 * @brief Encode cloud buffer data.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] gnss_buf pointer to GNSS data
 * @param[in] sensor_buf pointer to Sensor data
 * @param[in] modem_stat_buf pointer to static modem data
 * @param[in] modem_dyn_buf pointer to dynamic modem data
 * @param[in] ui_buf pointer to button data
 * @param[in] impact_buf pointer to impact data
 * @param[in] bat_buf pointer to battery data
 *
 * @retval 0 on success
 * @retval -ENODATA if none of the data elements are marked valid
 * @retval -EINVAL if the data is invalid
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ENOTSUP if nRF cloud codec is used
 */
int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_data_gnss *gnss_buf,
			    struct cloud_data_sensors *sensor_buf,
			    struct cloud_data_modem_static *modem_stat_buf,
			    struct cloud_data_modem_dynamic *modem_dyn_buf,
			    struct cloud_data_ui *ui_buf,
			    struct cloud_data_impact *impact_buf,
			    struct cloud_data_battery *bat_buf);

/**
 * @brief Encode UI data.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] ui_buf UI data to encode
 *
 * @retval 0 on success
 * @retval -ENODATA if the data elements is not marked valid
 * @retval -EINVAL if the data is invalid
 * @retval -ENOMEM if codec couldn't allocate memory
 */
int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf);

/**
 * @brief Encode impact data.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] impact_buf impact data to encode
 *
 * @retval 0 on success
 * @retval -ENODATA if the data elements is not marked valid
 * @retval -EINVAL if the data is invalid
 * @retval -ENOMEM if codec couldn't allocate memory
 */
int cloud_codec_encode_impact_data(struct cloud_codec_data *output,
				   struct cloud_data_impact *impact_buf);

/**
 * @brief Encode a batch of cloud buffer data.
 *
 * @param[out] output string buffer for encoding result
 * @param[in] gnss_buf GNSS data buffer
 * @param[in] sensor_buf Sensor data buffer
 * @param[in] modem_stat_buf static modem data buffer
 * @param[in] modem_dyn_buf dynamic modem data buffer
 * @param[in] ui_buf button data buffer
 * @param[in] impact_buf impact data buffer
 * @param[in] bat_buf battery data buffer
 * @param[in] gnss_buf_count length of GNSS data buffer
 * @param[in] sensor_buf_count length of Sensor data buffer
 * @param[in] modem_stat_buf_count length of static modem data buffer
 * @param[in] modem_dyn_buf_count length of dynamic modem data buffer
 * @param[in] ui_buf_count length of button data buffer
 * @param[in] impact_buf_count length of impact data buffer
 * @param[in] bat_buf_count length of battery data buffer
 *
 * @retval 0 on success
 * @retval -ENODATA if none of the data elements are marked valid
 * @retval -EINVAL if the data is invalid
 * @retval -ENOMEM if codec couldn't allocate memory
 * @retval -ENOTSUP if LwM2M cloud codec is used
 */
int cloud_codec_encode_batch_data(struct cloud_codec_data *output,
				  struct cloud_data_gnss *gnss_buf,
				  struct cloud_data_sensors *sensor_buf,
				  struct cloud_data_modem_static *modem_stat_buf,
				  struct cloud_data_modem_dynamic *modem_dyn_buf,
				  struct cloud_data_ui *ui_buf,
				  struct cloud_data_impact *impact_buf,
				  struct cloud_data_battery *bat_buf,
				  size_t gnss_buf_count,
				  size_t sensor_buf_count,
				  size_t modem_stat_buf_count,
				  size_t modem_dyn_buf_count,
				  size_t ui_buf_count,
				  size_t impact_buf_count,
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

void cloud_codec_populate_impact_buffer(
				struct cloud_data_impact *impact_buf,
				struct cloud_data_impact *new_impact_data,
				int *head_impact_buf,
				size_t buffer_count);

void cloud_codec_populate_bat_buffer(struct cloud_data_battery *bat_buffer,
				     struct cloud_data_battery *new_bat_data,
				     int *head_bat_buf,
				     size_t buffer_count);

void cloud_codec_populate_gnss_buffer(struct cloud_data_gnss *gnss_buffer,
				     struct cloud_data_gnss *new_gnss_data,
				     int *head_gnss_buf,
				     size_t buffer_count);

void cloud_codec_populate_modem_dynamic_buffer(
				struct cloud_data_modem_dynamic *modem_buffer,
				struct cloud_data_modem_dynamic *new_modem_data,
				int *head_modem_buf,
				size_t buffer_count);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif
