/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_NRF_CLOUD)
#define DATA_MOVEMENT		"acc"
#define DATA_MOVEMENT_X		"x"
#define DATA_MOVEMENT_Y		"y"
#define DATA_MOVEMENT_Z		"z"

#define DATA_GPS		"gps"
#define DATA_GPS_LONGITUDE	"lng"
#define DATA_GPS_LATITUDE	"lat"
#define DATA_GPS_ALTITUDE	"alt"
#define DATA_GPS_SPEED		"spd"
#define DATA_GPS_HEADING	"hdg"

#define DATA_MODEM_DYNAMIC	"networkInfo"
#define DATA_MODEM_STATIC	"deviceInfo"
#define DATA_BATTERY		"batteryVoltage"
#define DATA_TEMPERATURE	"temp"
#define DATA_HUMID		"hum"
#define DATA_ENVIRONMENTALS	"env"
#define DATA_BUTTON		"btn"
#define DATA_CONFIG		"config"
#define DATA_VERSION		"version"

#define DATA_GROUP		"messageType"
#define DATA_ID			"appId"
#define DATA_TYPE		"data"
#define DATA_TIMESTAMP		"time"
#define DATA_VALUE		"data"

#define MESSAGE_TYPE_DATA	"DATA"

#define APP_ID_BUTTON		"BUTTON"
#define APP_ID_GPS		"GPS"
#define APP_ID_HUMIDITY		"HUMID"
#define APP_ID_TEMPERATURE	"TEMP"
#define APP_ID_RSRP		"RSRP"

#define MODEM_CURRENT_BAND	"currentBand"
#define MODEM_NETWORK_MODE	"networkMode"
#define MODEM_ICCID		"iccid"
#define MODEM_FIRMWARE_VERSION	"modemFirmware"
#define MODEM_BOARD		"board"
#define MODEM_APP_VERSION	"appVersion"
#define MODEM_RSRP		"rsrp"
#define MODEM_AREA_CODE		"areaCode"
#define MODEM_MCCMNC		"mccmnc"
#define MODEM_CELL_ID		"cellID"
#define MODEM_IP_ADDRESS	"ipAddress"

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

#define CONFIG_DEVICE_MODE		  "activeMode"
#define CONFIG_ACTIVE_TIMEOUT		  "activeWaitTime"
#define CONFIG_MOVE_TIMEOUT		  "movementTimeout"
#define CONFIG_MOVE_RES			  "movementResolution"
#define CONFIG_GPS_TIMEOUT		  "gpsTimeout"
#define CONFIG_ACC_THRESHOLD		  "movementThreshold"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"

#define OBJECT_CONFIG		"config"
#define OBJECT_REPORTED		"reported"
#define OBJECT_STATE		"state"
#define OBJECT_DATA		"data"
#define OBJECT_DEVICE		"device"

/* Definitions used to identify data that should be addressed to the message topic in nRF Cloud. */
#define OBJECT_MSG_HUMID	"MSG_HUMIDITY"
#define OBJECT_MSG_TEMP		"MSG_TEMPERATURE"
#define OBJECT_MSG_GPS		"MSG_GPS"
#define OBJECT_MSG_RSRP		"MSG_RSRP"

#else /* CONFIG_NRF_CLOUD */

#define MODEM_CURRENT_BAND	"band"
#define MODEM_NETWORK_MODE	"nw"
#define MODEM_ICCID		"iccid"
#define MODEM_FIRMWARE_VERSION	"modV"
#define MODEM_BOARD		"brdV"
#define MODEM_APP_VERSION	"appV"
#define MODEM_RSRP		"rsrp"
#define MODEM_AREA_CODE		"area"
#define MODEM_MCCMNC		"mccmnc"
#define MODEM_CELL_ID		"cell"
#define MODEM_IP_ADDRESS	"ip"

#define CONFIG_DEVICE_MODE		  "act"
#define CONFIG_ACTIVE_TIMEOUT		  "actwt"
#define CONFIG_MOVE_TIMEOUT		  "mvt"
#define CONFIG_MOVE_RES			  "mvres"
#define CONFIG_GPS_TIMEOUT		  "gpst"
#define CONFIG_ACC_THRESHOLD		  "acct"
#define CONFIG_NO_DATA_LIST		  "nod"
#define CONFIG_NO_DATA_LIST_GNSS	  "gnss"
#define CONFIG_NO_DATA_LIST_NEIGHBOR_CELL "ncell"

#define DATA_VALUE		"v"
#define DATA_TIMESTAMP		"ts"

#define DATA_MODEM_DYNAMIC	"roam"
#define DATA_MODEM_STATIC	"dev"
#define DATA_BATTERY		"bat"
#define DATA_TEMPERATURE	"temp"
#define DATA_HUMID		"hum"
#define DATA_ENVIRONMENTALS	"env"
#define DATA_BUTTON		"btn"
#define DATA_CONFIG		"cfg"
#define DATA_VERSION		"version"

#define DATA_MOVEMENT		"acc"
#define DATA_MOVEMENT_X		"x"
#define DATA_MOVEMENT_Y		"y"
#define DATA_MOVEMENT_Z		"z"

#define DATA_GPS		"gps"
#define DATA_GPS_LONGITUDE	"lng"
#define DATA_GPS_LATITUDE	"lat"
#define DATA_GPS_ALTITUDE	"alt"
#define DATA_GPS_SPEED		"spd"
#define DATA_GPS_HEADING	"hdg"
#define DATA_GPS_NMEA		"nmea"

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

#define OBJECT_CONFIG		"cfg"

/* The only difference between AWS IoT and Azure IoT Hub is the shadow/device twin object
 * naming.
 */
#if defined(CONFIG_AWS_IOT)
#define OBJECT_REPORTED		"reported"
#define OBJECT_STATE		"state"
#elif defined(CONFIG_AZURE_IOT_HUB)
#define OBJECT_DESIRED		"desired"
#define OBJECT_CONFIG		"cfg"
#endif

#endif
