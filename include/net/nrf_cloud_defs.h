/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_DEFS_H__
#define NRF_CLOUD_DEFS_H__

#include <zephyr/toolchain/common.h>

/** @defgroup nrf_cloud_defs nRF Cloud common defines
 * @{
 */

/* Message schemas defined by nRF Cloud:
 * https://github.com/nRFCloud/application-protocols/tree/v1/schemas
 */

/* nRF Cloud appID values */
#define NRF_CLOUD_JSON_APPID_KEY		"appId"
#define NRF_CLOUD_JSON_APPID_VAL_AGPS		"AGPS"
#define NRF_CLOUD_JSON_APPID_VAL_PGPS		"PGPS"
#define NRF_CLOUD_JSON_APPID_VAL_GNSS		"GNSS"
#define NRF_CLOUD_JSON_APPID_VAL_LOCATION	"GROUND_FIX"
#define NRF_CLOUD_JSON_APPID_VAL_DEVICE		"DEVICE"
#define NRF_CLOUD_JSON_APPID_VAL_FLIP		"FLIP"
#define NRF_CLOUD_JSON_APPID_VAL_BTN		"BUTTON"
#define NRF_CLOUD_JSON_APPID_VAL_TEMP		"TEMP"
#define NRF_CLOUD_JSON_APPID_VAL_HUMID		"HUMID"
#define NRF_CLOUD_JSON_APPID_VAL_AIR_PRESS	"AIR_PRESS"
#define NRF_CLOUD_JSON_APPID_VAL_AIR_QUAL	"AIR_QUAL"
#define NRF_CLOUD_JSON_APPID_VAL_RSRP		"RSRP"
#define NRF_CLOUD_JSON_APPID_VAL_LIGHT		"LIGHT"
#define NRF_CLOUD_JSON_APPID_VAL_MODEM		"MODEM"
#define NRF_CLOUD_JSON_APPID_VAL_ALERT		"ALERT"
#define NRF_CLOUD_JSON_APPID_VAL_LOG		"LOG"
#define NRF_CLOUD_JSON_APPID_VAL_DICTIONARY_LOG	"DICTLOG"

/* Message type */
#define NRF_CLOUD_JSON_MSG_TYPE_KEY		"messageType"
#define NRF_CLOUD_JSON_MSG_TYPE_VAL_CMD		"CMD"
#define NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA	"DATA"
#define NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT	"DISCON"

/* Misc message keys */
#define NRF_CLOUD_JSON_DATA_KEY			"data"
#define NRF_CLOUD_JSON_ERR_KEY			"err"
#define NRF_CLOUD_JSON_FULFILL_KEY		"fulfilledWith"
#define NRF_CLOUD_JSON_FILTERED_KEY		"filtered"
#define NRF_CLOUD_MSG_TIMESTAMP_KEY		"ts"

/* Modem info key text */
#define NRF_CLOUD_JSON_MCC_KEY			"mcc"
#define NRF_CLOUD_JSON_MNC_KEY			"mnc"
#define NRF_CLOUD_JSON_AREA_CODE_KEY		"tac"
#define NRF_CLOUD_JSON_CELL_ID_KEY		"eci"
#define NRF_CLOUD_JSON_PHYCID_KEY		"phycid"
#define NRF_CLOUD_JSON_RSRP_KEY			"rsrp"

/* Cellular positioning */
#define NRF_CLOUD_CELL_POS_JSON_KEY_LTE		"lte"
#define NRF_CLOUD_CELL_POS_JSON_KEY_ECI		NRF_CLOUD_JSON_CELL_ID_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_MCC		NRF_CLOUD_JSON_MCC_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_MNC		NRF_CLOUD_JSON_MNC_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_TAC		NRF_CLOUD_JSON_AREA_CODE_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_AGE		"age"
#define NRF_CLOUD_CELL_POS_JSON_KEY_T_ADV	"adv"
#define NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN	"earfcn"
#define NRF_CLOUD_CELL_POS_JSON_KEY_PCI		"pci"
#define NRF_CLOUD_CELL_POS_JSON_KEY_NBORS	"nmr"
#define NRF_CLOUD_CELL_POS_JSON_KEY_RSRP	NRF_CLOUD_JSON_RSRP_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ	"rsrq"
#define NRF_CLOUD_CELL_POS_JSON_KEY_TDIFF	"timeDiff"

/* Location */
#define NRF_CLOUD_LOCATION_KEY_DOREPLY		"doReply"
#define NRF_CLOUD_LOCATION_JSON_KEY_WIFI	"wifi"
#define NRF_CLOUD_LOCATION_JSON_KEY_APS		"accessPoints"
#define NRF_CLOUD_LOCATION_JSON_KEY_WIFI_MAC	"macAddress"
#define NRF_CLOUD_LOCATION_JSON_KEY_WIFI_CH	"channel"
#define NRF_CLOUD_LOCATION_JSON_KEY_WIFI_RSSI	"signalStrength"
#define NRF_CLOUD_LOCATION_JSON_KEY_WIFI_SSID	"ssid"
#define NRF_CLOUD_LOCATION_JSON_KEY_LAT		"lat"
#define NRF_CLOUD_LOCATION_JSON_KEY_LON		"lon"
#define NRF_CLOUD_LOCATION_JSON_KEY_UNCERT	"uncertainty"
#define NRF_CLOUD_LOCATION_TYPE_VAL_MCELL	"MCELL"
#define NRF_CLOUD_LOCATION_TYPE_VAL_SCELL	"SCELL"
#define NRF_CLOUD_LOCATION_TYPE_VAL_WIFI	"WIFI"

/* P-GPS */
#define NRF_CLOUD_JSON_PGPS_PRED_COUNT		"predictionCount"
#define NRF_CLOUD_JSON_PGPS_INT_MIN		"predictionIntervalMinutes"
#define NRF_CLOUD_JSON_PGPS_GPS_DAY		"startGpsDay"
#define NRF_CLOUD_JSON_PGPS_GPS_TIME		"startGpsTimeOfDaySeconds"
#define NRF_CLOUD_PGPS_RCV_ARRAY_IDX_HOST	0
#define NRF_CLOUD_PGPS_RCV_ARRAY_IDX_PATH	1
#define NRF_CLOUD_PGPS_RCV_REST_HOST		"host"
#define NRF_CLOUD_PGPS_RCV_REST_PATH		"path"

/* A-GPS */
#define NRF_CLOUD_JSON_KEY_ELEVATION_MASK	"mask"
#define NRF_CLOUD_JSON_KEY_AGPS_TYPES		"types"

/* FOTA */
#define NRF_CLOUD_FOTA_TYPE_MODEM_DELTA		"MODEM"
#define NRF_CLOUD_FOTA_TYPE_MODEM_FULL		"MDM_FULL"
#define NRF_CLOUD_FOTA_TYPE_BOOT		"BOOT"
#define NRF_CLOUD_FOTA_TYPE_APP			"APP"
#define NRF_CLOUD_FOTA_REST_KEY_JOB_DOC		"jobDocument"
#define NRF_CLOUD_FOTA_REST_KEY_JOB_ID		"jobId"
#define NRF_CLOUD_FOTA_REST_KEY_PATH		"path"
#define NRF_CLOUD_FOTA_REST_KEY_HOST		"host"
#define NRF_CLOUD_FOTA_REST_KEY_TYPE		"firmwareType"
#define NRF_CLOUD_FOTA_REST_KEY_SIZE		"fileSize"
#define NRF_CLOUD_FOTA_REST_KEY_VER		"version"
/** Current FOTA version number */
#define NRF_CLOUD_FOTA_VER			2

/* REST */
#define NRF_CLOUD_REST_ERROR_CODE_KEY		"code"
#define NRF_CLOUD_REST_ERROR_MSG_KEY		"message"
#define NRF_CLOUD_REST_TOPIC_KEY		"topic"
#define NRF_CLOUD_REST_MSG_KEY			"message"

/* GNSS - PVT */
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_LAT		"lat"
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_LON		"lng"
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_ACCURACY	"acc"
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_ALTITUDE	"alt"
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_SPEED	"spd"
#define NRF_CLOUD_JSON_GNSS_PVT_KEY_HEADING	"hdg"

/* Device Info */
#define NRF_CLOUD_DEVICE_JSON_KEY_NET_INF	"networkInfo"
#define NRF_CLOUD_DEVICE_JSON_KEY_SIM_INF	"simInfo"
#define NRF_CLOUD_DEVICE_JSON_KEY_DEV_INF	"deviceInfo"

/* Alerts */
#define NRF_CLOUD_JSON_ALERT_SEQUENCE		"seq"
#define NRF_CLOUD_JSON_ALERT_DESCRIPTION	"desc"
#define NRF_CLOUD_JSON_ALERT_TYPE		"type"
#define NRF_CLOUD_JSON_ALERT_VALUE		"value"

/* Logs */
#define NRF_CLOUD_JSON_LOG_KEY_SEQUENCE		"seq"
#define NRF_CLOUD_JSON_LOG_KEY_DOMAIN		"dom"
#define NRF_CLOUD_JSON_LOG_KEY_SOURCE		"src"
#define NRF_CLOUD_JSON_LOG_KEY_LEVEL		"lvl"
#define NRF_CLOUD_JSON_LOG_KEY_MESSAGE		"msg"

/* Settings Module */
/** nRF Cloud's string identifier for persistent settings */
#define NRF_CLOUD_SETTINGS_NAME			"nrf_cloud"
#define NRF_CLOUD_SETTINGS_FOTA_KEY		"fota"
#define NRF_CLOUD_SETTINGS_FOTA_JOB		"job"
/** String used when defining a settings handler for FOTA */
#define NRF_CLOUD_SETTINGS_FULL_FOTA		NRF_CLOUD_SETTINGS_NAME \
						"/" \
						NRF_CLOUD_SETTINGS_FOTA_KEY
/** String used when saving FOTA job info to settings */
#define NRF_CLOUD_SETTINGS_FULL_FOTA_JOB	NRF_CLOUD_SETTINGS_FULL_FOTA \
						"/" \
						NRF_CLOUD_SETTINGS_FOTA_JOB

/* Shadow */
#define NRF_CLOUD_JSON_KEY_STATE		"state"
#define NRF_CLOUD_JSON_KEY_REP			"reported"
#define NRF_CLOUD_JSON_KEY_DES			"desired"
#define NRF_CLOUD_JSON_KEY_DELTA		"delta"
#define NRF_CLOUD_JSON_KEY_DEVICE		"device"
#define NRF_CLOUD_JSON_KEY_SRVC_INFO		"serviceInfo"
#define NRF_CLOUD_JSON_KEY_SRVC_INFO_UI		"ui"
#define NRF_CLOUD_JSON_KEY_SRVC_INFO_FOTA	NRF_CLOUD_FOTA_VER_STR
#define NRF_CLOUD_JSON_KEY_CFG			"config"
#define NRF_CLOUD_JSON_KEY_CTRL			"control"
#define NRF_CLOUD_JSON_KEY_ALERT		"alertsEn"
#define NRF_CLOUD_JSON_KEY_LOG			"logLvl"
#define NRF_CLOUD_JSON_KEY_TOPICS		"topics"
#define NRF_CLOUD_JSON_KEY_STAGE		"stage"
#define NRF_CLOUD_JSON_KEY_PAIRING		"pairing"
#define NRF_CLOUD_JSON_KEY_PAIR_STAT		"pairingStatus"
#define NRF_CLOUD_JSON_KEY_TOPIC_PRFX		"nrfcloud_mqtt_topic_prefix"
#define NRF_CLOUD_JSON_KEY_KEEPALIVE		"keepalive"
#define NRF_CLOUD_JSON_KEY_CONN			"connection"
#define NRF_CLOUD_JSON_KEY_APP_VER		"appVersion"
#define NRF_CLOUD_JSON_VAL_NOT_ASSOC		"not_associated"
#define NRF_CLOUD_JSON_VAL_PAIRED		"paired"
/* Current FOTA version string used in device shadow */
#define NRF_CLOUD_FOTA_VER_STR			"fota_v" STRINGIFY(NRF_CLOUD_FOTA_VER)
/* Max length of nRF Cloud's stage/environment name */
#define NRF_CLOUD_STAGE_ID_MAX_LEN		8
/** Max length of a tenant ID on nRF Cloud */
#define NRF_CLOUD_TENANT_ID_MAX_LEN		64
/** Max length of a device's MQTT client ID (device ID) on nRF Cloud*/
#define NRF_CLOUD_CLIENT_ID_MAX_LEN		64

/* Topics */
#ifndef CONFIG_NRF_CLOUD_GATEWAY
#define NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD	"d2c"
#define NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE	"c2d"
#else
#define NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD	"g2c"
#define NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE	"c2g"
#endif
#define NRF_CLOUD_BULK_MSG_TOPIC		"/bulk"
#define NRF_CLOUD_JSON_VAL_TOPIC_C2D		"/" NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE
#define NRF_CLOUD_JSON_VAL_TOPIC_AGPS		"/agps"
#define NRF_CLOUD_JSON_VAL_TOPIC_PGPS		"/pgps"
#define NRF_CLOUD_JSON_VAL_TOPIC_GND_FIX	"/ground_fix"
#define NRF_CLOUD_JSON_VAL_TOPIC_RCV		"/r"
#define NRF_CLOUD_JSON_VAL_TOPIC_WILDCARD	"/+"
#define NRF_CLOUD_JSON_VAL_TOPIC_BIN		"/bin"

/** @} */
#endif /* NRF_CLOUD_DEFS_H__ */
