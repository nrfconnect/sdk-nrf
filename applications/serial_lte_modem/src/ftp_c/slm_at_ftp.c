/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>
#include <net/ftp_client.h>
#include <zephyr/sys/ring_buffer.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_ftp.h"

LOG_MODULE_REGISTER(slm_ftp, CONFIG_SLM_LOG_LEVEL);

#define FTP_MAX_OPTION		32

#define FTP_USER_ANONYMOUS      "anonymous"
#define FTP_PASSWORD_ANONYMOUS  "anonymous@example.com"
#define FTP_DEFAULT_PORT        21

static bool ftp_verbose_on;
static char filepath[SLM_MAX_FILEPATH];
static int sz_filepath;
static int (*ftp_data_mode_handler)(const uint8_t *data, int len, uint8_t flags);

RING_BUF_DECLARE(ftp_data_buf, SLM_MAX_MESSAGE_SIZE);

void ftp_ctrl_callback(const uint8_t *msg, uint16_t len)
{
	char code_str[4];  /* Proprietary code 900 ~ 999 */
	int code;

	strncpy(code_str, msg, sizeof(code_str) - 1);
	code_str[sizeof(code_str) - 1] = '\0';
	code = atoi(code_str);
	if (FTP_PROPRIETARY(code)) {
		switch (code) {
		case FTP_CODE_901:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -ECONNRESET);
			}
			break;
		case FTP_CODE_902:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -ECONNABORTED);
			}
			break;
		case FTP_CODE_903:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -EIO);
			}
			break;
		case FTP_CODE_904:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -EAGAIN);
			}
			break;
		case FTP_CODE_905:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -ENETDOWN);
			}
			break;
		default:
			if (ftp_verbose_on) {
				rsp_send("\r\n#XFTP: %d,\"disconnected\"\r\n", -ENOEXEC);
			}
			break;
		}
		if (ftp_data_mode_handler && exit_datamode_handler(-EAGAIN)) {
			ftp_data_mode_handler = NULL;
		}
		return;
	}

	if (ftp_verbose_on) {
		data_send((uint8_t *)msg, len);
	}
}

static void ftp_data_save(uint8_t *data, uint32_t length)
{
	const int ret = ring_buf_put(&ftp_data_buf, data, length);

	if (ret != length) {
		LOG_WRN("FTP buffer overflow (%u bytes dropped).", length - ret);
	}
}

static void ftp_data_send(void)
{
	uint32_t sz_send;

	while ((sz_send = ring_buf_get(&ftp_data_buf, slm_data_buf, sizeof(slm_data_buf)))) {
		data_send(slm_data_buf, sz_send);
	}
}

void ftp_data_callback(const uint8_t *msg, uint16_t len)
{
	ftp_data_save((uint8_t *)msg, len);
}

/* AT#XFTP="open",<username>,<password>,<hostname>[,<port>[,<sec_tag>]] */
/* similar to ftp://<user>:<password>@<host>:<port> */
SLM_AT_CMD_CUSTOM(xftp_open, "AT#XFTP=\"open\"", do_ftp_open);
static int do_ftp_open(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
		       uint32_t param_count)
{
	int ret = 0;
	char username[SLM_MAX_USERNAME] = "";  /* DO initialize, in case of login error */
	int sz_username = sizeof(username);
	char password[SLM_MAX_PASSWORD] = "";  /* DO initialize, in case of login error */
	int sz_password = sizeof(password);
	char hostname[SLM_MAX_URL];
	int sz_hostname = sizeof(hostname);
	uint16_t port = FTP_DEFAULT_PORT;
	sec_tag_t sec_tag = INVALID_SEC_TAG;

	/* Parse AT command */
	ret = util_string_get(parser, 2, username, &sz_username);
	if (ret || strlen(username) == 0) {
		strcpy(username, FTP_USER_ANONYMOUS);
		strcpy(password, FTP_PASSWORD_ANONYMOUS);
	} else {
		ret = util_string_get(parser, 3, password, &sz_password);
		if (ret) {
			return -EINVAL;
		}
	}
	ret = util_string_get(parser, 4, hostname, &sz_hostname);
	if (ret) {
		return ret;
	}
	if (param_count > 5) {
		ret = at_parser_num_get(parser, 5, &port);
		if (ret) {
			return ret;
		}
	}
	if (param_count > 6) {
		ret = at_parser_num_get(parser, 6, &sec_tag);
		if (ret) {
			return ret;
		}
	}

	/* FTP open */
	ret = ftp_open(hostname, port, sec_tag);
	if (ret != FTP_CODE_200) {
		return -ENETUNREACH;
	}

	/* FTP login */
	ret = ftp_login(username, password);
	if (ret != FTP_CODE_230) {
		return -EACCES;
	}

	return 0;
}

SLM_AT_CMD_CUSTOM(xftp_close, "AT#XFTP=\"close\"", do_ftp_close);
static int do_ftp_close(enum at_parser_cmd_type, struct at_parser *, uint32_t)
{
	int ret = ftp_close();

	return (ret == FTP_CODE_221) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_status, "AT#XFTP=\"status\"", do_ftp_status);
static int do_ftp_status(enum at_parser_cmd_type, struct at_parser *, uint32_t)
{
	int ret = ftp_status();

	return (ret == FTP_CODE_211) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_ascii, "AT#XFTP=\"ascii\"", do_ftp_ascii);
static int do_ftp_ascii(enum at_parser_cmd_type, struct at_parser *, uint32_t)
{
	int ret = ftp_type(FTP_TYPE_ASCII);

	return (ret == FTP_CODE_200) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_binary, "AT#XFTP=\"binary\"", do_ftp_binary);
static int do_ftp_binary(enum at_parser_cmd_type, struct at_parser *, uint32_t)
{
	int ret = ftp_type(FTP_TYPE_BINARY);

	return (ret == FTP_CODE_200) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_verbose, "AT#XFTP=\"verbose\"", do_ftp_verbose);
static int do_ftp_verbose(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret = 0;
	char vb_mode[4];
	int sz_mode = 4;

	ret = util_string_get(parser, 2, vb_mode, &sz_mode);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(vb_mode, "ON")) {
		ftp_verbose_on = true;
		rsp_send("\r\nVerbose mode on\r\n");
	} else if (slm_util_casecmp(vb_mode, "OFF")) {
		ftp_verbose_on = false;
		rsp_send("\r\nVerbose mode off\r\n");
	} else {
		return -EINVAL;
	}

	return 0;
}

SLM_AT_CMD_CUSTOM(xftp_pwd, "AT#XFTP=\"pwd\"", do_ftp_pwd);
static int do_ftp_pwd(enum at_parser_cmd_type, struct at_parser *, uint32_t)
{
	int ret = ftp_pwd();

	return (ret == FTP_CODE_257) ? 0 : -1;
}

/* AT#XFTP="ls"[,<options>[,<folder>]] */
SLM_AT_CMD_CUSTOM(xftp_ls, "AT#XFTP=\"ls\"", do_ftp_ls);
static int do_ftp_ls(enum at_parser_cmd_type, struct at_parser *parser, uint32_t param_count)
{
	int ret;
	char options[FTP_MAX_OPTION] = "";
	int sz_options = FTP_MAX_OPTION;

	if (param_count > 2) {
		ret = util_string_get(parser, 2, options, &sz_options);
		if (ret) {
			return ret;
		}
	}
	memset(filepath, 0x00, SLM_MAX_FILEPATH);
	sz_filepath = SLM_MAX_FILEPATH;
	if (param_count > 3) {
		ret = util_string_get(parser, 3, filepath, &sz_filepath);
		if (ret) {
			return ret;
		}
	}

	ring_buf_reset(&ftp_data_buf);
	ret = ftp_list(options, filepath);
	if (ret == FTP_CODE_226) {
		ftp_data_send();
		return 0;
	} else {
		return -1;
	}
}

SLM_AT_CMD_CUSTOM(xftp_cd, "AT#XFTP=\"cd\"", do_ftp_cd);
static int do_ftp_cd(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_cwd(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_mkdir, "AT#XFTP=\"mkdir\"", do_ftp_mkdir);
static int do_ftp_mkdir(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_mkd(filepath);
	return (ret == FTP_CODE_257) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_rmdir, "AT#XFTP=\"rmdir\"", do_ftp_rmdir);
static int do_ftp_rmdir(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_rmd(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_rename, "AT#XFTP=\"rename\"", do_ftp_rename);
static int do_ftp_rename(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;
	char file_new[SLM_MAX_FILEPATH];
	int sz_file_new = SLM_MAX_FILEPATH;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}
	ret = util_string_get(parser, 3, file_new, &sz_file_new);
	if (ret) {
		return ret;
	}

	ret = ftp_rename(filepath, file_new);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_delete, "AT#XFTP=\"delete\"", do_ftp_delete);
static int do_ftp_delete(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_delete(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_get, "AT#XFTP=\"get\"", do_ftp_get);
static int do_ftp_get(enum at_parser_cmd_type, struct at_parser *parser, uint32_t)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ring_buf_reset(&ftp_data_buf);
	ret = ftp_get(filepath);
	if (ret == FTP_CODE_226) {
		ftp_data_send();
		return 0;
	} else {
		return -1;
	}
}

static int ftp_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if (ftp_data_mode_handler) {
			ret = ftp_data_mode_handler(data, len, flags);
			LOG_INF("datamode send: %d", ret);
		} else {
			LOG_ERR("no datamode send handler");
		}
	} else if (op == DATAMODE_EXIT) {
		ftp_data_mode_handler = NULL;
	}

	return ret;
}

/* FTP PUT data mode handler */
static int ftp_put_handler(const uint8_t *data, int len, uint8_t flags)
{
	int ret = -1;

	if (strlen(filepath) > 0 && data != NULL) {
		ret = ftp_put(filepath, data, len, FTP_PUT_NORMAL);
		if (ret != FTP_CODE_226) {
			exit_datamode_handler(-EAGAIN);
		} else if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
		}
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_put, "AT#XFTP=\"put\"", do_ftp_put);
static int do_ftp_put(enum at_parser_cmd_type, struct at_parser *parser,
		      uint32_t param_count)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	if (param_count == 3) {
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}
		ftp_data_mode_handler = ftp_put_handler;
	} else {
		char data[SLM_MAX_PAYLOAD_SIZE] = {0};
		int size = sizeof(data);
		int err;

		err = util_string_get(parser, 3, data, &size);
		if (err) {
			return err;
		}
		err = ftp_put(filepath, data, size, FTP_PUT_NORMAL);
		ret = (err == FTP_CODE_226) ? 0 : -1;
	}

	return ret;
}

/* FTP UPUT data mode handler */
static int ftp_uput_handler(const uint8_t *data, int len, uint8_t flags)
{
	int ret = -1;

	if (data != NULL) {
		ret = ftp_put(NULL, data, len, FTP_PUT_UNIQUE);
		if (ret != FTP_CODE_226) {
			exit_datamode_handler(-EAGAIN);
		} else if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
		}
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_uput, "AT#XFTP=\"uput\"", do_ftp_uput);
static int do_ftp_uput(enum at_parser_cmd_type, struct at_parser *parser,
		       uint32_t param_count)
{
	int ret = -1;

	if (param_count == 2) {
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}
		ftp_data_mode_handler = ftp_uput_handler;
	} else {
		char data[SLM_MAX_PAYLOAD_SIZE] = {0};
		int size = sizeof(data);
		int err;

		err = util_string_get(parser, 2, data, &size);
		if (err) {
			return err;
		}
		err = ftp_put(NULL, data, size, FTP_PUT_UNIQUE);
		ret = (err == FTP_CODE_226) ? 0 : -1;
	}

	return ret;
}

/* FTP MPUT data mode handler */
static int ftp_mput_handler(const uint8_t *data, int len, uint8_t flags)
{
	int ret = -1;

	ARG_UNUSED(flags);

	if (strlen(filepath) > 0 && data != NULL) {
		ret = ftp_put(filepath, data, len, FTP_PUT_APPEND);
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

SLM_AT_CMD_CUSTOM(xftp_mput, "AT#XFTP=\"mput\"", do_ftp_mput);
static int do_ftp_mput(enum at_parser_cmd_type, struct at_parser *parser,
		       uint32_t param_count)
{
	int ret;

	sz_filepath = SLM_MAX_FILEPATH;
	ret = util_string_get(parser, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	if (param_count == 3) {
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}
		ftp_data_mode_handler = ftp_mput_handler;
	} else {
		char data[SLM_MAX_PAYLOAD_SIZE] = {0};
		int size = sizeof(data);
		int err;

		err = util_string_get(parser, 3, data, &size);
		if (err) {
			return err;
		}
		err = ftp_put(filepath, data, size, FTP_PUT_APPEND);
		ret = (err == FTP_CODE_226) ? 0 : -1;
	}

	return ret;
}

/**@brief API to initialize FTP AT commands handler
 */
int slm_at_ftp_init(void)
{
	ftp_verbose_on = true;
	ftp_data_mode_handler = NULL;

	return ftp_init(ftp_ctrl_callback, ftp_data_callback);
}

/**@brief API to uninitialize FTP AT commands handler
 */
int slm_at_ftp_uninit(void)
{
	return ftp_uninit();
}
