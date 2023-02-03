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

#define TFTP_MAX_FILEPATH	128
#define TFTP_DEFAULT_PORT       69

/**@brief Socketopt operations. */
enum slm_ftp_operation {
	TFTP_DUMMY,
	TFTP_GET,	/** IPv4 TFTP Read */
	TFTP_PUT,	/** IPv4 TFTP Write */
	TFTP_GET6,	/** IPv6 TFTP Read */
	TFTP_PUT6,	/** IPv6 TFTP Write */
	/* count */
	TFTP_OP_MAX
};

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern uint8_t data_buf[SLM_MAX_MESSAGE_SIZE];

static int do_tftp_get(int family, const char *server, uint16_t port, const char *filepath,
			const char *mode)
{
	int ret;
	struct sockaddr sa = {
		.sa_family = AF_UNSPEC
	};

	ret = util_resolve_host(0, server, port, family, &sa);
	if (ret) {
		LOG_ERR("getaddrinfo() error: %s", gai_strerror(ret));
		return -EAGAIN;
	}

	struct tftpc client = {
		.user_buf = data_buf,
		.user_buf_size = sizeof(data_buf)
	};

	ret = tftp_get(&sa, &client, filepath, mode);
	if (ret != 0) {
		switch (ret) {
		case TFTPC_DUPLICATE_DATA:
			rsp_send("\r\n#XTFTP: -1, \"duplicate data\"\r\n");
			break;
		case TFTPC_BUFFER_OVERFLOW:
			rsp_send("\r\n#XTFTP: -2, \"buffer overflow\"\r\n");
			break;
		case TFTPC_REMOTE_ERROR:
			rsp_send("\r\n#XTFTP: -4, \"remote error\"\r\n");
			break;
		case TFTPC_RETRIES_EXHAUSTED:
			rsp_send("\r\n#XTFTP: -5, \"retries exhausted\"\r\n");
			break;
		case TFTPC_UNKNOWN_FAILURE:
		default:
			rsp_send("\r\n#XTFTP: -3, \"unknown failure\"\r\n");
			break;
		}
	} else {
		rsp_send("\r\n#XTFTP: %d,\"success\"\r\n", client.user_buf_size);
	}
	/* API does not add null-terminator */
	if (client.user_buf_size < sizeof(data_buf)) {
		data_buf[client.user_buf_size] = 0x00;
	}
	data_send(data_buf, strlen(data_buf));
	return ret;
}



/**@brief handle AT#XTFTP commands
 *  AT#XTFTP=<op>,<url>,<port>,<file_path>[,<mode>]
 *  AT#XTFTP? READ is not supported
 *  AT#XTFTP=?
 */
int handle_at_tftp(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	int param_count = at_params_valid_count_get(&at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == TFTP_GET || op == TFTP_GET6) {
			uint16_t port;
			char url[SLM_MAX_URL];
			char filepath[TFTP_MAX_FILEPATH];
			char mode[16];   /** "netascii", "octet", "mail" */
			int size;

			size = sizeof(url);
			err = util_string_get(&at_param_list, 2, url, &size);
			if (err) {
				return err;
			}
			(void)at_params_unsigned_short_get(&at_param_list, 3, &port);
			if (port == 0) {
				port = TFTP_DEFAULT_PORT;
			}
			size = sizeof(filepath);
			err = util_string_get(&at_param_list, 4, filepath, &size);
			if (err) {
				return err;
			}
			if (param_count > 5) {
				size = sizeof(mode);
				err = util_string_get(&at_param_list, 5, mode, &size);
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
			} else {
				err = do_tftp_get(AF_INET6, url, port, filepath, mode);
			}
		} else if (op == TFTP_PUT || op == TFTP_PUT6) {
			LOG_WRN("TFTP Put not supported");
			err = -ENOSYS;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XTFTP: (%d,%d,%d,%d),<url>,<port>,<file_path>,<mode>\r\n",
			TFTP_GET, TFTP_PUT, TFTP_GET6, TFTP_PUT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}
