/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/tftp.h>
#include "slm_util.h"
#include "slm_at_host.h"

LOG_MODULE_REGISTER(slm_tftp, CONFIG_SLM_LOG_LEVEL);

/**@brief Socketopt operations. */
enum slm_ftp_operation {
	TFTP_DUMMY,
	TFTP_GET,	/** IPv4 TFTP Read */
	TFTP_PUT,	/** IPv4 TFTP Write */
	TFTP_GET6,	/** IPv6 TFTP Read */
	TFTP_PUT6,	/** IPv6 TFTP Write */
	TFTP_OP_MAX
};

void tftp_callback(const struct tftp_evt *evt)
{
	switch (evt->type) {
	case TFTP_EVT_DATA:
		if (evt->param.data.len > 0) {
			memcpy(slm_data_buf, evt->param.data.data_ptr, evt->param.data.len);
			data_send(slm_data_buf, evt->param.data.len);
		}
		break;

	case TFTP_EVT_ERROR:
		LOG_ERR("ERROR: %d, %s", evt->param.error.code, evt->param.error.msg);
		break;

	default:
		break;
	}
}

static int do_tftp_get(int family, const char *server, uint16_t port, const char *filepath,
			const char *mode)
{
	int ret;
	struct tftpc client = {
		.callback = tftp_callback
	};
	ret = util_resolve_host(0, server, port, family,
		Z_LOG_OBJECT_PTR(slm_tftp), &client.server);
	if (ret) {
		return -EAGAIN;
	}

	ret = tftp_get(&client, filepath, mode);
	if (ret < 0) {
		switch (ret) {
		case TFTPC_DUPLICATE_DATA:
			rsp_send("\r\n#XTFTP: %d, \"duplicate data\"\r\n", ret);
			break;
		case TFTPC_BUFFER_OVERFLOW:
			rsp_send("\r\n#XTFTP: %d, \"buffer overflow\"\r\n", ret);
			break;
		case TFTPC_UNKNOWN_FAILURE:
			rsp_send("\r\n#XTFTP: %d, \"unknown failure\"\r\n", ret);
			break;
		case TFTPC_REMOTE_ERROR:
			rsp_send("\r\n#XTFTP: %d, \"remote error\"\r\n", ret);
			break;
		case TFTPC_RETRIES_EXHAUSTED:
			rsp_send("\r\n#XTFTP: %d, \"retries exhausted\"\r\n", ret);
			break;
		default:
			rsp_send("\r\n#XTFTP: %d, \"other failure\"\r\n", ret);
			break;
		}
	} else {
		rsp_send("\r\n#XTFTP: %d,\"success\"\r\n", ret);
		ret = 0;
	}

	return ret;
}

static int do_tftp_put(int family, const char *server, uint16_t port, const char *filepath,
			const char *mode, const uint8_t *data, size_t datalen)
{
	int ret;
	struct tftpc client = {
		.callback = tftp_callback
	};

	ret = util_resolve_host(0, server, port, family,
		Z_LOG_OBJECT_PTR(slm_tftp), &client.server);
	if (ret) {
		return -EAGAIN;
	}

	ret = tftp_put(&client, filepath, mode, data, datalen);
	if (ret < 0) {
		switch (ret) {
		case TFTPC_DUPLICATE_DATA:
			rsp_send("\r\n#XTFTP: %d, \"duplicate data\"\r\n", ret);
			break;
		case TFTPC_BUFFER_OVERFLOW:
			rsp_send("\r\n#XTFTP: %d, \"unknown overflow\"\r\n", ret);
			break;
		case TFTPC_UNKNOWN_FAILURE:
			rsp_send("\r\n#XTFTP: %d, \"unknown failure\"\r\n", ret);
			break;
		case TFTPC_REMOTE_ERROR:
			rsp_send("\r\n#XTFTP: %d, \"remote error\"\r\n", ret);
			break;
		case TFTPC_RETRIES_EXHAUSTED:
			rsp_send("\r\n#XTFTP: %d, \"retries exhausted\"\r\n", ret);
			break;
		default:
			rsp_send("\r\n#XTFTP: %d, \"other failure\"\r\n", ret);
			break;
		}
	} else {
		rsp_send("\r\n#XTFTP: %d,\"success\"\r\n", ret);
		ret = 0;
	}

	return ret;
}

/**@brief handle AT#XTFTP commands
 *  AT#XTFTP=<op>,<url>,<port>,<file_path>[,<mode>,<data>]
 *  AT#XTFTP? READ is not supported
 *  AT#XTFTP=?
 */
int handle_at_tftp(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t port;
	char url[SLM_MAX_URL];
	char filepath[SLM_MAX_FILEPATH];
	char mode[16];   /** "netascii", "octet", "mail" */
	int size;
	int param_count = at_params_valid_count_get(&slm_at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err) {
			return err;
		}

		size = sizeof(url);
		err = util_string_get(&slm_at_param_list, 2, url, &size);
		if (err) {
			return err;
		}
		err = at_params_unsigned_short_get(&slm_at_param_list, 3, &port);
		if (err) {
			return err;
		}
		size = sizeof(filepath);
		err = util_string_get(&slm_at_param_list, 4, filepath, &size);
		if (err) {
			return err;
		}
		if (param_count > 5) {
			size = sizeof(mode);
			err = util_string_get(&slm_at_param_list, 5, mode, &size);
			if (err) {
				return err;
			}
			if (!slm_util_cmd_casecmp(mode, "netascii") &&
			    !slm_util_cmd_casecmp(mode, "octet") &&
			    !slm_util_cmd_casecmp(mode, "mail")) {
				return -EINVAL;
			}
		} else {
			strcpy(mode, "octet");
		}

		if (op == TFTP_GET) {
			err = do_tftp_get(AF_INET, url, port, filepath, mode);
		} else if (op == TFTP_GET6) {
			err = do_tftp_get(AF_INET6, url, port, filepath, mode);
		} else if (op == TFTP_PUT || op == TFTP_PUT6) {
			uint8_t data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};

			size = sizeof(data);
			err = util_string_get(&slm_at_param_list, 6, data, &size);
			if (err) {
				return err;
			}
			if (op == TFTP_PUT) {
				err = do_tftp_put(AF_INET, url, port, filepath, mode, data, size);
			} else {
				err = do_tftp_put(AF_INET6, url, port, filepath, mode, data, size);
			}
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XTFTP: (%d,%d,%d,%d),<url>,<port>,<file_path>,<mode>,<data>\r\n",
			TFTP_GET, TFTP_PUT, TFTP_GET6, TFTP_PUT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}
