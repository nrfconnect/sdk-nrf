/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @defgroup cloud_codec Cloud codec
 * @brief  Module that encodes and decodes data destined for a cloud solution.
 * @{
 */

#ifndef CLOUD_CODEC_H__
#define CLOUD_CODEC_H__

#include <net/cloud.h>
#include "env_sensors.h"
#include "motion.h"
#include "light_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Cloud sensor types. */
enum cloud_channel {
	/** The GPS sensor on the device. */
	CLOUD_CHANNEL_GPS,
	/** The FLIP movement sensor on the device. */
	CLOUD_CHANNEL_FLIP,
	/** The IMPACT movement sensor on the device. */
	CLOUD_CHANNEL_IMPACT,
	/** The Button press sensor on the device. */
	CLOUD_CHANNEL_BUTTON,
	/** The PIN unit on the device. */
	CLOUD_CHANNEL_PIN,
	/** The RGB LED on the device. */
	CLOUD_CHANNEL_RGB_LED,
	/** The BUZZER on the device. */
	CLOUD_CHANNEL_BUZZER,
	/** The TEMP sensor on the device. */
	CLOUD_CHANNEL_TEMP,
	/** The Humidity sensor on the device. */
	CLOUD_CHANNEL_HUMID,
	/** The Air Pressure sensor on the device. */
	CLOUD_CHANNEL_AIR_PRESS,
	/** The Air Quality sensor on the device. */
	CLOUD_CHANNEL_AIR_QUAL,
	/** The RSPR data obtained from the modem. */
	CLOUD_CHANNEL_LTE_LINK_RSRP,
	/** The descriptive DEVICE data indicating its status. */
	CLOUD_CHANNEL_DEVICE_INFO,
	/** The RBG IR light levels on the device. */
	CLOUD_CHANNEL_LIGHT_SENSOR,
	/** The red light level on the device. */
	CLOUD_CHANNEL_LIGHT_RED,
	/** The green light level on the device. */
	CLOUD_CHANNEL_LIGHT_GREEN,
	/** The blue light level on the device. */
	CLOUD_CHANNEL_LIGHT_BLUE,
	/** The IR light level on the device. */
	CLOUD_CHANNEL_LIGHT_IR,
	/** The assisted GPS channel. */
	CLOUD_CHANNEL_ASSISTED_GPS,

	CLOUD_CHANNEL__TOTAL
};

#define CLOUD_CHANNEL_STR_GPS "GPS"
#define CLOUD_CHANNEL_STR_FLIP "FLIP"
#define CLOUD_CHANNEL_STR_BUTTON "BUTTON"
#define CLOUD_CHANNEL_STR_TEMP "TEMP"
#define CLOUD_CHANNEL_STR_HUMID "HUMID"
#define CLOUD_CHANNEL_STR_AIR_PRESS "AIR_PRESS"
#define CLOUD_CHANNEL_STR_AIR_QUAL "AIR_QUAL"
#define CLOUD_CHANNEL_STR_LTE_LINK_RSRP "RSRP"
/* The "device" is intended for the shadow, which expects its objects
 * to have lowercase keys.
 */
#define CLOUD_CHANNEL_STR_DEVICE_INFO "device"
#define CLOUD_CHANNEL_STR_LIGHT_SENSOR "LIGHT"
#define CLOUD_CHANNEL_STR_LIGHT_RED "LIGHT_RED"
#define CLOUD_CHANNEL_STR_LIGHT_GREEN "LIGHT_GREEN"
#define CLOUD_CHANNEL_STR_LIGHT_BLUE "LIGHT_BLUE"
#define CLOUD_CHANNEL_STR_LIGHT_IR "LIGHT_IR"
#define CLOUD_CHANNEL_STR_ASSISTED_GPS "AGPS"
#define CLOUD_CHANNEL_STR_RGB_LED "LED"

struct cloud_data {
	char *buf;
	size_t len;
};

/**@brief Sensor data parameters. */
struct cloud_channel_data {
	/** The sensor that is the source of the data. */
	enum cloud_channel type;
	/** Sensor data to be transmitted. */
	struct cloud_data data;
	/** Unique tag to identify the sent data.
	 *  Useful for matching the acknowledgment.
	 */
	u32_t tag;
};

enum cloud_cmd_group {
	CLOUD_CMD_GROUP_HELLO,
	CLOUD_CMD_GROUP_START,
	CLOUD_CMD_GROUP_STOP,
	CLOUD_CMD_GROUP_INIT,
	CLOUD_CMD_GROUP_GET,
	CLOUD_CMD_GROUP_STATUS,
	CLOUD_CMD_GROUP_DATA,
	CLOUD_CMD_GROUP_OK,
	CLOUD_CMD_GROUP_CFG_SET,
	CLOUD_CMD_GROUP_CFG_GET,

	CLOUD_CMD_GROUP__TOTAL
};

#define CLOUD_CMD_GROUP_STR_HELLO "HELLO"
#define CLOUD_CMD_GROUP_STR_START "START"
#define CLOUD_CMD_GROUP_STR_STOP "STOP"
#define CLOUD_CMD_GROUP_STR_INIT "INIT"
#define CLOUD_CMD_GROUP_STR_GET "GET"
#define CLOUD_CMD_GROUP_STR_STATUS "STATUS"
#define CLOUD_CMD_GROUP_STR_DATA "DATA"
#define CLOUD_CMD_GROUP_STR_OK "OK"
#define CLOUD_CMD_GROUP_STR_CFG_SET "CFG_SET"
#define CLOUD_CMD_GROUP_STR_CFG_GET "CFG_GET"

enum cloud_cmd_type {
	CLOUD_CMD_EMPTY,
	CLOUD_CMD_ENABLE,
	CLOUD_CMD_THRESHOLD_HIGH,
	CLOUD_CMD_THRESHOLD_LOW,
	CLOUD_CMD_INTERVAL,
	CLOUD_CMD_COLOR,
	CLOUD_CMD_MODEM_PARAM,

	CLOUD_CMD__TOTAL
};

enum cloud_cmd_state {
	CLOUD_CMD_STATE_UNDEFINED = -1,
	CLOUD_CMD_STATE_FALSE = 0,
	CLOUD_CMD_STATE_TRUE,
};

#define CLOUD_CMD_TYPE_STR_EMPTY "empty"
#define CLOUD_CMD_TYPE_STR_ENABLE "enable"
#define CLOUD_CMD_TYPE_STR_THRESH_LO "thresh_lo"
#define CLOUD_CMD_TYPE_STR_THRESH_HI "thresh_hi"
#define CLOUD_CMD_TYPE_STR_INTERVAL "interval"
#define CLOUD_CMD_TYPE_STR_COLOR "color"
#define CLOUD_CMD_TYPE_STR_MODEM_PARAM "modemParams"

#define MODEM_PARAM_BLOB_KEY_STR "blob"
#define MODEM_PARAM_CHECKSUM_KEY_STR "checksum"

struct cloud_command_state_value {
	double value; /* The value to be written to the recipient/channel. */
	/* The truth value to be written to the recipient/channel. */
	enum cloud_cmd_state state;
};

struct cloud_command_modem_params {
	/* TODO: add proper members when A-GPS is implemented */
	char *blob;
	char *checksum;
};

struct cloud_command {
	enum cloud_cmd_group group; /* The decoded command's group. */
	enum cloud_channel channel; /* The command's desired channel. */
	enum cloud_cmd_type type; /* The command type, the desired action. */
	union {
		struct cloud_command_state_value sv;
		struct cloud_command_modem_params mp;
	} data;
};

typedef void (*cloud_cmd_cb_t)(struct cloud_command *cmd);

/**
 * @brief Encode cloud data.
 *
 * @param channel The cloud channel type.
 * @param output Pointer to the cloud data output.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_encode_data(const struct cloud_channel_data *channel,
		      struct cloud_msg *output);

/**
 * @brief Decode cloud data.
 *
 * @param output Pointer to the cloud data input.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_decode_command(char const *input);

/**
 * @brief Init the cloud decoder.
 *
 * @param cb The callback handler to be used by the cloud decoder.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_decode_init(cloud_cmd_cb_t cb);

/**
 * @brief Encode data to be transmitted to the digital twin,
 *	  from sensor data structure to a cloud data structure
 *	  containing a JSON string.
 *
 * @param sensor Pointer to sensor data.
 * @param output Pointer to encoded data structure.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_encode_digital_twin_data(const struct cloud_channel_data *channel,
				   struct cloud_msg *output);

/**
 * @brief Releases memory used by cloud data structure.
 *
 * @param data Pointer to cloud data to be released.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
static inline void cloud_release_data(struct cloud_msg *data)
{
	k_free(data->buf);
}

int cloud_encode_env_sensors_data(const env_sensor_data_t *sensor_data,
				  struct cloud_msg *output);

int cloud_encode_motion_data(const motion_data_t *motion_data,
			     struct cloud_msg *output);

#if CONFIG_LIGHT_SENSOR
int cloud_encode_light_sensor_data(const struct light_sensor_data *sensor_data,
				   struct cloud_msg *output);
#endif /* CONFIG_LIGHT_SENSOR */

/**
 * @brief Checks if data could be sent to the cloud based on config.
 *
 * @param channel The cloud channel type..
 * @param value Current data value for channel.
 *
 * @return true If the data should be sent to the cloud.
 */
bool cloud_is_send_allowed(const enum cloud_channel channel,
			   const double value);
/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* CLOUD_CODEC_H__ */
