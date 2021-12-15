/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DATA_MOVEMENT	"acc"
#define DATA_MOVEMENT_X	"x"
#define DATA_MOVEMENT_Y	"y"
#define DATA_MOVEMENT_Z	"z"

#define DATA_GNSS	   "gnss"
#define DATA_GNSS_LONGITUDE "lng"
#define DATA_GNSS_LATITUDE  "lat"
#define DATA_GNSS_ALTITUDE  "alt"
#define DATA_GNSS_SPEED	   "spd"
#define DATA_GNSS_HEADING   "hdg"
#define DATA_GNSS_ACCURACY  "acc"

#define DATA_MODEM_DYNAMIC  "networkInfo"
#define DATA_MODEM_STATIC   "deviceInfo"
#define DATA_BATTERY	    "batteryVoltage"
#define DATA_TEMPERATURE    "temp"
#define DATA_HUMID	    "hum"
#define DATA_ENVIRONMENTALS "env"
#define DATA_BUTTON	    "btn"
#define DATA_CONFIG	    "config"
#define DATA_VERSION	    "version"

#define DATA_GROUP     "messageType"
#define DATA_ID	       "appId"
#define DATA_TYPE      "data"
#define DATA_TIMESTAMP "ts"
#define DATA_VALUE     "data"

#define MESSAGE_TYPE_DATA "DATA"

#define APP_ID_BUTTON	   "BUTTON"
#define APP_ID_VOLTAGE	   "VOLTAGE"
#define APP_ID_DEVICE      "DEVICE"
#define APP_ID_GPS	   "GPS"
#define APP_ID_HUMIDITY	   "HUMID"
#define APP_ID_TEMPERATURE "TEMP"
#define APP_ID_RSRP	   "RSRP"
#define APP_ID_CELL_POS    "CELL_POS"

#define MODEM_CURRENT_BAND     "currentBand"
#define MODEM_NETWORK_MODE     "networkMode"
#define MODEM_ICCID	       "iccid"
#define MODEM_FIRMWARE_VERSION "modemFirmware"
#define MODEM_BOARD	       "board"
#define MODEM_APP_VERSION      "appVersion"
#define MODEM_RSRP	       "rsrp"
#define MODEM_AREA_CODE	       "areaCode"
#define MODEM_MCCMNC	       "mccmnc"
#define MODEM_CELL_ID	       "cellID"
#define MODEM_IP_ADDRESS       "ipAddress"

#define DATA_NEIGHBOR_CELLS_LTE           "lte"
#define DATA_NEIGHBOR_CELLS_MCC		  "mcc"
#define DATA_NEIGHBOR_CELLS_MNC		  "mnc"
#define DATA_NEIGHBOR_CELLS_CID		  "eci"
#define DATA_NEIGHBOR_CELLS_TAC		  "tac"
#define DATA_NEIGHBOR_CELLS_EARFCN	  "earfcn"
#define DATA_NEIGHBOR_CELLS_TIMING	  "adv"
#define DATA_NEIGHBOR_CELLS_RSRP	  "rsrp"
#define DATA_NEIGHBOR_CELLS_RSRQ	  "rsrq"
#define DATA_NEIGHBOR_CELLS_NEIGHBOR_MEAS "nmr"
#define DATA_NEIGHBOR_CELLS_PCI		  "pci"

#define CONFIG_DEVICE_MODE		  "activeMode"
#define CONFIG_ACTIVE_TIMEOUT		  "activeWaitTime"
#define CONFIG_MOVE_TIMEOUT		  "movementTimeout"
#define CONFIG_MOVE_RES			  "movementResolution"
#define CONFIG_GNSS_TIMEOUT		  "gnssTimeout"
#define CONFIG_ACC_THRESHOLD		  "movementThreshold"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"

#define DATA_AGPS_REQUEST_MCC   "mcc"
#define DATA_AGPS_REQUEST_MNC   "mnc"
#define DATA_AGPS_REQUEST_CID   "cell"
#define DATA_AGPS_REQUEST_TAC   "area"
#define DATA_AGPS_REQUEST_TYPES "types"

#define DATA_AGPS_REQUEST_TYPE_UTC_PARAMETERS		 1
#define DATA_AGPS_REQUEST_TYPE_EPHEMERIDES		 2
#define DATA_AGPS_REQUEST_TYPE_ALMANAC			 3
#define DATA_AGPS_REQUEST_TYPE_KLOBUCHAR_CORRECTION	 4
#define DATA_AGPS_REQUEST_TYPE_NEQUICK_CORRECTION	 5
#define DATA_AGPS_REQUEST_TYPE_GPS_TOWS			 6
#define DATA_AGPS_REQUEST_TYPE_GPS_SYSTEM_CLOCK_AND_TOWS 7
#define DATA_AGPS_REQUEST_TYPE_LOCATION			 8
#define DATA_AGPS_REQUEST_TYPE_INTEGRITY		 9

#define DATA_PGPS_REQUEST_COUNT    "n"
#define DATA_PGPS_REQUEST_INTERVAL "int"
#define DATA_PGPS_REQUEST_DAY      "day"
#define DATA_PGPS_REQUEST_TIME     "time"

#define OBJECT_CONFIG	"config"
#define OBJECT_REPORTED	"reported"
#define OBJECT_STATE	"state"
#define OBJECT_DATA	"data"
#define OBJECT_DEVICE	"device"

#define OBJECT_MSG_HUMID "MSG_HUMIDITY"
#define OBJECT_MSG_TEMP	 "MSG_TEMPERATURE"
#define OBJECT_MSG_GNSS	 "MSG_GNSS"
#define OBJECT_MSG_RSRP	 "MSG_RSRP"
