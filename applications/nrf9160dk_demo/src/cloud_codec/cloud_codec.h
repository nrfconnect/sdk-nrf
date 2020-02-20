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

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Cloud sensor types. */
enum cloud_channel {
	/** The Button press sensor on the device. */
	CLOUD_CHANNEL_BUTTON,
	/** The RGB LED or some simple LEDs on the device. */
	CLOUD_CHANNEL_LED,
	/** The descriptive DEVICE data indicating its status. */
	CLOUD_CHANNEL_DEVICE_INFO,
	/** The message channel. */
	CLOUD_CHANNEL_MSG,

	CLOUD_CHANNEL__TOTAL
};

#define CLOUD_CHANNEL_STR_BUTTON "BUTTON"
#define CLOUD_CHANNEL_STR_DEVICE_INFO "DEVICE"
#define CLOUD_CHANNEL_STR_LED "LED"
#define CLOUD_CHANNEL_STR_MSG "MSG"

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
	CLOUD_CMD_GROUP_COMMAND,

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
#define CLOUD_CMD_GROUP_STR_COMMAND "CMD"

enum cloud_cmd_type {
	CLOUD_CMD_EMPTY,
	CLOUD_CMD_ENABLE,
	CLOUD_CMD_THRESHOLD_HIGH,
	CLOUD_CMD_THRESHOLD_LOW,
	CLOUD_CMD_INTERVAL,
	CLOUD_CMD_COLOR,
	CLOUD_CMD_DATA_STRING,
	CLOUD_CMD_STATE,

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
#define CLOUD_CMD_TYPE_STR_DATA_STRING "data_string"
#define CLOUD_CMD_TYPE_STR_STATE "state"


#define MODEM_PARAM_BLOB_KEY_STR "blob"
#define MODEM_PARAM_CHECKSUM_KEY_STR "checksum"

struct cloud_command_state_value {
	double value; /* The value to be written to the recipient/channel. */
	/* The truth value to be written to the recipient/channel. */
	enum cloud_cmd_state state;
};

struct cloud_command {
	enum cloud_cmd_group group; /* The decoded command's group. */
	enum cloud_channel channel; /* The command's desired channel. */
	enum cloud_cmd_type type; /* The command type, the desired action. */
	union {
		struct cloud_command_state_value sv;
		char *data_string;
	} data;
};

typedef void (*cloud_cmd_cb_t)(struct cloud_command *cmd);

/**
 * @brief Encode cloud data.
 *
 * @param channel The cloud channel type.
 * @param group The channel data's group.
 * @param output Pointer to the cloud data output.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_encode_data(const struct cloud_channel_data *channel,
	const enum cloud_cmd_group group, struct cloud_msg *output);

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
 * @brief Encode device status data to be transmitted to the
 *        digital twin.
 *
 * @param modem_param Pointer to optional modem parameter data.
 * @param ui Pointer to array of cloud data channel name
 *           strings.
 * @param ui_count Size of the ui array.
 * @param fota Pointer to array of FOTA services.
 * @param fota_count Size of the fota array.
 * @param fota_version Version of the FOTA service.
 * @param output Pointer to encoded data structure.
 *
 * @return 0 if the operation was successful, otherwise a
 *         (negative) error code.
 */
int cloud_encode_device_status_data(
	void *modem_param,
	const char *const ui[], const u32_t ui_count,
	const char *const fota[], const u32_t fota_count,
	const u16_t fota_version,
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

/**
 *
 * @brief Get string representing cloud cmd group name
 *
 * @param group enum cloud_cmd_group value
 *
 * @return const char* name
 */
const char *cloud_get_group_name(const enum cloud_cmd_group group);

/**
 * @brief Get string representing cloud_channel value
 *
 * @param channel enum cloud_channel value
 *
 * @return const char* name
 */
const char *cloud_get_channel_name(const enum cloud_channel channel);

/**
 *
 * @brief Get string representing cloud cmd type
 *
 * @param type enum cloud_cmd_type value
 *
 * @return const char*
 */
const char *cloud_get_type_name(const enum cloud_cmd_type type);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* CLOUD_CODEC_H__ */
