/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DATA_GNSS_LONGITUDE "lng"
#define DATA_GNSS_LATITUDE  "lat"
#define DATA_GNSS_ALTITUDE  "alt"
#define DATA_GNSS_SPEED	   "spd"
#define DATA_GNSS_HEADING   "hdg"
#define DATA_GNSS_ACCURACY  "acc"

#define DATA_MODEM_DYNAMIC  "networkInfo"
#define DATA_MODEM_STATIC   "deviceInfo"
#define DATA_CONFIG	    "config"
#define DATA_VERSION	    "version"
#define DATA_IMPACT         "impact"

#define DATA_GROUP     "messageType"
#define DATA_ID	       "appId"
#define DATA_TYPE      "data"
#define DATA_TIMESTAMP "ts"

#define MESSAGE_TYPE_DATA "DATA"

#define APP_ID_BUTTON	   "BUTTON"
#define APP_ID_VOLTAGE	   "VOLTAGE"
#define APP_ID_DEVICE      "DEVICE"
#define APP_ID_GNSS	   "GNSS"
#define APP_ID_HUMIDITY	   "HUMID"
#define APP_ID_AIR_PRESS   "AIR_PRESS"
#define APP_ID_AIR_QUAL    "AIR_QUAL"
#define APP_ID_TEMPERATURE "TEMP"
#define APP_ID_RSRP	   "RSRP"
#define APP_ID_CELL_POS    "CELL_POS"
#define APP_ID_IMPACT      "IMPACT"

#define MODEM_CURRENT_BAND     "currentBand"
#define MODEM_NETWORK_MODE     "networkMode"
#define MODEM_RSRP	       "rsrp"
#define MODEM_AREA_CODE	       "areaCode"
#define MODEM_MCCMNC	       "mccmnc"
#define MODEM_CELL_ID	       "cellID"
#define MODEM_IP_ADDRESS       "ipAddress"

#define CONFIG_DEVICE_MODE		  "activeMode"
#define CONFIG_ACTIVE_TIMEOUT		  "activeWaitTime"
#define CONFIG_MOVE_TIMEOUT		  "movementTimeout"
#define CONFIG_MOVE_RES			  "movementResolution"
#define CONFIG_LOCATION_TIMEOUT		  "locationTimeout"
#define CONFIG_ACC_ACT_THRESHOLD	  "accThreshAct"
#define CONFIG_ACC_INACT_THRESHOLD	  "accThreshInact"
#define CONFIG_ACC_INACT_TIMEOUT	  "accTimeoutInact"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"
#define CONFIG_NO_DATA_LIST_WIFI	  "wifi"

#define OBJECT_CONFIG	"config"
#define OBJECT_REPORTED	"reported"
#define OBJECT_STATE	"state"
