/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_H__
#define NRF_CLOUD_H__

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/mqtt.h>
#if defined(CONFIG_MODEM_INFO)
#include <modem/modem_info.h>
#endif
#include <net/nrf_cloud_defs.h>
#include <dfu/dfu_target_full_modem.h>
#if defined(CONFIG_NRF_MODEM)
#include <nrf_modem_gnss.h>
#endif
#include <net/nrf_cloud_os.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud nRF Cloud
 * @{
 */

/** MQTT message ID: not specified, use next incremented value */
#define NCT_MSG_ID_USE_NEXT_INCREMENT     0
/** MQTT message ID: subscribe to control channel topics */
#define NCT_MSG_ID_CC_SUB               100
/** MQTT message ID: subscribe to data channel topics */
#define NCT_MSG_ID_DC_SUB               101
/** MQTT message ID: subscribe to FOTA topics */
#define NCT_MSG_ID_FOTA_SUB             103
/** MQTT message ID: unsubscribe from control channel topics */
#define NCT_MSG_ID_CC_UNSUB             150
/** MQTT message ID: unsubscribe from data channel topics */
#define NCT_MSG_ID_DC_UNSUB             151
/** MQTT message ID: unsubscribe from FOTA topics */
#define NCT_MSG_ID_FOTA_UNSUB           153
/** MQTT message ID: request device state (shadow) */
#define NCT_MSG_ID_STATE_REQUEST        200
/** MQTT message ID: request FOTA job */
#define NCT_MSG_ID_FOTA_REQUEST         201
/** MQTT message ID: request BLE FOTA job */
#define NCT_MSG_ID_FOTA_BLE_REQUEST     202
/** MQTT message ID: update device state (shadow) */
#define NCT_MSG_ID_STATE_REPORT         300
/** MQTT message ID: set pairing/associated state */
#define NCT_MSG_ID_PAIR_STATUS_REPORT   301
/** MQTT message ID: update FOTA job status */
#define NCT_MSG_ID_FOTA_REPORT          302
/** MQTT message ID: update BLE FOTA job status */
#define NCT_MSG_ID_FOTA_BLE_REPORT      303
/** MQTT message ID: Start of incrementing range */
#define NCT_MSG_ID_INCREMENT_BEGIN     1000
/** MQTT message ID: End of incrementing range */
#define NCT_MSG_ID_INCREMENT_END       9999
/** MQTT message ID: Start of user specified (custom) range */
#define NCT_MSG_ID_USER_TAG_BEGIN      (NCT_MSG_ID_INCREMENT_END + 1)
/** MQTT message ID: End of user specified (custom) range */
#define NCT_MSG_ID_USER_TAG_END        0xFFFF /* MQTT message IDs are uint16_t */

/** Determine if an MQTT message ID is a user specified tag to be used for ACK matching */
#define IS_VALID_USER_TAG(tag) ((tag >= NCT_MSG_ID_USER_TAG_BEGIN) && \
				(tag <= NCT_MSG_ID_USER_TAG_END))

/** Maximum valid duration for JWTs generated by @ref nrf_cloud_jwt_generate */
#define NRF_CLOUD_JWT_VALID_TIME_S_MAX		(7 * 24 * 60 * 60)
/** Default valid duration for JWTs generated by @ref nrf_cloud_jwt_generate */
#define NRF_CLOUD_JWT_VALID_TIME_S_DEF		(10 * 60)

/** Value used to disable sending a timestamp. Timestamps are in UNIX epoch format. */
#define NRF_CLOUD_NO_TIMESTAMP 0

/** @brief Asynchronous nRF Cloud events notified by the module. */
enum nrf_cloud_evt_type {
	/** The transport to the nRF Cloud is established. */
	NRF_CLOUD_EVT_TRANSPORT_CONNECTED = 0x1,
	/** In the process of connecting to nRF Cloud. */
	NRF_CLOUD_EVT_TRANSPORT_CONNECTING,
	/** There was a request from nRF Cloud to associate the device
	 *  with a user on the nRF Cloud.
	 */
	NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
	/** The device is successfully associated with a user. */
	NRF_CLOUD_EVT_USER_ASSOCIATED,
	/** The device can now start sending sensor data to the cloud. */
	NRF_CLOUD_EVT_READY,
	/** The device received non-specific data from the cloud. */
	NRF_CLOUD_EVT_RX_DATA_GENERAL,
	/** The device received location data from the cloud
	 *  and no response callback was registered.
	 */
	NRF_CLOUD_EVT_RX_DATA_LOCATION,
	/** The device received shadow related data from the cloud. */
	NRF_CLOUD_EVT_RX_DATA_SHADOW,
	/** The device has received a ping response from the cloud. */
	NRF_CLOUD_EVT_PINGRESP,
	/** The data sent to the cloud was acknowledged. */
	NRF_CLOUD_EVT_SENSOR_DATA_ACK,
	/** The transport was disconnected. The status field in the event struct will
	 *  be populated with a @ref nrf_cloud_disconnect_status value.
	 */
	NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED,
	/** A FOTA update has started.
	 * This event is only sent if @kconfig{CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB} is enabled.
	 */
	NRF_CLOUD_EVT_FOTA_START,
	/** The device should be restarted to apply a firmware upgrade */
	NRF_CLOUD_EVT_FOTA_DONE,
	/** An error occurred during the FOTA update. */
	NRF_CLOUD_EVT_FOTA_ERROR,
	/** An error occurred when connecting to the nRF Cloud. The status field in the event
	 *  struct will be populated with a @ref nrf_cloud_connect_result value.
	 */
	NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR,
	/** FOTA update job information has been received.
	 * When ready, the application should start the job by
	 * calling @ref nrf_cloud_fota_job_start.
	 * This event is only sent if @kconfig{CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB} is disabled.
	 */
	NRF_CLOUD_EVT_FOTA_JOB_AVAILABLE,
	/** An error occurred. The status field in the event struct will
	 *  be populated with a @ref nrf_cloud_error_status value.
	 */
	NRF_CLOUD_EVT_ERROR = 0xFF
};

/** @brief nRF Cloud error status, used to describe @ref NRF_CLOUD_EVT_ERROR event. */
enum nrf_cloud_error_status {
	/** No error */
	NRF_CLOUD_ERR_STATUS_NONE = 0,

	/** MQTT connection failed */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_FAIL,
	/** MQTT protocol version not supported by server */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_PROT_VER,
	/** MQTT client identifier rejected by server */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_ID_REJECTED,
	/** MQTT service is unavailable */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_SERVER_UNAVAIL,
	/** Malformed user name or password */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_USR_PWD,
	/** Client is not authorized to connect */
	NRF_CLOUD_ERR_STATUS_MQTT_CONN_NOT_AUTH,
	/** Failed to subscribe to MQTT topic */
	NRF_CLOUD_ERR_STATUS_MQTT_SUB_FAIL,
	/** Error processing A-GNSS data */
	NRF_CLOUD_ERR_STATUS_AGNSS_PROC,
	/** Error processing P-GPS data */
	NRF_CLOUD_ERR_STATUS_PGPS_PROC,
};

/** @brief nRF Cloud disconnect status, used to describe @ref NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED
 * event.
 */
enum nrf_cloud_disconnect_status {
	/** Disconnect was requested by user/application */
	NRF_CLOUD_DISCONNECT_USER_REQUEST,
	/** Disconnect was caused by the cloud */
	NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE,
	/** Socket received POLLNVAL event */
	NRF_CLOUD_DISCONNECT_INVALID_REQUEST,
	/** Socket poll returned an error value or POLLERR event */
	NRF_CLOUD_DISCONNECT_MISC,
};

/** @brief nRF Cloud connect result.
 * Used as return value for nrf_cloud_connect() and to describe
 * @ref NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR event.
 */
enum nrf_cloud_connect_result {
	/** The connection was successful */
	NRF_CLOUD_CONNECT_RES_SUCCESS = 0,
	/** The library is not initialized; @ref nrf_cloud_init */
	NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD = -1,
	/** A network error ocurred */
	NRF_CLOUD_CONNECT_RES_ERR_NETWORK = -3,
	/** A connection error occurred relating to the nRF Cloud backend */
	NRF_CLOUD_CONNECT_RES_ERR_BACKEND = -4,
	/** The connection failed due to an indeterminate reason */
	NRF_CLOUD_CONNECT_RES_ERR_MISC = -5,
	/** Insufficient memory is available */
	NRF_CLOUD_CONNECT_RES_ERR_NO_MEM = -6,
	/** Invalid private key */
	NRF_CLOUD_CONNECT_RES_ERR_PRV_KEY = -7,
	/** Invalid CA or client cert */
	NRF_CLOUD_CONNECT_RES_ERR_CERT = -8,
	/** Other cert issue */
	NRF_CLOUD_CONNECT_RES_ERR_CERT_MISC = -9,
	/** Timeout, SIM card may be out of data */
	NRF_CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA = -10,
	/** The connection exists */
	NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED = -11,
};

/** @brief nRF Cloud error codes.
 * See the <a href="https://api.nrfcloud.com/v1#section/Error-Codes">Error Codes</a>
 * section of nRF Cloud API documentation for more information.
 */
enum nrf_cloud_error {
	NRF_CLOUD_ERROR_UNKNOWN			= -1,
	NRF_CLOUD_ERROR_NONE			= 0,
	/* nRF Cloud API defined error codes below */
	NRF_CLOUD_ERROR_BAD_REQUEST		= 40000,
	NRF_CLOUD_ERROR_INVALID_CERT		= 40001,
	NRF_CLOUD_ERROR_DISSOCIATE		= 40002,
	NRF_CLOUD_ERROR_ACCESS_DENIED		= 40100,
	NRF_CLOUD_ERROR_DEV_ID_IN_USE		= 40101,
	NRF_CLOUD_ERROR_INVALID_OWNER_CODE	= 40102,
	NRF_CLOUD_ERROR_DEV_NOT_ASSOCIATED	= 40103,
	NRF_CLOUD_ERROR_DATA_NOT_FOUND		= 40410,
	NRF_CLOUD_ERROR_NRF_DEV_NOT_FOUND	= 40411,
	NRF_CLOUD_ERROR_NO_DEV_NOT_PROV		= 40412,
	NRF_CLOUD_ERROR_NO_DEV_DISSOCIATE	= 40413,
	NRF_CLOUD_ERROR_NO_DEV_DELETE		= 40414,
	/* Item was not found. No error occurred, the requested item simply does not exist */
	NRF_CLOUD_ERROR_NOT_FOUND_NO_ERROR	= 40499,
	NRF_CLOUD_ERROR_BAD_RANGE		= 41600,
	NRF_CLOUD_ERROR_VALIDATION		= 42200,
	NRF_CLOUD_ERROR_INTERNAL_SERVER		= 50010,
};

/** @brief Sensor types supported by the nRF Cloud. */
enum nrf_cloud_sensor {
	/** The GNSS sensor on the device. */
	NRF_CLOUD_SENSOR_GNSS,
	/** The FLIP movement sensor on the device. */
	NRF_CLOUD_SENSOR_FLIP,
	/** The Button press sensor on the device. */
	NRF_CLOUD_SENSOR_BUTTON,
	/** The TEMP sensor on the device. */
	NRF_CLOUD_SENSOR_TEMP,
	/** The Humidity sensor on the device. */
	NRF_CLOUD_SENSOR_HUMID,
	/** The Air Pressure sensor on the device. */
	NRF_CLOUD_SENSOR_AIR_PRESS,
	/** The Air Quality sensor on the device. */
	NRF_CLOUD_SENSOR_AIR_QUAL,
	/** The RSPR data obtained from the modem. */
	NRF_CLOUD_LTE_LINK_RSRP,
	/** Log messages generated on the device. */
	NRF_CLOUD_LOG,
	/** Log messages generated on the device in dictionary (binary) format. */
	NRF_CLOUD_DICTIONARY_LOG,
	/** The descriptive DEVICE data indicating its status. */
	NRF_CLOUD_DEVICE_INFO,
	/** The light sensor on the device. */
	NRF_CLOUD_SENSOR_LIGHT,
};

/** @brief Topic types supported by nRF Cloud. */
enum nrf_cloud_topic_type {
	/** Endpoint used to update the cloud-side device shadow state . */
	NRF_CLOUD_TOPIC_STATE = 0x1,
	/** Endpoint used to directly message the nRF Cloud Web UI. */
	NRF_CLOUD_TOPIC_MESSAGE,
	/** Endpoint used to publish bulk messages to nRF Cloud. Bulk messages combine multiple
	 *  messages into a single message that will be unwrapped and re-published to the
	 *  message topic in the nRF Cloud backend.
	 */
	NRF_CLOUD_TOPIC_BULK,
	/** Endpoint used to publish binary data to nRF Cloud for certain services.
	 *  One example is dictionary formatted logs enabled by
	 *  @kconfig{CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DICTIONARY}, which enables the
	 *  Zephyr option @kconfig{CONFIG_LOG_DICTIONARY_SUPPORT}. Binary data published to
	 *  this topic should be prefixed by the binary header structure defined in
	 *  nrf_cloud_codec.h - struct nrf_cloud_bin_hdr.  A unique format value
	 *  should be included to distinguish this data from binary logging.
	 */
	NRF_CLOUD_TOPIC_BIN
};

/** @brief FOTA status reported to nRF Cloud. */
enum nrf_cloud_fota_status {
	NRF_CLOUD_FOTA_QUEUED = 0,
	NRF_CLOUD_FOTA_IN_PROGRESS = 1,
	NRF_CLOUD_FOTA_FAILED = 2,
	NRF_CLOUD_FOTA_SUCCEEDED = 3,
	NRF_CLOUD_FOTA_TIMED_OUT = 4,
	NRF_CLOUD_FOTA_CANCELED = 5,
	NRF_CLOUD_FOTA_REJECTED = 6,
	NRF_CLOUD_FOTA_DOWNLOADING = 7,
};

/** @brief FOTA update type. */
enum nrf_cloud_fota_type {
	NRF_CLOUD_FOTA_TYPE__FIRST = 0,

	/** Application update. */
	NRF_CLOUD_FOTA_APPLICATION = NRF_CLOUD_FOTA_TYPE__FIRST,
	/** Delta modem update */
	NRF_CLOUD_FOTA_MODEM_DELTA = 1,
	/** Bootloader update. */
	NRF_CLOUD_FOTA_BOOTLOADER = 2,

	/* Types not handled by this library:
	 * NRF_CLOUD_FOTA_BLE_BOOT = 3,
	 * NRF_CLOUD_FOTA_BLE_SOFTDEVICE = 4,
	 */

	/** Full modem update */
	NRF_CLOUD_FOTA_MODEM_FULL = 5,

	NRF_CLOUD_FOTA_TYPE__INVALID
};

/** Size of nRF Cloud FOTA job ID; version 4 UUID: 32 bytes, 4 hyphens, NULL */
#define NRF_CLOUD_FOTA_JOB_ID_SIZE (32 + 4 + 1)

/** @brief Common FOTA job info */
struct nrf_cloud_fota_job_info {
	enum nrf_cloud_fota_type type;
	/** Null-terminated FOTA job identifier */
	char *id;
	char *host;
	char *path;
	int file_size;
};

/** @brief Validity of an in-progress/installed FOTA update */
enum nrf_cloud_fota_validate_status {
	/** No FOTA update in progress */
	NRF_CLOUD_FOTA_VALIDATE_NONE = 0,
	/** The FOTA update has not yet been validated */
	NRF_CLOUD_FOTA_VALIDATE_PENDING,
	/** Validation succeeded */
	NRF_CLOUD_FOTA_VALIDATE_PASS,
	/** Validation failed */
	NRF_CLOUD_FOTA_VALIDATE_FAIL,
	/** Validation result could not be determined */
	NRF_CLOUD_FOTA_VALIDATE_UNKNOWN,
	/** The validation process is finished */
	NRF_CLOUD_FOTA_VALIDATE_DONE,
	/** An error occurred before validation */
	NRF_CLOUD_FOTA_VALIDATE_ERROR,
};

/** @brief Status flags for tracking the update process of the b1 bootloader (MCUBOOT) */
enum nrf_cloud_fota_bootloader_status_flags {
	/** Initial state */
	NRF_CLOUD_FOTA_BL_STATUS_CLEAR		= 0,
	/** Indicates if the NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE flag has been updated */
	NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET	= (1 << 0),
	/** Indicates if slot 0 was active prior to the update */
	NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE	= (1 << 1),
	/** Indicates if the planned reboot has occurred during the update install process */
	NRF_CLOUD_FOTA_BL_STATUS_REBOOTED	= (1 << 2),
};

/** @brief FOTA job info provided to the settings module to track FOTA job status. */
struct nrf_cloud_settings_fota_job {
	enum nrf_cloud_fota_validate_status validate;
	enum nrf_cloud_fota_type type;
	char id[NRF_CLOUD_FOTA_JOB_ID_SIZE];
	enum nrf_cloud_fota_bootloader_status_flags bl_flags;
};

/** @brief Generic encapsulation for any data that is sent to the cloud. */
struct nrf_cloud_data {
	/** Length of the data. */
	uint32_t len;
	/** Pointer to the data. */
	const void *ptr;
};

/** @brief MQTT topic. */
struct nrf_cloud_topic {
	/** Length of the topic. */
	uint32_t len;
	/** Pointer to the topic. */
	const void *ptr;
};

/** @brief Sensor data transmission parameters. */
struct nrf_cloud_sensor_data {
	/** The sensor that is the source of the data. */
	enum nrf_cloud_sensor type;
	/** Sensor data to be transmitted. */
	struct nrf_cloud_data data;
	/** Unique tag to identify the sent data. Can be used to match
	 * acknowledgment on the NRF_CLOUD_EVT_SENSOR_DATA_ACK event.
	 * Valid range: NCT_MSG_ID_USER_TAG_BEGIN to NCT_MSG_ID_USER_TAG_END.
	 * Any other value will suppress the NRF_CLOUD_EVT_SENSOR_DATA_ACK event.
	 */
	uint16_t tag;
	/** UNIX epoch timestamp (in milliseconds) of the data.
	 * If a value <= NRF_CLOUD_NO_TIMESTAMP (0) is provided, the timestamp will be ignored.
	 */
	int64_t ts_ms;
};

/** @brief Asynchronous events received from the module. */
struct nrf_cloud_evt {
	/** The event that occurred. */
	enum nrf_cloud_evt_type type;
	/** Any status associated with the event. */
	uint32_t status;
	/** Received data. */
	struct nrf_cloud_data data;
	/** Topic on which data was received. */
	struct nrf_cloud_topic topic;
	/** Decoded shadow data: valid for NRF_CLOUD_EVT_RX_DATA_SHADOW events. */
	struct nrf_cloud_obj_shadow_data *shadow;
};

/** @brief Structure used to send data to nRF Cloud. */
struct nrf_cloud_tx_data {
	/** Object containing data to be published */
	struct nrf_cloud_obj *obj;
	/** Pre-encoded data that is to be published if an object is not provided */
	struct nrf_cloud_data data;

	/** Endpoint topic type published to. */
	enum nrf_cloud_topic_type topic_type;
	/** Quality of Service of the message. */
	enum mqtt_qos qos;
	/** Message ID */
	uint32_t id;
};

/** @brief Controls which values are added to the FOTA array in the "serviceInfo" shadow section */
struct nrf_cloud_svc_info_fota {
	/** Flag to indicate if bootloader updates are supported */
	uint8_t bootloader:1;
	/** Flag to indicate if modem delta updates are supported */
	uint8_t modem:1;
	/** Flag to indicate if application updates are supported */
	uint8_t application:1;
	/** Flag to indicate if full modem image updates are supported */
	uint8_t modem_full:1;

	/** Reserved for future use */
	uint8_t _rsvd:4;
};

/** @brief DEPRECATED - No longer used by nRF Cloud */
struct nrf_cloud_svc_info_ui {
	/* Items with UI support on nRF Cloud */
	/** Temperature */
	uint8_t temperature:1;
	/** Location (map) */
	uint8_t gnss:1;
	/** Orientation */
	uint8_t flip:1;
	/** Humidity */
	uint8_t humidity:1;
	/** Air pressure */
	uint8_t air_pressure:1;
	/** RSRP */
	uint8_t rsrp:1;
	/** Logs */
	uint8_t log:1;
	/** Dictionary (binary) Logs */
	uint8_t dictionary_log:1;
	/** Air Quality */
	uint8_t air_quality:1;

	/* Items without UI support on nRF Cloud */
	/** Light sensor */
	uint8_t light_sensor:1;
	/** Button */
	uint8_t button:1;

	/** Unused bits */
	uint8_t _rsvd:5;
};

/** @brief How the info sections are handled when encoding shadow data */
enum nrf_cloud_shadow_info {
	/** Data will not be modified */
	NRF_CLOUD_INFO_NO_CHANGE = 0,
	/** Data will be set/updated */
	NRF_CLOUD_INFO_SET = 1,
	/** Data section will be cleared */
	NRF_CLOUD_INFO_CLEAR = 2,
};

/** @brief Modem info data and which sections should be encoded */
struct nrf_cloud_modem_info {
	enum nrf_cloud_shadow_info device;
	enum nrf_cloud_shadow_info network;
	enum nrf_cloud_shadow_info sim;

#if defined(CONFIG_MODEM_INFO)
	/** Pointer to a populated @ref modem_param_info struct.
	 * If NULL, modem data will be fetched.
	 */
	const struct modem_param_info *mpi;
#endif

	/** Application version string to report to nRF Cloud.
	 *  Used only if device parameter is NRF_CLOUD_INFO_SET.
	 */
	const char *application_version;
};

/** @brief Structure to specify which components are added to the encoded service info object */
struct nrf_cloud_svc_info {
	/** Specify FOTA components to enable, set to NULL to remove the FOTA entry */
	struct nrf_cloud_svc_info_fota *fota;

	/** DEPRECATED - nRF Cloud no longer requires the device to set UI values in the shadow */
	struct nrf_cloud_svc_info_ui *ui;
};

/** @brief Structure to specify which components are added to the encoded device status object */
struct nrf_cloud_device_status {
	/** Specify which modem info components to include, set to NULL to skip */
	struct nrf_cloud_modem_info *modem;
	/** Specify which service info components to include, set to NULL to skip */
	struct nrf_cloud_svc_info *svc;
	/** Specify how the connection info data should be handled */
	enum nrf_cloud_shadow_info conn_inf;
};

/** @brief GNSS data type contained in @ref nrf_cloud_gnss_data */
enum nrf_cloud_gnss_type {
	/** Invalid data type */
	NRF_CLOUD_GNSS_TYPE_INVALID,
	/** Specifies a data type of @ref nrf_cloud_gnss_pvt */
	NRF_CLOUD_GNSS_TYPE_PVT,
	/** Specifies a data type of @ref nrf_cloud_gnss_nmea */
	NRF_CLOUD_GNSS_TYPE_NMEA,
	/** Specifies a data type of nrf_modem_gnss_pvt_data_frame */
	NRF_CLOUD_GNSS_TYPE_MODEM_PVT,
	/** Specifies a data type of nrf_modem_gnss_nmea_data_frame */
	NRF_CLOUD_GNSS_TYPE_MODEM_NMEA,
};

/** @brief PVT data */
struct nrf_cloud_gnss_pvt {
	/** Latitude in degrees; required. */
	double lat;
	/** Longitude in degrees; required. */
	double lon;
	/** Position accuracy (2D 1-sigma) in meters; required. */
	float accuracy;

	/** Altitude above WGS-84 ellipsoid in meters; optional. */
	float alt;
	/** Horizontal speed in m/s; optional. */
	float speed;
	/** Heading of user movement in degrees; optional. */
	float heading;

	/** Altitude value is set */
	uint8_t has_alt:1;
	/** Speed value is set */
	uint8_t has_speed:1;
	/** Heading value is set */
	uint8_t has_heading:1;

	/** Reserved for future use */
	uint8_t _rsvd:5;
};

struct nrf_cloud_credentials_status {
	/* Configured sec_tag for nRF Cloud */
	uint32_t sec_tag;

	/* Flags to indicate if the specified credentials exist */
	uint8_t ca:1;
	uint8_t client_cert:1;
	uint8_t prv_key:1;
};

#if !defined(CONFIG_NRF_MODEM)
#define NRF_MODEM_GNSS_NMEA_MAX_LEN 83
#endif
/** @brief NMEA data */
struct nrf_cloud_gnss_nmea {
	/** Null-terminated NMEA sentence. Supported types are GPGGA, GPGLL, GPRMC.
	 * Max string length is NRF_MODEM_GNSS_NMEA_MAX_LEN - 1
	 */
	const char *sentence;
};

/** @brief GNSS data to be sent to nRF Cloud as a device message */
struct nrf_cloud_gnss_data {
	/** The type of GNSS data below. */
	enum nrf_cloud_gnss_type type;
	/** UNIX epoch timestamp (in milliseconds) of the data.
	 * If a value <= NRF_CLOUD_NO_TIMESTAMP (0) is provided, the timestamp will be ignored.
	 */
	int64_t ts_ms;
	union {
		/** For type: NRF_CLOUD_GNSS_TYPE_PVT */
		struct nrf_cloud_gnss_pvt pvt;
		/** For type: NRF_CLOUD_GNSS_TYPE_NMEA */
		struct nrf_cloud_gnss_nmea nmea;
#if defined(CONFIG_NRF_MODEM)
		/** For type: NRF_CLOUD_GNSS_TYPE_MODEM_PVT */
		struct nrf_modem_gnss_nmea_data_frame *mdm_nmea;
		/** For type: NRF_CLOUD_GNSS_TYPE_MODEM_NMEA */
		struct nrf_modem_gnss_pvt_data_frame *mdm_pvt;
#endif
	};
};

#ifdef CONFIG_NRF_CLOUD_GATEWAY
/** @brief Structure to hold message received from nRF Cloud. */
struct nrf_cloud_gw_data {
	struct nrf_cloud_data data;
	uint16_t id;
};
#endif

/** @brief Data to control behavior of the nrf_cloud library from the
 *  cloud side. This data is stored in the device shadow.
 */
struct nrf_cloud_ctrl_data {
	/** If true, nrf_cloud_alert_send() or nrf_cloud_rest_alert_send()
	 *  will transfer alerts to the cloud whenever they are raised.
	 *  If false, alerts will be suppressed.
	 */
	bool alerts_enabled;
	/** If 0, the nrf_cloud library logging backend will be disabled.
	 *  If values from 1 to 4, this level and any lower levels will
	 *  be sent to the cloud. Level 1 is most urgent (LOG_ERR),
	 *  level 4 least (LOG_DBG).
	 */
	int log_level;
};

/**
 * @brief  Event handler registered with the module to handle asynchronous
 * events from the module.
 *
 * @param[in]  evt The event and any associated parameters.
 */
typedef void (*nrf_cloud_event_handler_t)(const struct nrf_cloud_evt *evt);

/** @brief Initialization parameters for the module. */
struct nrf_cloud_init_param {
	/** Event handler that is registered with the module. */
	nrf_cloud_event_handler_t event_handler;
	/** Null-terminated MQTT client ID string.
	 * Must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN.
	 * Must be set if NRF_CLOUD_CLIENT_ID_SRC_RUNTIME
	 * is enabled; otherwise, NULL.
	 */
	char *client_id;
	/** Flash device information required for full modem FOTA updates.
	 * Only used if @kconfig{CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE} is enabled.
	 * Ignored if @kconfig{CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION} is
	 * enabled.
	 */
	struct dfu_target_fmfu_fdev *fmfu_dev_inf;
	/** Optional hooks to override memory management functions.
	 *  If set, @ref nrf_cloud_os_mem_hooks_init will be called by @ref nrf_cloud_init.
	 */
	struct nrf_cloud_os_mem_hooks *hooks;
	/** Optional application version that is reported to nRF Cloud if
	 * @kconfig{CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS} is enabled.
	 */
	const char *application_version;
};

/**
 * @brief Initialize the module.
 *
 * @note This API must be called prior to using nRF Cloud
 *       and it must return successfully.
 *
 * @param[in] param Initialization parameters.
 *
 * @retval 0       If successful.
 * @retval -EACCES Already initialized or @ref nrf_cloud_uninit is in progress.
 * @return A negative value indicates an error.
 */
int nrf_cloud_init(const struct nrf_cloud_init_param *param);

/**
 * @brief Uninitialize nRF Cloud; disconnects cloud connection
 *  and cleans up allocated memory.
 *
 * @note If nRF Cloud FOTA is enabled and a FOTA job is active
 *  uninit will not be performed.
 *
 * @note This function is solely intended to allow this library to be deactivated when
 *  no longer needed. You can recover from any error state you may encounter without calling this
 *  function. See @ref nrf_cloud_disconnect for a way to reset the nRF Cloud connection state
 *  without uninitializing the whole library.
 *
 * @retval 0        If successful.
 * @retval -EBUSY   If a FOTA job is in progress.
 * @retval -EISCONN If the expected disconnect event did not occur.
 * @retval -ETIME   If @kconfig{CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD} is enabled and
 *                  the connection poll thread did not become inactive.
 * @return A negative value indicates an error.
 */
int nrf_cloud_uninit(void);

/**
 * @brief Connect to the cloud.
 *
 * The @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED event indicates
 * that the cloud connection has been established.
 * In any stage of connecting to the cloud, an @ref NRF_CLOUD_EVT_ERROR
 * might be received.
 * If it is received before @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
 * the application may repeat the call to @ref nrf_cloud_connect to try again.
 *
 * @retval Connect result defined by enum nrf_cloud_connect_result.
 */
int nrf_cloud_connect(void);

/**
 * @brief Send sensor data reliably.
 *
 * This API should only be called after receiving an
 * @ref NRF_CLOUD_EVT_READY event.
 * If the API succeeds, you can expect the
 * @ref NRF_CLOUD_EVT_SENSOR_DATA_ACK event for data sent with
 * a valid tag value.
 *
 * @param[in] param Sensor data; the data pointed to by param->data.ptr
 *                  must be a string.
 *
 * @retval 0       If successful.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_sensor_data_send(const struct nrf_cloud_sensor_data *param);

/**
 * @brief Update the device status in the shadow.
 *
 * @param[in] dev_status Device status to be encoded.
 *
 * @retval 0       If successful.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_shadow_device_status_update(const struct nrf_cloud_device_status * const dev_status);

/**
 * @brief Stream sensor data. Uses lowest QoS; no acknowledgment,
 *
 * This API should only be called after receiving an
 * @ref NRF_CLOUD_EVT_READY event.
 *
 * @param[in] param Sensor data; tag value is ignored.
 *
 * @retval 0       If successful.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_sensor_data_stream(const struct nrf_cloud_sensor_data *param);

/**
 * @brief Send data to nRF Cloud.
 *
 * This API is used for sending data to nRF Cloud.
 *
 * @param[in] msg Pointer to a structure containing data and topic
 *                information.
 *
 * @retval 0       If successful.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @retval -EIO Error; failed to encode data.
 * @return A negative value indicates an error.
 */
int nrf_cloud_send(const struct nrf_cloud_tx_data *msg);

/**
 * @brief Update the device's shadow with the data from the provided object.
 *
 * @param[in] shadow_obj Object containing data to be written to the device's shadow.
 *
 * @retval 0       If successful.
 * @retval -EINVAL Error; invalid parameter.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @retval -EIO Error; failed to encode data.
 * @return A negative value indicates an error.
 */
int nrf_cloud_obj_shadow_update(struct nrf_cloud_obj *const shadow_obj);

/**
 * @brief Disconnect from the cloud.
 *
 * This API may be called any time after receiving the
 * @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED event.
 * The @ref NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED event indicates
 * that the disconnect has completed successfully.
 *
 * @retval 0       If successful.
 * @retval -EACCES Cloud connection is not established; wait for
 *                 @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED.
 * @return A negative value indicates an error.
 */
int nrf_cloud_disconnect(void);

/**
 * @brief Function that must be called periodically to keep the module
 * functional.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_process(void);

/**
 * @brief The application has handled reinit after a modem FOTA update and the
 *        LTE link has been reestablished.
 *        This function must be called in order to complete the modem update.
 *        Depends on @kconfig{CONFIG_NRF_CLOUD_FOTA}.
 *
 * @param[in] fota_success true if modem update was successful, false otherwise.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_modem_fota_completed(const bool fota_success);

/**
 * @brief Retrieve the current device ID.
 *
 * @param[in,out] id_buf Buffer to receive the device ID as a null-terminated string.
 * @param[in] id_buf_sz  Size of buffer, maximum size is NRF_CLOUD_CLIENT_ID_MAX_LEN + 1.
 *
 * @retval 0         If successful.
 * @retval -EMSGSIZE The provided buffer is too small.
 * @retval -EIO      The client ID could not be initialized.
 * @retval -ENXIO    The Kconfig option @kconfig{CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME} is enabled
 *                   but the runtime client ID has not been set.
 *                   See @ref nrf_cloud_client_id_runtime_set.
 * @return A negative value indicates an error.
 */
int nrf_cloud_client_id_get(char *id_buf, size_t id_buf_sz);

/**
 * @brief Set the device ID at runtime.
 *        Requires @kconfig{CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME} to be enabled.
 *
 * @note This function does not perform any management of the device's connection to nRF Cloud.
 *
 * @param[in] client_id Null-terminated device ID string.
 *                      Max string length is NRF_CLOUD_CLIENT_ID_MAX_LEN.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_client_id_runtime_set(const char *const client_id);

/**
 * @brief Retrieve the current customer tenant ID.
 *
 * @param[in,out] id_buf Buffer to receive the tenant ID.
 * @param[in] id_len     Size of buffer (NRF_CLOUD_TENANT_ID_MAX_LEN).
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_tenant_id_get(char *id_buf, size_t id_len);

/**
 * @brief Generate a JWT to be used with nRF Cloud's REST API.
 *        This library's configured values for client id and sec tag (NRF_CLOUD_SEC_TAG)
 *        will be used for generating the JWT.
 *
 * @param[in] time_valid_s How long (seconds) the JWT will be valid. Maximum
 *                         duration specified by @ref NRF_CLOUD_JWT_VALID_TIME_S_MAX.
 *                         If zero, NRF_CLOUD_JWT_VALID_TIME_S_DEF will be used.
 * @param[in,out] jwt_buf Buffer to hold the JWT.
 * @param[in] jwt_buf_sz  Size of the buffer (recommended size >= 600 bytes).
 *
 * @retval 0      JWT generated successfully.
 * @retval -ETIME Modem does not have valid date/time, JWT not generated.
 * @return A negative value indicates an error.
 */
int nrf_cloud_jwt_generate(uint32_t time_valid_s, char * const jwt_buf, size_t jwt_buf_sz);

/**
 * @brief Process/validate a pending FOTA update job. Typically the job
 *        information is read from non-volatile storage on startup. This function
 *        is intended to be used by custom REST-based FOTA implementations.
 *        It is called internally if @kconfig{CONFIG_NRF_CLOUD_FOTA} is enabled.
 *        For pending NRF_CLOUD_FOTA_MODEM_DELTA jobs the modem library must
 *        be initialized before calling this function, otherwise the job will
 *        be marked as completed without validation.
 *
 * @param[in] job FOTA job state information.
 * @param[out] reboot_required A reboot is needed to complete a FOTA update.
 *
 * @retval 0       A Pending FOTA job has been processed.
 * @retval -ENODEV No pending/unvalidated FOTA job exists.
 * @retval -ENOENT Error; unknown FOTA job type.
 * @retval -ESRCH Error; not configured for FOTA job type.
 */
int nrf_cloud_pending_fota_job_process(struct nrf_cloud_settings_fota_job * const job,
				       bool * const reboot_required);

/**
 * @brief Set the active bootloader (B1) slot flag which is needed
 *        to validate a bootloader FOTA update. For proper functionality,
 *        @kconfig{CONFIG_FOTA_DOWNLOAD} must be enabled.
 *
 * @param[in,out] job FOTA job state information.
 *
 * @retval 0 Flag set successfully or not a bootloader FOTA update.
 * @return A negative value indicates an error.
 */
int nrf_cloud_bootloader_fota_slot_set(struct nrf_cloud_settings_fota_job * const job);

/**
 * @brief Retrieve the FOTA type of a pending FOTA job. A value of
 *        NRF_CLOUD_FOTA_TYPE__INVALID indicates that there are no pending FOTA jobs.
 *        Depends on @kconfig{CONFIG_NRF_CLOUD_FOTA}.
 *
 * @param[out] pending_fota_type FOTA type of pending job.
 *
 * @retval 0 FOTA type was retrieved.
 * @retval -EINVAL Error; invalid parameter.
 * @return A negative value indicates an error.
 */
int nrf_cloud_fota_pending_job_type_get(enum nrf_cloud_fota_type * const pending_fota_type);

/**
 * @brief Validate a pending FOTA installation before initializing this library.
 *        This function enables the application to control the reboot/reinit process during FOTA
 *        updates. If this function is not called directly by the application, it will
 *        be called internally when @ref nrf_cloud_init is executed.
 *        For pending NRF_CLOUD_FOTA_MODEM_DELTA jobs the modem library must
 *        be initialized before calling this function, otherwise the job will
 *        be marked as completed without validation.
 *        Depends on @kconfig{CONFIG_NRF_CLOUD_FOTA}.
 *
 * @param[out] fota_type_out FOTA type of pending job.
 *                           NRF_CLOUD_FOTA_TYPE__INVALID if no pending job.
 *                           Can be NULL.
 *
 * @retval 0 Pending FOTA job processed.
 * @retval 1 Pending FOTA job processed and requires the application to perform a reboot or,
 *           for modem FOTA types, reinitialization of the modem library.
 * @retval -ENODEV No pending/unvalidated FOTA job exists.
 * @retval -EIO Error; failed to load FOTA job info from settings module.
 * @retval -ENOENT Error; unknown FOTA job type.
 */
int nrf_cloud_fota_pending_job_validate(enum nrf_cloud_fota_type * const fota_type_out);

/**
 * @brief Set the flash device used for full modem FOTA updates.
 *        This function is intended to be used by custom REST-based FOTA implementations.
 *        It is called internally when @ref nrf_cloud_init is executed if
 *        @kconfig{CONFIG_NRF_CLOUD_FOTA} is enabled. It can be called before @ref nrf_cloud_init
 *        if required by the application.
 *
 * @param[in] fmfu_dev_inf Flash device information.
 *
 * @retval 0 Flash device was successfully set.
 * @retval 1 Flash device has already been set.
 * @return A negative value indicates an error.
 */
int nrf_cloud_fota_fmfu_dev_set(const struct dfu_target_fmfu_fdev *const fmfu_dev_inf);

/**
 * @brief Install a full modem update from flash. If successful,
 *        reboot the device or reinit the modem to complete the update.
 *        This function is intended to be used by custom REST-based FOTA implementations.
 *        If @kconfig{CONFIG_NRF_CLOUD_FOTA} is enabled,
 *        call @ref nrf_cloud_fota_pending_job_validate
 *        to install a downloaded NRF_CLOUD_FOTA_MODEM_FULL update after the
 *        @ref NRF_CLOUD_EVT_FOTA_DONE event is received.
 *        Depends on @kconfig{CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE}.
 *
 * @retval 0 Modem update installed successfully.
 * @retval -EACCES Flash device not set; see @ref nrf_cloud_fota_fmfu_dev_set or
 *                 @ref nrf_cloud_init.
 * @return A negative value indicates an error. Modem update not installed.
 */
int nrf_cloud_fota_fmfu_apply(void);

/**
 * @brief Determine if FOTA type is modem related.
 *
 * @return true if FOTA is modem type, otherwise false.
 */
bool nrf_cloud_fota_is_type_modem(const enum nrf_cloud_fota_type type);

/**
 * @brief Determine if the specified FOTA type is enabled by the
 *        configuration. This function returns false if both @kconfig{CONFIG_NRF_CLOUD_FOTA}
 *        and @kconfig{CONFIG_NRF_CLOUD_REST} are disabled.
 *        REST-based applications are responsible for implementing FOTA updates
 *        for all configured types.
 *
 * @param[in] type Fota type.
 *
 * @retval true  Specified FOTA type is enabled by the configuration.
 * @retval false Specified FOTA type is not enabled by the configuration.
 */
bool nrf_cloud_fota_is_type_enabled(const enum nrf_cloud_fota_type type);

/**
 * @brief Start the FOTA update job.
 *        This function should be called after the NRF_CLOUD_EVT_FOTA_JOB_AVAILABLE event
 *        is received.
 *        This function should only be called if @kconfig{CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB}
 *        is disabled.
 *        Depends on @kconfig{CONFIG_NRF_CLOUD_FOTA}.
 *
 * @retval 0            FOTA update job started successfully.
 * @retval -ENOTSUP     Error; @kconfig{CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB} is enabled.
 * @retval -ENOENT      Error; FOTA update job information has not been received.
 * @retval -EINPROGRESS Error; a FOTA update job is in progress.
 * @retval -EPIPE       Error; failed to start FOTA download.
 * @return A negative value indicates an error starting the job.
 */
int nrf_cloud_fota_job_start(void);

/**
 * @brief Check if credentials exist in the configured location.
 *
 * @param[out] cs Results of credentials check.
 *
 * @retval 0 Successfully obtained status of credentials.
 * @retval -EIO Error checking if credential exists.
 * @retval -EINVAL Error; invalid parameter.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_check(struct nrf_cloud_credentials_status *const cs);

/**
 * @brief Check if the credentials required for connecting to nRF Cloud exist.
 *        The application's configuration is used to determine which credentials
 *        are required.
 *
 * @retval 0 Required credentials exist.
 * @retval -EIO Error checking if credentials exists.
 * @retval -ENOTSUP Required credentials do not exist.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_configured_check(void);

/**
 * @brief Set the sec tag used for nRF Cloud credentials.
 *        The default sec tag value is @kconfig{CONFIG_NRF_CLOUD_COAP_SEC_TAG} or
 *        @kconfig{CONFIG_NRF_CLOUD_COAP_SEC_TAG} for CoAP.
 *
 * @note This API only needs to be called if the default configured sec tag value is no
 *       longer applicable. This function does not perform any management of the
 *       device's connection to nRF Cloud.
 *
 * @param sec_tag The sec tag.
 *
 */
void nrf_cloud_sec_tag_set(const sec_tag_t sec_tag);

/**
 * @brief Get the sec tag used for nRF Cloud credentials.
 *
 * @return The sec tag.
 */
sec_tag_t nrf_cloud_sec_tag_get(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_H__ */
