/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <net/cloud.h>

#include "cJSON.h"
#include "cJSON_os.h"
#include "cloud_codec.h"

#include "env_sensors.h"

#define CMD_GROUP_KEY_STR "messageType"
#define CMD_CHAN_KEY_STR "appId"
#define CMD_DATA_TYPE_KEY_STR "data"

#define DISABLE_SEND_INTERVAL_VAL 0
#define MIN_INTERVAL_VAL_SECONDS 5

struct cmd {
	const char *const key;
	union {
		enum cloud_cmd_group group;
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

#define CMD_ARRAY(...) ((struct cmd[]){ __VA_ARGS__ })

#define CMD_NEW_TYPE(_type) \
	{ \
		.key = CMD_DATA_TYPE_KEY_STR, .type = _type, \
	}

#define CMD_NEW_CHAN(_chan, _children) \
	{ \
		.key = CMD_CHAN_KEY_STR, .channel = _chan, \
		.children = _children, .num_children = ARRAY_SIZE(_children), \
	}

#define CMD_NEW_GROUP(_var_name, _group, _children) \
	struct cmd _var_name = { \
		.key = CMD_GROUP_KEY_STR, \
		.group = _group, \
		.children = _children, \
		.num_children = ARRAY_SIZE(_children), \
	}

static CMD_NEW_GROUP(group_cfg_set, CLOUD_CMD_GROUP_CFG_SET, CMD_ARRAY(
		CMD_NEW_CHAN(CLOUD_CHANNEL_HUMID, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_AIR_PRESS, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_TEMP, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_AIR_QUAL, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_GPS, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_INTERVAL)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_LIGHT_SENSOR, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_LIGHT_RED, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_LIGHT_GREEN, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_LIGHT_BLUE, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_LIGHT_IR, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_HIGH),
			CMD_NEW_TYPE(CLOUD_CMD_THRESHOLD_LOW)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_RGB_LED, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_COLOR),
			CMD_NEW_TYPE(CLOUD_CMD_ENABLE)
			)
		),
	)
);

static CMD_NEW_GROUP(group_get, CLOUD_CMD_GROUP_GET, CMD_ARRAY(
		CMD_NEW_CHAN(CLOUD_CHANNEL_LTE_LINK_RSRP, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_EMPTY)
			)
		),
		CMD_NEW_CHAN(CLOUD_CHANNEL_DEVICE_INFO, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_EMPTY)
			)
		)
	)
);

static CMD_NEW_GROUP(group_data, CLOUD_CMD_GROUP_DATA, CMD_ARRAY(
		CMD_NEW_CHAN(CLOUD_CHANNEL_ASSISTED_GPS, CMD_ARRAY(
			CMD_NEW_TYPE(CLOUD_CMD_MODEM_PARAM)
			)
		),
	)
);

struct cmd *cmd_groups[] = {&group_cfg_set, &group_get, &group_data};
static cloud_cmd_cb_t cloud_command_cb;
struct cloud_command cmd_parsed;

static const char *const channel_type_str[] = {
	[CLOUD_CHANNEL_GPS] = CLOUD_CHANNEL_STR_GPS,
	[CLOUD_CHANNEL_FLIP] = CLOUD_CHANNEL_STR_FLIP,
	[CLOUD_CHANNEL_IMPACT] = "",
	[CLOUD_CHANNEL_BUTTON] = CLOUD_CHANNEL_STR_BUTTON,
	[CLOUD_CHANNEL_PIN] = "",
	[CLOUD_CHANNEL_RGB_LED] = CLOUD_CHANNEL_STR_RGB_LED,
	[CLOUD_CHANNEL_BUZZER] = "",
	[CLOUD_CHANNEL_TEMP] = CLOUD_CHANNEL_STR_TEMP,
	[CLOUD_CHANNEL_HUMID] = CLOUD_CHANNEL_STR_HUMID,
	[CLOUD_CHANNEL_AIR_PRESS] = CLOUD_CHANNEL_STR_AIR_PRESS,
	[CLOUD_CHANNEL_AIR_QUAL] = CLOUD_CHANNEL_STR_AIR_QUAL,
	[CLOUD_CHANNEL_LTE_LINK_RSRP] = CLOUD_CHANNEL_STR_LTE_LINK_RSRP,
	[CLOUD_CHANNEL_DEVICE_INFO] = CLOUD_CHANNEL_STR_DEVICE_INFO,
	[CLOUD_CHANNEL_LIGHT_SENSOR] = CLOUD_CHANNEL_STR_LIGHT_SENSOR,
	[CLOUD_CHANNEL_LIGHT_RED] = CLOUD_CHANNEL_STR_LIGHT_RED,
	[CLOUD_CHANNEL_LIGHT_GREEN] = CLOUD_CHANNEL_STR_LIGHT_GREEN,
	[CLOUD_CHANNEL_LIGHT_BLUE] = CLOUD_CHANNEL_STR_LIGHT_BLUE,
	[CLOUD_CHANNEL_LIGHT_IR] = CLOUD_CHANNEL_STR_LIGHT_IR,
	[CLOUD_CHANNEL_ASSISTED_GPS] = CLOUD_CHANNEL_STR_ASSISTED_GPS,
};
BUILD_ASSERT(ARRAY_SIZE(channel_type_str) == CLOUD_CHANNEL__TOTAL);

static const char *const cmd_group_str[] = {
	[CLOUD_CMD_GROUP_HELLO] = CLOUD_CMD_GROUP_STR_HELLO,
	[CLOUD_CMD_GROUP_START] = CLOUD_CMD_GROUP_STR_START,
	[CLOUD_CMD_GROUP_STOP] = CLOUD_CMD_GROUP_STR_STOP,
	[CLOUD_CMD_GROUP_INIT] = CLOUD_CMD_GROUP_STR_INIT,
	[CLOUD_CMD_GROUP_GET] = CLOUD_CMD_GROUP_STR_GET,
	[CLOUD_CMD_GROUP_STATUS] = CLOUD_CMD_GROUP_STR_STATUS,
	[CLOUD_CMD_GROUP_DATA] = CLOUD_CMD_GROUP_STR_DATA,
	[CLOUD_CMD_GROUP_OK] = CLOUD_CMD_GROUP_STR_OK,
	[CLOUD_CMD_GROUP_CFG_SET] = CLOUD_CMD_GROUP_STR_CFG_SET,
	[CLOUD_CMD_GROUP_CFG_GET] = CLOUD_CMD_GROUP_STR_CFG_GET,
};
BUILD_ASSERT(ARRAY_SIZE(cmd_group_str) == CLOUD_CMD_GROUP__TOTAL);

static const char *const cmd_type_str[] = {
	[CLOUD_CMD_EMPTY] = CLOUD_CMD_TYPE_STR_EMPTY,
	[CLOUD_CMD_ENABLE] = CLOUD_CMD_TYPE_STR_ENABLE,
	[CLOUD_CMD_THRESHOLD_HIGH] = CLOUD_CMD_TYPE_STR_THRESH_LO,
	[CLOUD_CMD_THRESHOLD_LOW] = CLOUD_CMD_TYPE_STR_THRESH_HI,
	[CLOUD_CMD_INTERVAL] = CLOUD_CMD_TYPE_STR_INTERVAL,
	[CLOUD_CMD_COLOR] = CLOUD_CMD_TYPE_STR_COLOR,
	[CLOUD_CMD_MODEM_PARAM] = CLOUD_CMD_TYPE_STR_MODEM_PARAM,
};
BUILD_ASSERT(ARRAY_SIZE(cmd_type_str) == CLOUD_CMD__TOTAL);

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

static bool json_value_string_compare(cJSON *obj, const char *const str)
{
	char *json_str = cJSON_GetStringValue(obj);

	if ((json_str == NULL) || (str == NULL)) {
		return false;
	}

	return (strcmp(json_str, str) == 0);
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

	ret = json_add_str(root_obj, CMD_CHAN_KEY_STR,
			   channel_type_str[channel->type]);
	ret += json_add_str(root_obj, CMD_DATA_TYPE_KEY_STR, channel->data.buf);
	ret += json_add_str(root_obj, CMD_GROUP_KEY_STR,
			    CLOUD_CMD_GROUP_STR_DATA);
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

static int cloud_decode_modem_params(cJSON *const data_obj,
			  struct cloud_command_modem_params *const params)
{
	cJSON *blob;
	cJSON *checksum;

	if ((data_obj == NULL) || (params == NULL)) {
		return -EINVAL;
	}

	if (!cJSON_IsObject(data_obj)) {
		return -ESRCH;
	}

	blob = json_object_decode(data_obj, MODEM_PARAM_BLOB_KEY_STR);
	params->blob = cJSON_IsString(blob) ? blob->valuestring : NULL;

	checksum = json_object_decode(data_obj, MODEM_PARAM_CHECKSUM_KEY_STR);
	params->checksum =
		cJSON_IsString(checksum) ? checksum->valuestring : NULL;

	return (((params->blob == NULL) || (params->checksum == NULL)) ?
			-ESRCH : 0);
}

static int cloud_cmd_parse_type(const struct cmd *const type_cmd,
				cJSON *type_obj,
				struct cloud_command *const parsed_cmd)
{
	int err;
	cJSON *decoded_obj = NULL;

	if ((type_cmd == NULL) || (parsed_cmd == NULL)) {
		return -EINVAL;
	}

	if (type_obj != NULL) {
		decoded_obj = json_object_decode(type_obj,
						 cmd_type_str[type_cmd->type]);

		if (!decoded_obj) {
			return -ENOENT; /* Command not found */
		}

		switch (type_cmd->type) {
		case CLOUD_CMD_ENABLE: {
			if (cJSON_IsNull(decoded_obj)) {
				parsed_cmd->data.sv.state =
					CLOUD_CMD_STATE_FALSE;
			} else if (cJSON_IsBool(decoded_obj)) {
				parsed_cmd->data.sv.state =
					cJSON_IsTrue(decoded_obj) ?
						CLOUD_CMD_STATE_TRUE :
						CLOUD_CMD_STATE_FALSE;
			} else {
				return -ESRCH;
			}

			break;
		}
		case CLOUD_CMD_INTERVAL:
		case CLOUD_CMD_THRESHOLD_LOW:
		case CLOUD_CMD_THRESHOLD_HIGH: {
			if (cJSON_IsNull(decoded_obj)) {
				parsed_cmd->data.sv.state =
					CLOUD_CMD_STATE_FALSE;
			} else if (cJSON_IsNumber(decoded_obj)) {
				parsed_cmd->data.sv.state =
					CLOUD_CMD_STATE_UNDEFINED;
				parsed_cmd->data.sv.value =
					decoded_obj->valuedouble;
			} else {
				return -ESRCH;
			}

			break;
		}
		case CLOUD_CMD_COLOR: {
			if (!cJSON_IsString(decoded_obj) ||
				!decoded_obj->valuestring) {
				return -ESRCH;
			}

			parsed_cmd->data.sv.value = (double)strtol(
				decoded_obj->valuestring, NULL, 16);
			break;
		}
		case CLOUD_CMD_MODEM_PARAM: {
			err = cloud_decode_modem_params(decoded_obj,
							&parsed_cmd->data.mp);

			if (err) {
				return err;
			}

			break;
		}
		case CLOUD_CMD_EMPTY:
		default: {
			return -ENOTSUP;
		}
		}
	} else if (type_cmd->type != CLOUD_CMD_EMPTY) {
		/* Only the empty cmd type can have no data */
		return -EINVAL;
	}

	/* Validate interval value */
	if ((type_cmd->type == CLOUD_CMD_INTERVAL) &&
	    (parsed_cmd->data.sv.state == CLOUD_CMD_STATE_UNDEFINED)) {
		if (parsed_cmd->data.sv.value == DISABLE_SEND_INTERVAL_VAL) {
			parsed_cmd->data.sv.state = CLOUD_CMD_STATE_FALSE;
		} else if (parsed_cmd->data.sv.value <
				   MIN_INTERVAL_VAL_SECONDS) {
			parsed_cmd->data.sv.value = MIN_INTERVAL_VAL_SECONDS;
		}
	}

	parsed_cmd->type = type_cmd->type;

	return 0;
}

static int cloud_search_cmd(cJSON *root_obj)
{
	int ret;
	struct cmd *group	= NULL;
	struct cmd *chan	= NULL;
	struct cmd *type	= NULL;
	cJSON *group_obj;
	cJSON *channel_obj;
	cJSON *type_obj;

	if (root_obj == NULL) {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(cmd_groups); ++i) {
		group_obj = json_object_decode(root_obj, cmd_groups[i]->key);

		if ((group_obj != NULL) &&
			(json_value_string_compare(group_obj,
					cmd_group_str[cmd_groups[i]->group]))) {
			group = cmd_groups[i];
			break;
		}
	}

	if (group == NULL) {
		return -ENOTSUP;
	}

	cmd_parsed.group = group->group;

	for (size_t j = 0; j < group->num_children; ++j) {
		channel_obj =
			json_object_decode(root_obj, group->children[j].key);

		if ((channel_obj != NULL) &&
		    (json_value_string_compare(
			    channel_obj,
			    channel_type_str[group->children[j].channel]))) {
			chan = &group->children[j];
			break;
		}
	}

	if (chan == NULL) {
		return -ENOTSUP;
	}

	cmd_parsed.channel = chan->channel;

	for (size_t k = 0; k < chan->num_children; ++k) {

		type = &chan->children[k];
		type_obj = json_object_decode(root_obj, type->key);

		ret = cloud_cmd_parse_type(type, type_obj, &cmd_parsed);

		if (ret != 0) {
			if (ret != -ENOENT) {
				printk("[%s:%d] Unhandled cmd format for %s, %s, error %d\n",
						__func__, __LINE__,
						cmd_group_str[group->group],
						channel_type_str[chan->channel],
						ret);
			}
			continue;
		}

		printk("[%s:%d] Found cmd %s, %s, %s\n", __func__, __LINE__,
				cmd_group_str[cmd_parsed.group],
				channel_type_str[cmd_parsed.channel],
				cmd_type_str[cmd_parsed.type]);

		/* Handle cfg commands */
		(void)cloud_cmd_handle_sensor_set_chan_cfg(&cmd_parsed);

		if (cloud_command_cb) {
			cloud_command_cb(&cmd_parsed);
		}
	}

	return 0;
}

int cloud_decode_command(char const *input)
{
	cJSON *root_obj = NULL;

	if (input == NULL) {
		return -EINVAL;
	}

	root_obj = cJSON_Parse(input);
	if (root_obj == NULL) {
		printk("[%s:%d] Unable to parse input\n", __func__, __LINE__);
		return -ENOENT;
	}

	cloud_search_cmd(root_obj);

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

	if ((cmd == NULL) || (cmd->group != CLOUD_CMD_GROUP_CFG_SET)) {
		return -EINVAL;
	}

	switch (cmd->type) {
	case CLOUD_CMD_INTERVAL:
		if (cmd->data.sv.state == CLOUD_CMD_STATE_UNDEFINED) {
			/* Undefined means a valid value was set, so enable */
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE,
				true);

		} else {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_SEND_ENABLE,
				(cmd->data.sv.state == CLOUD_CMD_STATE_TRUE));
		}
		break;
	case CLOUD_CMD_THRESHOLD_HIGH:
		if (cmd->data.sv.state == CLOUD_CMD_STATE_UNDEFINED) {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_VALUE,
				cmd->data.sv.value);
			cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE,
				true);

		} else {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_HIGH_ENABLE,
				(cmd->data.sv.state == CLOUD_CMD_STATE_TRUE));
		}
		break;
	case CLOUD_CMD_THRESHOLD_LOW:
		if (cmd->data.sv.state == CLOUD_CMD_STATE_UNDEFINED) {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_VALUE,
				cmd->data.sv.value);
			cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE,
				true);

		} else {
			err = cloud_set_chan_cfg_item(
				cmd->channel,
				SENSOR_CHAN_CFG_ITEM_TYPE_THRESH_LOW_ENABLE,
				(cmd->data.sv.state == CLOUD_CMD_STATE_TRUE));
		}
		break;
	default:
		break;
	}

	return err;
}
