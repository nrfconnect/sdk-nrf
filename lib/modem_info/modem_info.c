/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <modem/at_cmd_parser.h>
#include <ctype.h>
#include <zephyr/device.h>
#include <errno.h>
#include <modem/modem_info.h>
#include <nrf_errno.h>
#include <zephyr/net/socket.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modem_info);

#define INVALID_DESCRIPTOR	-1

#define AT_CMD_CESQ		"AT+CESQ"
#define AT_CMD_CESQ_ON		"AT%%CESQ=1"
#define AT_CMD_CESQ_OFF		"AT%%CESQ=0"
#define AT_CMD_CURRENT_BAND	"AT%%XCBAND"
#define AT_CMD_SUPPORTED_BAND	"AT%%XCBAND=?"
#define AT_CMD_CURRENT_MODE	"AT+CEMODE?"
#define AT_CMD_CURRENT_OP	"AT+COPS?"
#define AT_CMD_NETWORK_STATUS	"AT+CEREG?"
#define AT_CMD_PDP_CONTEXT	"AT+CGDCONT?"
#define AT_CMD_UICC_STATE	"AT%%XSIM?"
#define AT_CMD_VBAT		"AT%%XVBAT"
#define AT_CMD_TEMP		"AT%%XTEMP?"
#define AT_CMD_FW_VERSION	"AT+CGMR"
#define AT_CMD_CRSM		"AT+CRSM"
#define AT_CMD_ICCID		"AT+CRSM=176,12258,0,0,10"
#define AT_CMD_SYSTEMMODE	"AT%%XSYSTEMMODE?"
#define AT_CMD_IMSI		"AT+CIMI"
#define AT_CMD_IMEI		"AT+CGSN"
#define AT_CMD_DATE_TIME	"AT+CCLK?"
#define AT_CMD_SUCCESS_SIZE	5

#define RSRP_DATA_NAME		"rsrp"
#define CUR_BAND_DATA_NAME	"currentBand"
#define SUP_BAND_DATA_NAME	"supportedBands"
#define UE_MODE_DATA_NAME	"ueMode"
#define OPERATOR_DATA_NAME	"mccmnc"
#define MCC_DATA_NAME		"mcc"
#define MNC_DATA_NAME		"mnc"
#define AREA_CODE_DATA_NAME	"areaCode"
#define CELLID_DATA_NAME	"cellID"
#define IP_ADDRESS_DATA_NAME	"ipAddress"
#define UICC_DATA_NAME		"uiccMode"
#define BATTERY_DATA_NAME	"batteryVoltage"
#define TEMPERATURE_DATA_NAME	"temperature"
#define MODEM_FW_DATA_NAME	"modemFirmware"
#define ICCID_DATA_NAME		"iccid"
#define LTE_MODE_DATA_NAME	"lteMode"
#define NBIOT_MODE_DATA_NAME	"nbiotMode"
#define GPS_MODE_DATA_NAME	"gpsMode"
#define IMSI_DATA_NAME		"imsi"
#define MODEM_IMEI_DATA_NAME	"imei"
#define DATE_TIME_DATA_NAME	"dateTime"
#define APN_DATA_NAME		"apn"

#define AT_CMD_RSP_DELIM "\r\n"
#define IP_ADDR_SEPARATOR ", "
#define IP_ADDR_SEPARATOR_LEN (sizeof(IP_ADDR_SEPARATOR)-1)

#define RSRP_NOTIFY_PARAM_INDEX	1
#define RSRP_NOTIFY_PARAM_COUNT	5

#define RSRP_PARAM_INDEX	6
#define RSRP_PARAM_COUNT	7

#define BAND_PARAM_INDEX	1 /* Index of desired parameter */
#define BAND_PARAM_COUNT	2 /* Number of parameters */

#define MODE_PARAM_INDEX	1
#define MODE_PARAM_COUNT	2

#define OPERATOR_PARAM_INDEX	3
#define OPERATOR_PARAM_COUNT	5

#define CELLID_PARAM_INDEX	4
#define CELLID_PARAM_COUNT	10

#define AREA_CODE_PARAM_INDEX	3
#define AREA_CODE_PARAM_COUNT	10

#define IP_ADDRESS_PARAM_INDEX	4
#define IP_ADDRESS_PARAM_COUNT	7

#define UICC_PARAM_INDEX	1
#define UICC_PARAM_COUNT	2

#define VBAT_PARAM_INDEX	1
#define VBAT_PARAM_COUNT	2

#define TEMP_PARAM_INDEX	1
#define TEMP_PARAM_COUNT	2

#define MODEM_FW_PARAM_INDEX	0
#define MODEM_FW_PARAM_COUNT	1

#define ICCID_PARAM_INDEX	3
#define ICCID_PARAM_COUNT	4
#define ICCID_LEN		20
#define ICCID_PAD_CHAR		'F'

#define LTE_MODE_PARAM_INDEX	1
#define NBIOT_MODE_PARAM_INDEX	2
#define GPS_MODE_PARAM_INDEX	3
#define SYSTEMMODE_PARAM_COUNT	5

#define IMSI_PARAM_INDEX	0
#define IMSI_PARAM_COUNT	1

#define MODEM_IMEI_PARAM_INDEX	0
#define MODEM_IMEI_PARAM_COUNT  1

#define DATE_TIME_PARAM_INDEX	1
#define DATE_TIME_PARAM_COUNT	2

#define APN_PARAM_INDEX		3
#define APN_PARAM_COUNT		7

#define CELL_RSRP_INVALID	255

/* FW UUID is 36 characters: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */
#define FW_UUID_SIZE 37

/* Size of the format string for requesting modem firmware version */
#define SWVER_FMT_STR_SIZE 23

/* SVN is a NULL-terminated string with 2 digits: XX */
#define SVN_SIZE 3

#define HWVER_CMD_STR "HWVERSION"
#define HWVER_FMT_STR "%%%%" HWVER_CMD_STR ": %%%d[^" AT_CMD_RSP_DELIM "]"

struct modem_info_data {
	const char *cmd;
	const char *data_name;
	uint8_t param_index;
	uint8_t param_count;
	enum at_param_type data_type;
};

static const struct modem_info_data rsrp_data = {
	.cmd		= AT_CMD_CESQ,
	.data_name	= RSRP_DATA_NAME,
	.param_index	= RSRP_PARAM_INDEX,
	.param_count	= RSRP_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data band_data = {
	.cmd		= AT_CMD_CURRENT_BAND,
	.data_name	= CUR_BAND_DATA_NAME,
	.param_index	= BAND_PARAM_INDEX,
	.param_count	= BAND_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data band_sup_data = {
	.cmd		= AT_CMD_SUPPORTED_BAND,
	.data_name	= SUP_BAND_DATA_NAME,
	.param_index	= BAND_PARAM_INDEX,
	.param_count	= BAND_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data mode_data = {
	.cmd		= AT_CMD_CURRENT_MODE,
	.data_name	= UE_MODE_DATA_NAME,
	.param_index	= MODE_PARAM_INDEX,
	.param_count	= MODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data operator_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= OPERATOR_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data mcc_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= MCC_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data mnc_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= MNC_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data cellid_data = {
	.cmd		= AT_CMD_NETWORK_STATUS,
	.data_name	= CELLID_DATA_NAME,
	.param_index	= CELLID_PARAM_INDEX,
	.param_count	= CELLID_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data area_data = {
	.cmd		= AT_CMD_NETWORK_STATUS,
	.data_name	= AREA_CODE_DATA_NAME,
	.param_index	= AREA_CODE_PARAM_INDEX,
	.param_count	= AREA_CODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data ip_data = {
	.cmd		= AT_CMD_PDP_CONTEXT,
	.data_name	= IP_ADDRESS_DATA_NAME,
	.param_index	= IP_ADDRESS_PARAM_INDEX,
	.param_count	= IP_ADDRESS_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data uicc_data = {
	.cmd		= AT_CMD_UICC_STATE,
	.data_name	= UICC_DATA_NAME,
	.param_index	= UICC_PARAM_INDEX,
	.param_count	= UICC_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data battery_data = {
	.cmd		= AT_CMD_VBAT,
	.data_name	= BATTERY_DATA_NAME,
	.param_index	= VBAT_PARAM_INDEX,
	.param_count	= VBAT_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data temp_data = {
	.cmd		= AT_CMD_TEMP,
	.data_name	= TEMPERATURE_DATA_NAME,
	.param_index	= TEMP_PARAM_INDEX,
	.param_count	= TEMP_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data fw_data = {
	.cmd		= AT_CMD_FW_VERSION,
	.data_name	= MODEM_FW_DATA_NAME,
	.param_index	= MODEM_FW_PARAM_INDEX,
	.param_count	= MODEM_FW_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data iccid_data = {
	.cmd		= AT_CMD_ICCID,
	.data_name	= ICCID_DATA_NAME,
	.param_index	= ICCID_PARAM_INDEX,
	.param_count	= ICCID_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data lte_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= LTE_MODE_DATA_NAME,
	.param_index	= LTE_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data nbiot_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= NBIOT_MODE_DATA_NAME,
	.param_index	= NBIOT_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data gps_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= GPS_MODE_DATA_NAME,
	.param_index	= GPS_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_INT,
};

static const struct modem_info_data imsi_data = {
	.cmd		= AT_CMD_IMSI,
	.data_name	= IMSI_DATA_NAME,
	.param_index	= IMSI_PARAM_INDEX,
	.param_count	= IMSI_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data imei_data = {
	.cmd		= AT_CMD_IMEI,
	.data_name	= MODEM_IMEI_DATA_NAME,
	.param_index	= MODEM_IMEI_PARAM_INDEX,
	.param_count	= MODEM_IMEI_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data date_time_data = {
	.cmd		= AT_CMD_DATE_TIME,
	.data_name	= DATE_TIME_DATA_NAME,
	.param_index	= DATE_TIME_PARAM_INDEX,
	.param_count	= DATE_TIME_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data apn_data = {
	.cmd		= AT_CMD_PDP_CONTEXT,
	.data_name	= APN_DATA_NAME,
	.param_index	= APN_PARAM_INDEX,
	.param_count	= APN_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data *const modem_data[] = {
	[MODEM_INFO_RSRP]	= &rsrp_data,
	[MODEM_INFO_CUR_BAND]	= &band_data,
	[MODEM_INFO_SUP_BAND]	= &band_sup_data,
	[MODEM_INFO_UE_MODE]	= &mode_data,
	[MODEM_INFO_OPERATOR]	= &operator_data,
	[MODEM_INFO_MCC]	= &mcc_data,
	[MODEM_INFO_MNC]	= &mnc_data,
	[MODEM_INFO_AREA_CODE]	= &area_data,
	[MODEM_INFO_CELLID]	= &cellid_data,
	[MODEM_INFO_IP_ADDRESS]	= &ip_data,
	[MODEM_INFO_UICC]	= &uicc_data,
	[MODEM_INFO_BATTERY]	= &battery_data,
	[MODEM_INFO_TEMP]	= &temp_data,
	[MODEM_INFO_FW_VERSION]	= &fw_data,
	[MODEM_INFO_ICCID]	= &iccid_data,
	[MODEM_INFO_LTE_MODE]	= &lte_mode_data,
	[MODEM_INFO_NBIOT_MODE] = &nbiot_mode_data,
	[MODEM_INFO_GPS_MODE]   = &gps_mode_data,
	[MODEM_INFO_IMSI]	= &imsi_data,
	[MODEM_INFO_IMEI]	= &imei_data,
	[MODEM_INFO_DATE_TIME]	= &date_time_data,
	[MODEM_INFO_APN]	= &apn_data,
};

AT_MONITOR(modem_info_cesq_mon, "%CESQ", modem_info_rsrp_subscribe_handler, PAUSED);

static rsrp_cb_t modem_info_rsrp_cb;
static struct at_param_list m_param_list;

static void flip_iccid_string(char *buf)
{
	uint8_t current_char;
	uint8_t next_char;

	for (size_t i = 0; i < strlen(buf); i = i + 2) {
		current_char = buf[i];
		next_char = buf[i + 1];

		buf[i] = next_char;
		buf[i + 1] = current_char;
	}
}

static int modem_info_parse(const struct modem_info_data *modem_data,
			    const char *buf)
{
	int err;
	uint32_t param_index;

	err = at_parser_max_params_from_str(buf, NULL, &m_param_list,
					    modem_data->param_count);

	if (err == -EAGAIN) {
		LOG_DBG("More items exist to parse for: %s",
			modem_data->data_name);
		err = 0;
	} else if (err != 0) {
		return err;
	}

	param_index = at_params_valid_count_get(&m_param_list);
	if (param_index > modem_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

static int map_nrf_modem_at_scanf_error(int err)
{
	switch (err) {
	case -NRF_EPERM:
		return -EPERM;
	case -NRF_EFAULT:
		return -EFAULT;
	case -NRF_EBADMSG:
		return -EBADMSG;
	case -NRF_ENOMEM:
		return -ENOMEM;
	default:
		return -EIO;
	}
}

enum at_param_type modem_info_type_get(enum modem_info info_type)
{
	if (info_type >= MODEM_INFO_COUNT) {
		return -EINVAL;
	}

	return modem_data[info_type]->data_type;
}

int modem_info_name_get(enum modem_info info, char *name)
{
	int len;

	if (name == NULL || info < 0 || info >= MODEM_INFO_COUNT) {
		return -EINVAL;
	}

	len = strlen(modem_data[info]->data_name);
	__ASSERT_NO_MSG(len <= MODEM_INFO_MAX_RESPONSE_SIZE);

	if (len <= 0) {
		return -EINVAL;
	}

	memcpy(name,
		modem_data[info]->data_name,
		len);

	return len;
}

int modem_info_short_get(enum modem_info info, uint16_t *buf)
{
	int err;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};

	if (buf == NULL) {
		return -EINVAL;
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	err = nrf_modem_at_cmd(recv_buf, CONFIG_MODEM_INFO_BUFFER_SIZE, modem_data[info]->cmd);
	if (err != 0) {
		return -EIO;
	}

	err = modem_info_parse(modem_data[info], recv_buf);
	if (err) {
		return err;
	}

	err = at_params_unsigned_short_get(&m_param_list,
					   modem_data[info]->param_index,
					   buf);

	if (err) {
		return err;
	}

	return sizeof(uint16_t);
}

static int parse_ip_addresses(char *out_buf, size_t out_buf_size, char *in_buf)
{
	int err;
	char *p;
	char *str_end = in_buf;
	int current_ip_idx = 0;
	int total_ip_count = 0;
	size_t line_len = 0;
	/* Tracks length of buf when parsing multiple IP addresses */
	size_t out_buf_len = 0;
	/* Return value indicating length of the string written to IP buffer */
	size_t len = 0;
	size_t line_start_idx = 0;
	char ip_buf[INET_ADDRSTRLEN + sizeof(" ") + INET6_ADDRSTRLEN];
	char *ip_v6_str;
	bool first_address;

	p = strstr(in_buf, "OK\r\n");
	if (!p) {
		LOG_WRN("No response status: %s", in_buf);
		return -EINVAL;
	}

	/* Trim the last part of the response,
	 * we use \r\n to count the number of IP addresses
	 */
	*p = '\0';

	/* Check for potentially multiple IP addresses */
	while ((str_end = strstr(str_end, AT_CMD_RSP_DELIM)) != NULL) {
		str_end++;
		total_ip_count++;
	}

	if (total_ip_count == 0) {
		LOG_DBG("No IP addresses found");
		return 0;
	}

	current_ip_idx = total_ip_count;
	first_address = true;

	LOG_DBG("Device has %d IP addresses", total_ip_count);

parse_line:
	/* Parse each +CGDCONT: line separately */
	str_end = strstr(&in_buf[line_start_idx], AT_CMD_RSP_DELIM);
	if (str_end == NULL) {
		return -EFAULT;
	}

	/* Get the size and then null-terminate the line */
	line_len = str_end - &in_buf[line_start_idx];
	in_buf[++line_len + line_start_idx] = '\0';

	err = modem_info_parse(modem_data[MODEM_INFO_IP_ADDRESS], &in_buf[line_start_idx]);
	if (err) {
		LOG_ERR("Unable to parse data: %d", err);
		return err;
	}

	len = sizeof(ip_buf);
	err = at_params_string_get(&m_param_list,
				   modem_data[MODEM_INFO_IP_ADDRESS]->param_index,
				   ip_buf,
				   &len);
	if (err != 0) {
		return err;
	} else if (len >= sizeof(ip_buf)) {
		return -EMSGSIZE;
	}

	if (len == 0) {
		/* No IP address found */
		line_start_idx += line_len + 1;

		if (--current_ip_idx) {
			goto parse_line;
		} else {
			return strlen(out_buf);
		}
	}

	ip_buf[len] = '\0';

	/* For now get only IPv4 address if both v4 and v6 are given,
	 * discard IPv6 which are separated by a space.
	 */
	ip_v6_str = strstr(ip_buf, " ");

	if (ip_v6_str) {
		/* Discard IPv6 info and adjust length */
		*ip_v6_str = '\0';
		len = strlen(ip_buf);

		LOG_DBG("IPv6 address (%s) discarded", ip_v6_str + 1);
	}

	LOG_DBG("IP address found: %s", ip_buf);

	out_buf_len = strlen(out_buf);

	/* Write IP address to output buffer */
	err = snprintk(out_buf + out_buf_len,
		       out_buf_size - out_buf_len,
		       "%s%s",
		       first_address ? "" : IP_ADDR_SEPARATOR,
		       ip_buf);
	if ((err <= 0) || (err > (out_buf_size - out_buf_len))) {
		return -EMSGSIZE;
	}

	first_address = false;

	if (--current_ip_idx) {
		line_start_idx  += line_len + 1;
		goto parse_line;
	}

	return strlen(out_buf);
}

int modem_info_string_get(enum modem_info info, char *buf, const size_t buf_size)
{
	int err;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	uint16_t param_value;
	char *str_end = recv_buf;
	/* tracks length of buf when parsing multiple IP addresses */
	size_t out_buf_len = 0;
	/* return value indicating length of the string written to buf */
	size_t len = 0;
	/* The accumulated length written to buf when a command has more than
	 * one elements, such as multiple IP addresses.
	 */
	size_t accumulated_len = 0;

	if ((buf == NULL) || (buf_size == 0)) {
		return -EINVAL;
	}

	buf[0] = '\0';

	err = nrf_modem_at_cmd(recv_buf, CONFIG_MODEM_INFO_BUFFER_SIZE, modem_data[info]->cmd);
	if (err != 0) {
		return -EIO;
	}

	/* modem_info does not yet support array objects, so here we handle
	 * the supported bands independently as a string
	 */
	if (info == MODEM_INFO_SUP_BAND) {

		/* The list of supported bands is contained in parenthesis */
		char *str_begin = strchr(recv_buf, '(');

		str_end = strchr(recv_buf, ')');

		if (!str_begin || !str_end) {
			return -EFAULT;
		}

		/* terminate after the closing parenthesis */
		*(str_end + 1) = 0;

		len = strlen(str_begin);
		if (len >= buf_size) {
			return -EMSGSIZE;
		}

		strcpy(buf, str_begin);
		return len;
	}

	err = modem_info_parse(modem_data[info], recv_buf);
	if (err) {
		LOG_ERR("Unable to parse data: %d", err);
		return err;
	}

	if (info == MODEM_INFO_IP_ADDRESS) {
		return parse_ip_addresses(buf, buf_size, recv_buf);
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_NUM_INT) {
		err = at_params_unsigned_short_get(&m_param_list,
						    modem_data[info]->param_index,
						    &param_value);
		if (err) {
			LOG_ERR("Unable to obtain short: %d", err);
			return err;
		}

		len = snprintf(buf, buf_size, "%d", param_value);
		if ((len <= 0) || (len > buf_size)) {
			return -EMSGSIZE;
		}
	} else if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		len = buf_size - out_buf_len;
		err = at_params_string_get(&m_param_list,
					   modem_data[info]->param_index,
					   &buf[out_buf_len],
					   &len);
		if (err != 0) {
			return err;
		} else if (len >= buf_size) {
			return -EMSGSIZE;

		}

		accumulated_len += len;
		buf[accumulated_len] = '\0';
	}

	if (info == MODEM_INFO_ICCID) {
		flip_iccid_string(buf);

		/* Remove padding char from 19 digit (18+1) ICCIDs */
		if ((len == ICCID_LEN) &&
		   (buf[len - 1] == ICCID_PAD_CHAR)) {
			buf[len - 1] = '\0';
			--len;
		}
	}

	return len <= 0 ? -ENOTSUP : len;
}

static void modem_info_rsrp_subscribe_handler(const char *notif)
{
	int err;
	uint16_t param_value;

	const struct modem_info_data rsrp_notify_data = {
		.cmd		= AT_CMD_CESQ,
		.data_name	= RSRP_DATA_NAME,
		.param_index	= RSRP_NOTIFY_PARAM_INDEX,
		.param_count	= RSRP_NOTIFY_PARAM_COUNT,
		.data_type	= AT_PARAM_TYPE_NUM_INT,
	};

	err = modem_info_parse(&rsrp_notify_data, notif);
	if (err != 0) {
		LOG_ERR("modem_info_parse failed to parse "
			"CESQ notification, %d", err);
		return;
	}

	err = at_params_unsigned_short_get(&m_param_list,
					   rsrp_notify_data.param_index,
					   &param_value);
	if (err != 0) {
		LOG_ERR("Failed to obtain RSRP value, %d", err);
		return;
	}

	modem_info_rsrp_cb(param_value);
}

int modem_info_rsrp_register(rsrp_cb_t cb)
{
	modem_info_rsrp_cb = cb;

	at_monitor_resume(&modem_info_cesq_mon);

	if (nrf_modem_at_printf(AT_CMD_CESQ_ON) != 0) {
		return -EIO;
	}

	return 0;
}

int modem_info_get_fw_uuid(char *buf, size_t buf_size)
{
	int ret;

	if (buf == NULL || buf_size < FW_UUID_SIZE) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%XMODEMUUID",
				 "%%XMODEMUUID: %" STRINGIFY(FW_UUID_SIZE) "[^\r\n]",
				 buf);
	if (ret != 1) {
		LOG_ERR("Could not get FW ID, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_fw_version(char *buf, size_t buf_size)
{
	int ret;
	char format[SWVER_FMT_STR_SIZE];

	if (buf == NULL || buf_size < MODEM_INFO_FWVER_SIZE) {
		return -EINVAL;
	}

	sprintf(format, "%%%%SHORTSWVER: %%%d[^\r\n]", buf_size);

	ret = nrf_modem_at_scanf("AT%SHORTSWVER",
				 format,
				 buf);

	if (ret != 1) {
		LOG_ERR("Could not get FW version, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_hw_version(char *buf, uint8_t buf_size)
{
	int ret;
	char format[sizeof(HWVER_FMT_STR)];

	if ((buf == NULL) || (buf_size == 0)) {
		return -EINVAL;
	}

	sprintf(format, HWVER_FMT_STR, buf_size);

	ret = nrf_modem_at_scanf("AT%" HWVER_CMD_STR, format, buf);

	if (ret != 1) {
		LOG_ERR("Could not get HW version, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_svn(char *buf, size_t buf_size)
{
	int ret;

	if (buf == NULL || buf_size < SVN_SIZE) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT+CGSN=3",
				 "+CGSN: \"%" STRINGIFY(SVN_SIZE) "[^\"]",
				 buf);

	if (ret != 1) {
		LOG_ERR("Could not get SVN, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_batt_voltage(int *val)
{
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%XVBAT", "%%XVBAT: %d", val);

	if (ret != 1) {
		LOG_ERR("Could not get battery voltage, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_temperature(int *val)
{
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%XTEMP?", "%%XTEMP: %d", val);

	if (ret != 1) {
		LOG_ERR("Could not get temperature, error: %d", ret);
		return map_nrf_modem_at_scanf_error(ret);
	}
	return 0;
}

int modem_info_get_rsrp(int *val)
{
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT+CESQ",
				 "+CESQ: %*d,%*d,%*d,%*d,%*d,%d", val);

	if (ret != 1) {
		LOG_ERR("at_scanf_int failed");
		return map_nrf_modem_at_scanf_error(ret);
	}

	if (*val == CELL_RSRP_INVALID) {
		LOG_WRN("No valid RSRP");
		return -ENOENT;
	}

	*val = *val - RSRP_OFFSET_VAL;
	return 0;
}

int modem_info_init(void)
{
	int err = 0;

	if (m_param_list.params == NULL) {
		/* Init at_cmd_parser storage module */
		err = at_params_list_init(&m_param_list,
					  CONFIG_MODEM_INFO_MAX_AT_PARAMS_RSP);
	}

	return err;
}
