/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <at_cmd_parser.h>
#include <at_cmd.h>
#include <device.h>
#include <errno.h>
#include <modem_info.h>
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(modem_info);

#define INVALID_DESCRIPTOR	-1

#define AT_CMD_CESQ		"AT%CESQ"
#define AT_CMD_CESQ_ON		"AT%CESQ=1"
#define AT_CMD_CESQ_OFF		"AT%CESQ=0"
#define AT_CMD_CESQ_RESP	"%CESQ"
#define AT_CMD_CURRENT_BAND	"AT%XCBAND"
#define AT_CMD_CURRENT_MODE	"AT+CEMODE?"
#define AT_CMD_CURRENT_OP	"AT+COPS?"
#define AT_CMD_CELLID		"AT+CEREG?"
#define AT_CMD_PDP_CONTEXT	"AT+CGDCONT?"
#define AT_CMD_UICC_STATE	"AT%XSIM?"
#define AT_CMD_VBAT		"AT%XVBAT"
#define AT_CMD_TEMP		"AT%XTEMP"
#define AT_CMD_FW_VERSION	"AT+CGMR"
#define AT_CMD_CRSM		"AT+CRSM"
#define AT_CMD_ICCID		"AT+CRSM=176,12258,0,0,10"
#define AT_CMD_SUCCESS_SIZE	5
#define RSRP_PARAM_INDEX 0
#define RSRP_PARAM_COUNT 2
#define RSRP_OFFSET_VAL 141

#define BAND_PARAM_INDEX 0 /* Index of desired parameter in returned string */
#define BAND_PARAM_COUNT 1 /* Number of parameters returned from modem */

#define MODE_PARAM_INDEX 0
#define MODE_PARAM_COUNT 1

#define OPERATOR_PARAM_INDEX 2
#define OPERATOR_PARAM_COUNT 4

#define CELLID_PARAM_INDEX 3
#define CELLID_PARAM_COUNT 5

#define IP_ADDRESS_PARAM_INDEX 3
#define IP_ADDRESS_PARAM_COUNT 6

#define UICC_PARAM_INDEX 0
#define UICC_PARAM_COUNT 1

#define VBAT_PARAM_INDEX 0
#define VBAT_PARAM_COUNT 1

#define TEMP_PARAM_INDEX 1
#define TEMP_PARAM_COUNT 2

#define FW_PARAM_INDEX 0
#define FW_PARAM_COUNT 1

#define ICCID_PARAM_INDEX 2
#define ICCID_PARAM_COUNT 3

#define CMD_SIZE(x) (strlen(x) - 1)

struct modem_info_data {
	const char *cmd;
	u8_t param_index;
	u8_t param_count;
	enum at_param_type data_type;
};

static const struct modem_info_data rsrp_data = {
	.cmd = AT_CMD_CESQ,
	.param_index = RSRP_PARAM_INDEX,
	.param_count = RSRP_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data band_data = {
	.cmd = AT_CMD_CURRENT_BAND,
	.param_index = BAND_PARAM_INDEX,
	.param_count = BAND_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data mode_data = {
	.cmd = AT_CMD_CURRENT_MODE,
	.param_index = MODE_PARAM_INDEX,
	.param_count = MODE_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data operator_data = {
	.cmd = AT_CMD_CURRENT_OP,
	.param_index = OPERATOR_PARAM_INDEX,
	.param_count = OPERATOR_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data cellid_data = {
	.cmd = AT_CMD_CELLID,
	.param_index = CELLID_PARAM_INDEX,
	.param_count = CELLID_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data ip_data = {
	.cmd = AT_CMD_PDP_CONTEXT,
	.param_index = IP_ADDRESS_PARAM_INDEX,
	.param_count = IP_ADDRESS_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data uicc_data = {
	.cmd = AT_CMD_UICC_STATE,
	.param_index = UICC_PARAM_INDEX,
	.param_count = UICC_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data battery_data = {
	.cmd = AT_CMD_VBAT,
	.param_index = VBAT_PARAM_INDEX,
	.param_count = VBAT_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data temp_data = {
	.cmd = AT_CMD_TEMP,
	.param_index = TEMP_PARAM_INDEX,
	.param_count = TEMP_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data fw_data = {
	.cmd = AT_CMD_FW_VERSION,
	.param_index = FW_PARAM_INDEX,
	.param_count = FW_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data iccid_data = {
	.cmd = AT_CMD_ICCID,
	.param_index = ICCID_PARAM_INDEX,
	.param_count = ICCID_PARAM_COUNT,
	.data_type = AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data *const modem_data[] = {
	[MODEM_INFO_RSRP] = &rsrp_data,
	[MODEM_INFO_BAND] = &band_data,
	[MODEM_INFO_MODE] = &mode_data,
	[MODEM_INFO_OPERATOR] = &operator_data,
	[MODEM_INFO_CELLID] = &cellid_data,
	[MODEM_INFO_IP_ADDRESS] = &ip_data,
	[MODEM_INFO_UICC] = &uicc_data,
	[MODEM_INFO_BATTERY] = &battery_data,
	[MODEM_INFO_TEMP] = &temp_data,
	[MODEM_INFO_FW_VERSION] = &fw_data,
	[MODEM_INFO_ICCID] = &iccid_data,
};

static const char *const modem_data_name[] = {
	[MODEM_INFO_RSRP] = "RSRP",
	[MODEM_INFO_BAND] = "BAND",
	[MODEM_INFO_MODE] = "MODE",
	[MODEM_INFO_OPERATOR] = "OPERATOR",
	[MODEM_INFO_CELLID] = "CELLID",
	[MODEM_INFO_IP_ADDRESS] = "IP ADDRESS",
	[MODEM_INFO_UICC] = "UICC STATE",
	[MODEM_INFO_BATTERY] = "BATTERY",
	[MODEM_INFO_TEMP] = "TEMP",
	[MODEM_INFO_FW_VERSION] = "MODEM FW",
	[MODEM_INFO_ICCID] = "ICCID",
};

static rsrp_cb_t modem_info_rsrp_cb;
static struct at_param_list m_param_list;

static bool is_cesq_notification(char *buf, size_t len)
{
	return strstr(buf, AT_CMD_CESQ_RESP) ? true : false;
}

static void flip_iccid_string(char *buf)
{
	u8_t current_char;
	u8_t next_char;

	for (size_t i = 0; i < strlen(buf); i = i + 2) {
		current_char = buf[i];
		next_char = buf[i + 1];

		buf[i] = next_char;
		buf[i + 1] = current_char;
	}
}

static int modem_info_parse_iccid(const struct modem_info_data *modem_data,
				  char *buf)
{
	int err;
	u32_t param_index;

	err = at_parser_max_params_from_str(
		&buf[CMD_SIZE(AT_CMD_CRSM)], &m_param_list,
		modem_data->param_count);

	if (err != 0) {
		return err;
	}

	param_index = at_params_valid_count_get(&m_param_list);
	if (param_index != modem_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

static int modem_info_parse(const struct modem_info_data *modem_data, char *buf)
{
	int err;
	u32_t param_index;

	err = at_parser_max_params_from_str(
		&buf[CMD_SIZE(modem_data->cmd)], &m_param_list,
		modem_data->param_count);

	if (err != 0) {
		return err;
	}

	param_index = at_params_valid_count_get(&m_param_list);
	if (param_index != modem_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

enum at_param_type modem_info_type_get(enum modem_info info)
{
	__ASSERT(info < MODEM_INFO_COUNT, "Invalid argument.");

	return modem_data[info]->data_type;
}

int modem_info_name_get(enum modem_info info, char *name)
{
	int len;

	if (name == NULL) {
		return -EINVAL;
	}

	len = strlen(modem_data_name[info]);

	if (len <= 0) {
		return -EINVAL;
	}

	memcpy(name,
		modem_data_name[info],
		len);

	return len;
}

int modem_info_short_get(enum modem_info info, u16_t *buf)
{
	int err;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};

	if (buf == NULL) {
		return -EINVAL;
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	err = at_cmd_write(modem_data[info]->cmd,
			   recv_buf,
			   CONFIG_MODEM_INFO_BUFFER_SIZE,
			   NULL);

	if (err != 0) {
		return -EIO;
	}

	err = modem_info_parse(modem_data[info], recv_buf);

	if (err) {
		return err;
	}

	err = at_params_short_get(&m_param_list,
				  modem_data[info]->param_index,
				  buf);

	if (err) {
		return err;
	}

	return sizeof(u16_t);
}

int modem_info_string_get(enum modem_info info, char *buf)
{
	int err;
	int len = 0;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	u16_t param_value;

	if (buf == NULL) {
		return -EINVAL;
	}

	err = at_cmd_write(modem_data[info]->cmd,
			  recv_buf,
			  CONFIG_MODEM_INFO_BUFFER_SIZE,
			  NULL);

	if (err != 0) {
		return -EIO;
	}

	if (info == MODEM_INFO_ICCID) {
		err = modem_info_parse_iccid(modem_data[info], recv_buf);
	} else {
		err = modem_info_parse(modem_data[info], recv_buf);
	}

	if (err) {
		return err;
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_NUM_SHORT) {
		err = at_params_short_get(&m_param_list,
					  modem_data[info]->param_index,
					  &param_value);
		if (err) {
			return err;
		}

		len = snprintf(buf, MODEM_INFO_MAX_RESPONSE_SIZE,
				"%d", param_value);
	} else if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		len = at_params_string_get(&m_param_list,
					   modem_data[info]->param_index,
					   buf,
					   MODEM_INFO_MAX_RESPONSE_SIZE);
	}

	if (info == MODEM_INFO_ICCID) {
		flip_iccid_string(buf);
	}

	return len <= 0 ? -ENOTSUP : len;
}

static void modem_info_rsrp_subscribe_handler(char *response)
{
	u16_t param_value;
	int   len = strlen(response);

	if (is_cesq_notification(response, len)) {
		modem_info_parse(modem_data[MODEM_INFO_RSRP], response);
		len = modem_info_short_get(MODEM_INFO_RSRP, &param_value);
		modem_info_rsrp_cb(param_value);
	}
}

int modem_info_rsrp_register(rsrp_cb_t cb)
{
	modem_info_rsrp_cb = cb;

	at_cmd_set_notification_handler(modem_info_rsrp_subscribe_handler);

	if (at_cmd_write(AT_CMD_CESQ_ON, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int modem_info_init(void)
{
	/* Init at_cmd_parser storage module */
	int err = at_params_list_init(&m_param_list,
				CONFIG_MODEM_INFO_MAX_AT_PARAMS_RSP);

	return err;
}
