/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <net/cloud.h>

#include "cJSON.h"
#include "cJSON_os.h"
#include "cloud_codec.h"

#include <logging/log.h>
#include "env_sensors.h"

struct cmd {
	char *name;
	union {
		enum cloud_cmd_group group;
		enum cloud_cmd_recipient recipient;
		enum cloud_channel channel;
		enum cloud_cmd_type type;
	};

	struct cmd *children;
	size_t num_children;
};

#define CMD_ARRAY(...) ((struct cmd[]) {__VA_ARGS__})

#define CMD_NEW_TYPE(_name, _type)			\
	{ 						\
		.name = STRINGIFY(_name),		\
		.type = _type, 				\
	}

#define CMD_NEW_CHAN(_name, _chan, _children)		\
	{						\
		.name = STRINGIFY(_name),		\
		.channel = _chan,			\
		.children = _children,			\
		.num_children = ARRAY_SIZE(_children),	\
	}

#define CMD_NEW_RECIPIENT(_name, _recipient, _children)	\
	{						\
		.name = STRINGIFY(_name),		\
		.recipient = _recipient,		\
		.children = _children,			\
		.num_children = ARRAY_SIZE(_children),	\
	}

#define CMD_NEW_GROUP(_name, _group, _children)		\
	struct cmd _name = {				\
		.name = STRINGIFY(_name),		\
		.group = _group,			\
		.children = _children,			\
		.num_children = ARRAY_SIZE(_children),	\
	};


static CMD_NEW_GROUP(group_set, CLOUD_CMD_GROUP_SET, CMD_ARRAY(
	CMD_NEW_RECIPIENT(environment, CLOUD_RCPT_ENVIRONMENT, CMD_ARRAY(
		CMD_NEW_CHAN(humidity, CLOUD_CHANNEL_HUMID, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(pressure, CLOUD_CHANNEL_AIR_PRESS, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(temperature, CLOUD_CHANNEL_TEMP, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(air_quality, CLOUD_CHANNEL_AIR_QUAL, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		))
	),
	CMD_NEW_RECIPIENT(motion, CLOUD_RCPT_MOTION, CMD_ARRAY(
		CMD_NEW_CHAN(flip, CLOUD_CHANNEL_FLIP, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(impact, CLOUD_CHANNEL_IMPACT, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		))
	),
	CMD_NEW_RECIPIENT(ui, CLOUD_RCPT_UI, CMD_ARRAY(
		CMD_NEW_CHAN(pin, CLOUD_CHANNEL_PIN, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(pwm, CLOUD_CMD_PWM))
		),
		CMD_NEW_CHAN(led, CLOUD_CHANNEL_RGB_LED, CMD_ARRAY(
			CMD_NEW_TYPE(red, CLOUD_CMD_LED_RED),
			CMD_NEW_TYPE(green, CLOUD_CMD_LED_GREEN),
			CMD_NEW_TYPE(blue, CLOUD_CMD_LED_BLUE),
			CMD_NEW_TYPE(pulse_length, CLOUD_CMD_LED_PULSE_LENGTH),
			CMD_NEW_TYPE(pause, CLOUD_CMD_LED_PAUSE_LENGTH))
		),
		CMD_NEW_CHAN(buzzer, CLOUD_CHANNEL_BUZZER, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(disable, CLOUD_CMD_DISABLE),
			CMD_NEW_TYPE(play_melody, CLOUD_CMD_PLAY_MELODY),
			CMD_NEW_TYPE(play_note, CLOUD_CMD_PLAY_NOTE))
		))
	))
);

static CMD_NEW_GROUP(group_get, CLOUD_CMD_GROUP_GET, CMD_ARRAY(
	CMD_NEW_RECIPIENT(environment, CLOUD_RCPT_ENVIRONMENT, CMD_ARRAY(
		CMD_NEW_CHAN(humidity, CLOUD_CHANNEL_HUMID, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(pressure, CLOUD_CHANNEL_AIR_PRESS, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(temperature, CLOUD_CHANNEL_TEMP, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(air_quality, CLOUD_CHANNEL_AIR_QUAL, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		))
	),
	CMD_NEW_RECIPIENT(modem_info, CLOUD_RCPT_MODEM_INFO, CMD_ARRAY(
		CMD_NEW_CHAN(device, CLOUD_CHANNEL_DEVICE_INFO, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_new, CLOUD_CMD_READ_NEW))
		),
		CMD_NEW_CHAN(rsrp, CLOUD_CHANNEL_LTE_LINK_RSRP, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		))
	),
	CMD_NEW_RECIPIENT(motion, CLOUD_RCPT_MOTION, CMD_ARRAY(
		CMD_NEW_CHAN(flip, CLOUD_CHANNEL_FLIP, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(impact, CLOUD_CHANNEL_IMPACT, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		))
	),
	CMD_NEW_RECIPIENT(ui, CLOUD_RCPT_UI, CMD_ARRAY(
		CMD_NEW_CHAN(pin, CLOUD_CHANNEL_PIN, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(led, CLOUD_CHANNEL_RGB_LED, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		),
		CMD_NEW_CHAN(buzzer, CLOUD_CHANNEL_BUZZER, CMD_ARRAY(
			CMD_NEW_TYPE(read, CLOUD_CMD_READ),
			CMD_NEW_TYPE(read_all, CLOUD_CMD_READ))
		))
	))
);

static const char *const channel_type_str[] = {
	[CLOUD_CHANNEL_GPS] = "GPS",
	[CLOUD_CHANNEL_FLIP] = "FLIP",
	[CLOUD_CHANNEL_BUTTON] = "BUTTON",
	[CLOUD_CHANNEL_TEMP] = "TEMP",
	[CLOUD_CHANNEL_HUMID] = "HUMID",
	[CLOUD_CHANNEL_AIR_PRESS] = "AIR_PRESS",
	[CLOUD_CHANNEL_AIR_QUAL] = "AIR_QUAL",
	[CLOUD_CHANNEL_LTE_LINK_RSRP] = "RSRP",
	[CLOUD_CHANNEL_DEVICE_INFO] = "DEVICE",
};

static cloud_cmd_cb_t cloud_command_cb;
struct cloud_command cmd_parsed;

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

int cloud_encode_data(const struct cloud_channel_data *channel,
		      struct cloud_msg *output)
{
	int ret;

	if (channel == NULL || channel->data.buf == NULL ||
	    channel->data.len == 0 || output == NULL) {
		return -EINVAL;
	}

	cJSON *root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	ret = json_add_str(root_obj, "appId", channel_type_str[channel->type]);
	ret += json_add_str(root_obj, "data", channel->data.buf);
	ret += json_add_str(root_obj, "messageType", "DATA");
	if (ret != 0) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	char *buffer;

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	output->buf = buffer;
	output->len = strlen(buffer);

	return 0;
}

int cloud_encode_digital_twin_data(const struct cloud_channel_data *channel,
				 struct cloud_msg *output)
{
	int ret;
	char *buffer;

	__ASSERT_NO_MSG(channel != NULL);
	__ASSERT_NO_MSG(channel->data.buf != NULL);
	__ASSERT_NO_MSG(channel->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || reported_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		return -ENOMEM;
	}

	ret = json_add_obj(reported_obj, channel_type_str[channel->type],
			   (cJSON *)channel->data.buf);
	ret += json_add_obj(state_obj, "reported", reported_obj);
	ret += json_add_obj(root_obj, "state", state_obj);

	if (ret != 0) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		return -EAGAIN;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	output->buf = buffer;
	output->len = strlen(buffer);

	return 0;
}

static int cloud_search_cmd(cJSON *group_obj, enum cloud_cmd_group group)
{
	struct cmd *cmd_group;
	cJSON *recipient_obj	= NULL;
	cJSON *channel_obj	= NULL;
	cJSON *type_obj		= NULL;

	if (group_obj == NULL) {
		return -EINVAL;
	}

	if (group == CLOUD_CMD_GROUP_SET) {
		cmd_group = &group_set;
	} else if (group == CLOUD_CMD_GROUP_GET) {
		cmd_group = &group_get;
	} else {
		return -EINVAL;
	}

	for (size_t i = 0; i < cmd_group->num_children; i++) {
		struct cmd rcpt = cmd_group->children[i];
		recipient_obj = json_object_decode(group_obj,
						   rcpt.name);
		if (recipient_obj == NULL) {
			continue;
		}

		cmd_parsed.recipient = rcpt.recipient;

		for (size_t j = 0; j < rcpt.num_children; j++) {
			struct cmd chan = rcpt.children[j];
			channel_obj = json_object_decode(recipient_obj,
							 chan.name);
			if (channel_obj == NULL) {
				continue;
			}

			cmd_parsed.channel = chan.channel;

			for (size_t k = 0; k < chan.num_children; k++) {
				struct cmd typ = chan.children[k];
				type_obj = json_object_decode(channel_obj,
							      typ.name);
				if (type_obj == NULL) {
					continue;
				}

				cmd_parsed.type = typ.type;

				cloud_command_cb(&cmd_parsed);
			}
		}
	}

	return 0;
}

int cloud_decode_command(char const *input)
{
	int ret;
	cJSON *root_obj		= NULL;
	cJSON *group_obj	= NULL;

	if (input == NULL) {
		return -EINVAL;
	}

	root_obj = cJSON_Parse(input);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	group_obj = json_object_decode(root_obj, "get");
	if (group_obj != NULL) {
		cmd_parsed.group = group_get.group;
		ret = cloud_search_cmd(group_obj, cmd_parsed.group);
		if (ret) {
			return ret;
		}
	}

	group_obj = json_object_decode(root_obj, "set");
	if (group_obj != NULL) {
		cmd_parsed.group = group_set.group;
		ret = cloud_search_cmd(group_obj, cmd_parsed.group);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int cloud_decode_init(cloud_cmd_cb_t cb)
{
	cloud_command_cb = cb;

	return 0;
}

int cloud_encode_env_sensors_data(const env_sensor_data_t *sensor_data,
				 struct cloud_msg *output)
{
	__ASSERT_NO_MSG(sensor_data != NULL);
	__ASSERT_NO_MSG(output != NULL);

	char buf[6];
	u8_t len;
	struct cloud_channel_data cloud_sensor;

	switch (sensor_data->type) {
	case ENV_SENSOR_TEMPERATURE:
		cloud_sensor.type = CLOUD_CHANNEL_TEMP;
		break;

	case ENV_SENSOR_HUMIDITY:
		cloud_sensor.type = CLOUD_CHANNEL_HUMID;
		break;

	case ENV_SENSOR_AIR_PRESSURE:
		cloud_sensor.type = CLOUD_CHANNEL_AIR_PRESS;
		break;

	default:
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "%.1f",
		sensor_data->value);
	cloud_sensor.data.buf = buf;
	cloud_sensor.data.len = len;

	return cloud_encode_data(&cloud_sensor, output);
}
