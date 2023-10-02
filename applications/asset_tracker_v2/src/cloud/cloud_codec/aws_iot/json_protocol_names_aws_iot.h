/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODEM_CURRENT_BAND     "band"
#define MODEM_NETWORK_MODE     "nw"
#define MODEM_ICCID	       "iccid"
#define MODEM_FIRMWARE_VERSION "modV"
#define MODEM_BOARD	       "brdV"
#define MODEM_APP_VERSION      "appV"
#define MODEM_RSRP	       "rsrp"
#define MODEM_AREA_CODE	       "area"
#define MODEM_MCCMNC	       "mccmnc"
#define MODEM_CELL_ID	       "cell"
#define MODEM_IP_ADDRESS       "ip"
#define MODEM_IMEI             "imei"

#define CONFIG_DEVICE_MODE		  "act"
#define CONFIG_ACTIVE_TIMEOUT		  "actwt"
#define CONFIG_MOVE_TIMEOUT		  "mvt"
#define CONFIG_MOVE_RES			  "mvres"
#define CONFIG_LOCATION_TIMEOUT		  "loct"
#define CONFIG_ACC_ACT_THRESHOLD	  "accath"
#define CONFIG_ACC_INACT_THRESHOLD	  "accith"
#define CONFIG_ACC_INACT_TIMEOUT	  "accito"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"
#define CONFIG_NO_DATA_LIST_WIFI	  "wifi"

#define DATA_VALUE     "v"
#define DATA_TIMESTAMP "ts"

#define DATA_MODEM_DYNAMIC  "roam"
#define DATA_MODEM_STATIC   "dev"
#define DATA_BATTERY	    "bat"
#define DATA_TEMPERATURE    "temp"
#define DATA_HUMIDITY	    "hum"
#define DATA_PRESSURE       "atmp"
#define DATA_BSEC_IAQ       "bsec_iaq"
#define DATA_ENVIRONMENTALS "env"
#define DATA_BUTTON	    "btn"
#define DATA_CONFIG	    "cfg"
#define DATA_VERSION	    "version"
#define DATA_IMPACT	    "impact"

#define DATA_MOVEMENT   "acc"
#define DATA_MOVEMENT_X "x"
#define DATA_MOVEMENT_Y "y"
#define DATA_MOVEMENT_Z "z"

#define DATA_GNSS	    "gnss"
#define DATA_GNSS_LONGITUDE "lng"
#define DATA_GNSS_LATITUDE  "lat"
#define DATA_GNSS_ALTITUDE  "alt"
#define DATA_GNSS_SPEED	    "spd"
#define DATA_GNSS_HEADING   "hdg"
#define DATA_GNSS_ACCURACY  "acc"

#define DATA_NEIGHBOR_CELLS_ROOT	  "lte"
#define DATA_NEIGHBOR_CELLS_MCC		  "mcc"
#define DATA_NEIGHBOR_CELLS_MNC		  "mnc"
#define DATA_NEIGHBOR_CELLS_CID		  "cell"
#define DATA_NEIGHBOR_CELLS_TAC		  "area"
#define DATA_NEIGHBOR_CELLS_EARFCN	  "earfcn"
#define DATA_NEIGHBOR_CELLS_TIMING	  "adv"
#define DATA_NEIGHBOR_CELLS_RSRP	  "rsrp"
#define DATA_NEIGHBOR_CELLS_RSRQ	  "rsrq"
#define DATA_NEIGHBOR_CELLS_NEIGHBOR_MEAS "nmr"
#define DATA_NEIGHBOR_CELLS_PCI		  "cell"
#define DATA_WIFI_ROOT			  "wifi"
#define DATA_WIFI_AP_MEAS		  "aps"

#define DATA_AGNSS_REQUEST_MCC   "mcc"
#define DATA_AGNSS_REQUEST_MNC   "mnc"
#define DATA_AGNSS_REQUEST_CID   "cell"
#define DATA_AGNSS_REQUEST_TAC   "area"
#define DATA_AGNSS_REQUEST_TYPES "types"

#define DATA_AGNSS_REQUEST_TYPE_GPS_UTC_PARAMETERS	  1
#define DATA_AGNSS_REQUEST_TYPE_GPS_EPHEMERIDES		  2
#define DATA_AGNSS_REQUEST_TYPE_GPS_ALMANAC		  3
#define DATA_AGNSS_REQUEST_TYPE_KLOBUCHAR_CORRECTION	  4
#define DATA_AGNSS_REQUEST_TYPE_NEQUICK_CORRECTION	  5
#define DATA_AGNSS_REQUEST_TYPE_GPS_TOWS		  6
#define DATA_AGNSS_REQUEST_TYPE_GPS_SYSTEM_CLOCK_AND_TOWS 7
#define DATA_AGNSS_REQUEST_TYPE_LOCATION		  8
#define DATA_AGNSS_REQUEST_TYPE_GPS_INTEGRITY		  9

#define DATA_PGPS_REQUEST_COUNT    "n"
#define DATA_PGPS_REQUEST_INTERVAL "int"
#define DATA_PGPS_REQUEST_DAY      "day"
#define DATA_PGPS_REQUEST_TIME     "time"

#define OBJECT_CONFIG	"cfg"
#define OBJECT_REPORTED	"reported"
#define OBJECT_STATE	"state"
