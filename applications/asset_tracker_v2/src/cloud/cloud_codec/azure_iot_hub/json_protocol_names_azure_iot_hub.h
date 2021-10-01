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

#define CONFIG_DEVICE_MODE		  "act"
#define CONFIG_ACTIVE_TIMEOUT		  "actwt"
#define CONFIG_MOVE_TIMEOUT		  "mvt"
#define CONFIG_MOVE_RES			  "mvres"
#define CONFIG_GPS_TIMEOUT		  "gpst"
#define CONFIG_ACC_THRESHOLD		  "acct"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"

#define DATA_VALUE     "v"
#define DATA_TIMESTAMP "ts"

#define DATA_MODEM_DYNAMIC  "roam"
#define DATA_MODEM_STATIC   "dev"
#define DATA_BATTERY	    "bat"
#define DATA_TEMPERATURE    "temp"
#define DATA_HUMID	    "hum"
#define DATA_ENVIRONMENTALS "env"
#define DATA_BUTTON	    "btn"
#define DATA_CONFIG	    "cfg"
#define DATA_VERSION	    "version"

#define DATA_MOVEMENT   "acc"
#define DATA_MOVEMENT_X "x"
#define DATA_MOVEMENT_Y "y"
#define DATA_MOVEMENT_Z "z"

#define DATA_GPS	   "gps"
#define DATA_GPS_LONGITUDE "lng"
#define DATA_GPS_LATITUDE  "lat"
#define DATA_GPS_ALTITUDE  "alt"
#define DATA_GPS_SPEED	   "spd"
#define DATA_GPS_HEADING   "hdg"
#define DATA_GPS_ACCURACY  "acc"
#define DATA_GPS_NMEA	   "nmea"

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

#define OBJECT_DESIRED "desired"
#define OBJECT_CONFIG  "cfg"
