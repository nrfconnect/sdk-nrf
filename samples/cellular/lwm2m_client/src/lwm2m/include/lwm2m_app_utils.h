/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/drivers/sensor.h>

#ifndef LWM2M_APP_UTILS_H__
#define LWM2M_APP_UTILS_H__

/* LwM2M Object IDs */
#define LWM2M_OBJECT_SECURITY_ID 0
#define LWM2M_OBJECT_SERVER_ID 1

#define LWM2M_OBJECT_DEVICE_ID 3

/* IPSO Object IDs */
#define IPSO_OBJECT_COLOUR_ID 3335

/* Server RIDs */
#define LIFETIME_RID 1

/* Device RIDs */
#define MANUFACTURER_RID 0
#define MODEL_NUMBER_RID 1
#define SERIAL_NUMBER_RID 2
#define FACTORY_RESET_RID 5
#define POWER_SOURCE_RID 6
#define POWER_SOURCE_VOLTAGE_RID 7
#define POWER_SOURCE_CURRENT_RID 8
#define BATTERY_LEVEL_RID 9
#define CURRENT_TIME_RID 13
#define UTC_OFFSET_RID 14
#define TIMEZONE_RID 15
#define DEVICE_TYPE_RID 17
#define HARDWARE_VERSION_RID 18
#define SOFTWARE_VERSION_RID 19
#define BATTERY_STATUS_RID 20
#define MEMORY_TOTAL_RID 21

/* Location RIDs */
#define LATITUDE_RID 0
#define LONGITUDE_RID 1
#define ALTITUDE_RID 2
#define LOCATION_RADIUS_RID 3
#define LOCATION_VELOCITY_RID 4
#define LOCATION_TIMESTAMP_RID 5
#define LOCATION_SPEED_RID 6

/* Misc */
#define LWM2M_RES_DATA_FLAG_RW 0 /* LwM2M Resource read-write flag */
#define MAX_LWM2M_PATH_LEN 20 /* Maximum string length of an LwM2M path */
#define RGBIR_STR_LENGTH 11 /* String length of RGB-IR colour string: '0xRRGGBBIR\0' */

#ifdef __cplusplus
extern "C" {
#endif

/* Set timestamp resource */
void set_ipso_obj_timestamp(int ipso_obj_id, unsigned int obj_inst_id);

/* Check whether notification read callback or regular read callback */
bool is_regular_read_cb(int64_t read_timestamp);

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_APP_UTILS_H__ */
