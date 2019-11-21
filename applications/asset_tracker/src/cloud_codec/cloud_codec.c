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

enum sensor_chan_cfg_item_type {
	SENSOR_CHAN_CFG_ITEM_TYPE__BEGIN,

	SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE = SENSOR_CHAN_CFG_ITEM_TYPE__BEGIN,
	SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_VALUE,
	SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE,
	SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_VALUE,
	SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE,

	SENSOR_CHAN_CFG_ITEM_TYPE__END
};

struct sensor_chan_cfg {
	double value[SENSOR_CHAN_CFG_ITEM_TYPE__END];
};

struct cloud_sensor_chan_cfg {
	enum cloud_channel chan;
	struct sensor_chan_cfg cfg;
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
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(pressure, CLOUD_CHANNEL_AIR_PRESS, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(temperature, CLOUD_CHANNEL_TEMP, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(air_quality, CLOUD_CHANNEL_AIR_QUAL, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		))
	),
	CMD_NEW_RECIPIENT(motion, CLOUD_RCPT_MOTION, CMD_ARRAY(
		CMD_NEW_CHAN(flip, CLOUD_CHANNEL_FLIP, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(impact, CLOUD_CHANNEL_IMPACT, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		))
	),
	CMD_NEW_RECIPIENT(ui, CLOUD_RCPT_UI, CMD_ARRAY(
		CMD_NEW_CHAN(pin, CLOUD_CHANNEL_PIN, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
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
			CMD_NEW_TYPE(play_melody, CLOUD_CMD_PLAY_MELODY),
			CMD_NEW_TYPE(play_note, CLOUD_CMD_PLAY_NOTE))
		))
	),
	CMD_NEW_RECIPIENT(light, CLOUD_RCPT_LIGHT, CMD_ARRAY(
		CMD_NEW_CHAN(red, CLOUD_CHANNEL_LIGHT_RED, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(green, CLOUD_CHANNEL_LIGHT_GREEN, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(blue, CLOUD_CHANNEL_LIGHT_BLUE, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
		),
		CMD_NEW_CHAN(ir, CLOUD_CHANNEL_LIGHT_IR, CMD_ARRAY(
			CMD_NEW_TYPE(enable, CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(threshold_high, CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(threshold_low, CLOUD_CMD_THRESHOLD_LOW))
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
	[CLOUD_CHANNEL_GPS] = CLOUD_CHANNEL_STR_GPS,
	[CLOUD_CHANNEL_FLIP] = CLOUD_CHANNEL_STR_FLIP,
	[CLOUD_CHANNEL_BUTTON] = CLOUD_CHANNEL_STR_BUTTON,
	[CLOUD_CHANNEL_TEMP] = CLOUD_CHANNEL_STR_TEMP,
	[CLOUD_CHANNEL_HUMID] = CLOUD_CHANNEL_STR_HUMID,
	[CLOUD_CHANNEL_AIR_PRESS] = CLOUD_CHANNEL_STR_AIR_PRESS,
	[CLOUD_CHANNEL_AIR_QUAL] = CLOUD_CHANNEL_STR_AIR_QUAL,
	[CLOUD_CHANNEL_LTE_LINK_RSRP] = CLOUD_CHANNEL_STR_LTE_LINK_RSRP,
	[CLOUD_CHANNEL_DEVICE_INFO] = CLOUD_CHANNEL_STR_DEVICE_INFO,
	[CLOUD_CHANNEL_LIGHT_SENSOR] = CLOUD_CHANNEL_STR_LIGHT_SENSOR,
};
static struct cloud_sensor_chan_cfg sensor_cfg[] = {
	{ .chan = CLOUD_CHANNEL_TEMP,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_HUMID,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_AIR_PRESS,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_AIR_QUAL,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_LIGHT_RED,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_LIGHT_GREEN,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_LIGHT_BLUE,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } },
	{ .chan = CLOUD_CHANNEL_LIGHT_IR,
	  .cfg = { .value = { [SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE] = true } } }
};
static cloud_cmd_cb_t cloud_command_cb;
struct cloud_command cmd_parsed;

static int cloud_cmd_handle_sensor_set_chan_cfg(struct cloud_command const *const cmd);

static int cloud_set_chan_cfg_item(const enum cloud_channel channel,
				   const enum sensor_chan_cfg_item_type type,
				   const double value);

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
	int ret = 0;
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

	/* Workaround for deleting "DEVICE" objects (with uppercase key) if
	 * it already exists in the digital twin.
	 * The cloud implementations expect the key to be lowercase "device",
	 * but that would duplicate the information and needlessly increase
	 * the size of the digital twin document if the "DEVICE" is not
	 * deleted at the same time.
	 */
	if (channel->type == CLOUD_CHANNEL_DEVICE_INFO) {
		cJSON *dummy_obj = cJSON_CreateNull();
		if (dummy_obj == NULL) {
			/* Dummy creation failed, but we'll let it do so
			 * silently as it's not a functionally critical error.
			 */
		} else {
			ret += json_add_obj(reported_obj, "DEVICE", dummy_obj);
		}
	}

	ret += json_add_obj(reported_obj, channel_type_str[channel->type],
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
	cJSON *recipient_obj = NULL;
	cJSON *channel_obj = NULL;
	cJSON *type_obj = NULL;

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
		recipient_obj = json_object_decode(group_obj, rcpt.name);
		if (recipient_obj == NULL) {
			continue;
		}

		cmd_parsed.recipient = rcpt.recipient;

		for (size_t j = 0; j < rcpt.num_children; j++) {
			struct cmd chan = rcpt.children[j];
			channel_obj =
				json_object_decode(recipient_obj, chan.name);
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
				cmd_parsed.state = CLOUD_CMD_STATE_UNDEFINED;
				cmd_parsed.value = 0;

				if (cJSON_IsNull(type_obj)) {
					cmd_parsed.state =
						CLOUD_CMD_STATE_FALSE;
				} else if (cJSON_IsBool(type_obj)) {
					cmd_parsed.state =
						cJSON_IsTrue(type_obj) ?
							CLOUD_CMD_STATE_TRUE :
							CLOUD_CMD_STATE_FALSE;
				} else if (cJSON_IsNumber(type_obj)) {
					cmd_parsed.value =
						type_obj->valuedouble;
				} else {
					continue;
				}

				if ((group == CLOUD_CMD_GROUP_SET) &&
				    (cloud_cmd_handle_sensor_set_chan_cfg(
					     &cmd_parsed) == 0)) {
					/* no need to pass to cb if */
					/* cmd was successfully handled */
					continue;
				}

				cloud_command_cb(&cmd_parsed);
			}
		}
	}

	return 0;
}

int cloud_decode_command(char const *input)
{
	cJSON *root_obj = NULL;
	cJSON *get_obj = NULL;
	cJSON *set_obj = NULL;

	if (input == NULL) {
		return -EINVAL;
	}

	root_obj = cJSON_Parse(input);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	get_obj = json_object_decode(root_obj, "get");
	set_obj = json_object_decode(root_obj, "set");

	if ((get_obj == NULL) && (set_obj == NULL)) {
		return -ENOTSUP;
	}

	if (get_obj != NULL) {
		cmd_parsed.group = group_get.group;
		(void)cloud_search_cmd(get_obj, cmd_parsed.group);
	}

	if (set_obj != NULL) {
		cmd_parsed.group = group_set.group;
		(void)cloud_search_cmd(set_obj, cmd_parsed.group);
	}

	cJSON_Delete(root_obj);

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

	case ENV_SENSOR_AIR_QUALITY:
		cloud_sensor.type = CLOUD_CHANNEL_AIR_QUAL;
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

int cloud_encode_motion_data(const motion_data_t *motion_data,
				  struct cloud_msg *output)
{
	__ASSERT_NO_MSG(motion_data != NULL);
	__ASSERT_NO_MSG(output != NULL);

	struct cloud_channel_data cloud_sensor;

	cloud_sensor.type = CLOUD_CHANNEL_FLIP;

	switch (motion_data->orientation) {
	case MOTION_ORIENTATION_NORMAL:
		cloud_sensor.data.buf = "NORMAL";
		break;
	case MOTION_ORIENTATION_UPSIDE_DOWN:
		cloud_sensor.data.buf = "UPSIDE_DOWN";
		break;
	default:
		return -1;
	}

	cloud_sensor.data.len = sizeof(cloud_sensor.data.buf) - 1;

	return cloud_encode_data(&cloud_sensor, output);

}
#if CONFIG_LIGHT_SENSOR
/* 4 32-bit ints, 3 spaces, NULL */
#define LIGHT_SENSOR_DATA_STRING_MAX_LEN ((4 * 11) + 3 + 1)
#define LIGHT_SENSOR_DATA_NO_UPDATE (-1)
int cloud_encode_light_sensor_data(const struct light_sensor_data *sensor_data,
				   struct cloud_msg *output)
{
	char buf[LIGHT_SENSOR_DATA_STRING_MAX_LEN];
	u8_t len;
	struct cloud_channel_data cloud_sensor;
	struct light_sensor_data send = { .red = LIGHT_SENSOR_DATA_NO_UPDATE,
					  .green = LIGHT_SENSOR_DATA_NO_UPDATE,
					  .blue = LIGHT_SENSOR_DATA_NO_UPDATE,
					  .ir = LIGHT_SENSOR_DATA_NO_UPDATE };

	if ((sensor_data == NULL) || (output == NULL)) {
		return -EINVAL;
	}

	if (cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_RED, sensor_data->red)) {
		send.red = sensor_data->red;
	}

	if (cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_GREEN,
				  sensor_data->green)) {
		send.green = sensor_data->green;
	}

	if (cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_BLUE,
				  sensor_data->blue)) {
		send.blue = sensor_data->blue;
	}

	if (cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_IR, sensor_data->ir)) {
		send.ir = sensor_data->ir;
	}

	len = snprintf(buf, sizeof(buf), "%d %d %d %d", send.red, send.green,
		       send.blue, send.ir);

	cloud_sensor.data.buf = buf;
	cloud_sensor.data.len = len;
	cloud_sensor.type = CLOUD_CHANNEL_LIGHT_SENSOR;

	return cloud_encode_data(&cloud_sensor, output);
}
#endif /* CONFIG_LIGHT_SENSOR */

static int sensor_chan_cfg_set_item(struct sensor_chan_cfg *const cfg,
				  const enum sensor_chan_cfg_item_type type,
				  const double value)
{
	if ((type < SENSOR_CHAN_CFG_ITEM_TYPE__BEGIN) ||
	    (type >= SENSOR_CHAN_CFG_ITEM_TYPE__END) || (cfg == NULL)) {
		return -EINVAL;
	}

	cfg->value[type] = value;

	return 0;
}

static bool sensor_chan_cfg_is_send_allowed(const struct sensor_chan_cfg *const cfg,
										    const double sensor_value)
{
	if ((cfg == NULL) ||
	    (!cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE])) {
		return false;
	}

	if (((cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE]) &&
	     (sensor_value <
	      cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_VALUE]))) {
		return true;
	}

	if (((cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE]) &&
	     (sensor_value >
	      cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_VALUE]))) {
		return true;
	}

	return (!cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE] &&
		!cfg->value[SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE]);
}

static int cloud_set_chan_cfg_item(const enum cloud_channel channel,
			  const enum sensor_chan_cfg_item_type type,
			  const double value)
{
	for (int i = 0; i < ARRAY_SIZE(sensor_cfg); ++i) {
		if (sensor_cfg[i].chan == channel) {
			return sensor_chan_cfg_set_item(&sensor_cfg[i].cfg, type,
						      value);
		}
	}

	return -ENOTSUP;
}

bool cloud_is_send_allowed(const enum cloud_channel channel, const double value)
{
	for (int i = 0; i < ARRAY_SIZE(sensor_cfg); ++i) {
		if (sensor_cfg[i].chan == channel) {
			return sensor_chan_cfg_is_send_allowed(&sensor_cfg[i].cfg,
							     value);
		}
	}

	return false;
}

static int cloud_cmd_handle_sensor_set_chan_cfg(struct cloud_command const *const cmd)
{
	int err = -ENOTSUP;

	if ((cmd == NULL) || (cmd->group != CLOUD_CMD_GROUP_SET)) {
		return -EINVAL;
	}

	switch (cmd->type) {
	case CLOUD_CMD_ENABLE:
		err = cloud_set_chan_cfg_item(
			cmd->channel, SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE,
			(cmd->state == CLOUD_CMD_STATE_TRUE));
		break;
	case CLOUD_CMD_THRESHOLD_HIGH:
		if (cmd->state == CLOUD_CMD_STATE_UNDEFINED) {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_VALUE,
				cmd->value);
			cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE,
				true);

		} else {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE,
				(cmd->state == CLOUD_CMD_STATE_TRUE));
		}
		break;
	case CLOUD_CMD_THRESHOLD_LOW:
		if (cmd->state == CLOUD_CMD_STATE_UNDEFINED) {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_VALUE,
				cmd->value);
			cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE,
				true);

		} else {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE,
				(cmd->state == CLOUD_CMD_STATE_TRUE));
		}
		break;
	default:
		break;
	}

	return err;
}
