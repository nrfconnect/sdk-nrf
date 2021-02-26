/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/tls_credentials.h>
#include <net/net_ip.h>
#include <net/ftp_client.h>
#include <sys/ring_buffer.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_ftp.h"

LOG_MODULE_REGISTER(ftp, CONFIG_SLM_LOG_LEVEL);

#define INVALID_SEC_TAG		-1
#define FTP_MAX_HOSTNAME	64
#define FTP_MAX_USERNAME	32
#define FTP_MAX_PASSWORD	32
#define FTP_MAX_OPTION		32
#define FTP_MAX_FILEPATH	128

#define AT_FTP_STR		"AT#XFTP"

/*
 * Known limitation in this version
 */

/**@brief Socketopt operations. */
enum slm_ftp_operation {
	/* FTP Session Management */
	FTP_OP_CLOSE,
	FTP_OP_OPEN,
	FTP_OP_STATUS,
	FTP_OP_ASCII,
	FTP_OP_BINARY,
	/* FTP Directory Operation */
	FTP_OP_PWD,
	FTP_OP_LS,
	FTP_OP_CD,
	FTP_OP_MKDIR,
	FTP_OP_RMDIR,
	/* FTP File Operation */
	FTP_OP_RENAME,
	FTP_OP_DELETE,
	FTP_OP_GET,
	FTP_OP_PUT,
	FTP_OP_MAX
};

typedef int (*ftp_op_handler_t) (void);

typedef struct ftp_op_list {
	uint8_t op_code;
	char *op_str;
	ftp_op_handler_t handler;
} ftp_op_list_t;

/** forward declaration of cmd handlers **/
static int do_ftp_open(void);
static int do_ftp_status(void);
static int do_ftp_ascii(void);
static int do_ftp_binary(void);
static int do_ftp_close(void);

static int do_ftp_pwd(void);
static int do_ftp_ls(void);
static int do_ftp_cd(void);
static int do_ftp_mkdir(void);
static int do_ftp_rmdir(void);

static int do_ftp_rename(void);
static int do_ftp_delete(void);
static int do_ftp_get(void);
static int do_ftp_put(void);

/**@brief SLM AT Command list type. */
static ftp_op_list_t ftp_op_list[FTP_OP_MAX] = {
	{FTP_OP_CLOSE, "close", do_ftp_close},
	{FTP_OP_OPEN, "open", do_ftp_open},
	{FTP_OP_STATUS, "status", do_ftp_status},
	{FTP_OP_ASCII, "ascii", do_ftp_ascii},
	{FTP_OP_BINARY, "binary", do_ftp_binary},
	{FTP_OP_PWD, "pwd", do_ftp_pwd},
	{FTP_OP_CD, "cd", do_ftp_cd},
	{FTP_OP_LS, "ls", do_ftp_ls},
	{FTP_OP_MKDIR, "mkdir", do_ftp_mkdir},
	{FTP_OP_RMDIR, "rmdir", do_ftp_rmdir},
	{FTP_OP_RENAME, "rename", do_ftp_rename},
	{FTP_OP_DELETE, "delete", do_ftp_delete},
	{FTP_OP_GET, "get", do_ftp_get},
	{FTP_OP_PUT, "put", do_ftp_put},
};

RING_BUF_DECLARE(ftp_data_buf, CONFIG_AT_CMD_RESPONSE_MAX_LEN / 2);

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern struct k_work_q slm_work_q;

void ftp_ctrl_callback(const uint8_t *msg, uint16_t len)
{
	rsp_send((uint8_t *)msg, len);
}

static int ftp_data_save(uint8_t *data, uint32_t length)
{
	if (ring_buf_space_get(&ftp_data_buf) < length) {
		return -1; /* RX overrun */
	}

	return ring_buf_put(&ftp_data_buf, data, length);
}

static int ftp_data_send(void)
{
	uint32_t sz_send = 0;

	if (ring_buf_is_empty(&ftp_data_buf) == 0) {
		sz_send = ring_buf_get(&ftp_data_buf, rsp_buf, sizeof(rsp_buf));
		rsp_send(rsp_buf, sz_send);
		rsp_send("\r\n", 2);
	}

	return sz_send;
}

void ftp_data_callback(const uint8_t *msg, uint16_t len)
{
	if (slm_util_hex_check((uint8_t *)msg, len)) {
		int ret;
		int size = len * 2;

		ret = slm_util_htoa(msg, len, rsp_buf, size);
		if (ret > 0) {
			ftp_data_save(rsp_buf, ret);
		} else {
			LOG_WRN("hex convert error: %d", ret);
		}
	} else {
		ftp_data_save((uint8_t *)msg, len);
	}
}

/* AT#XFTP="open",<username>,<password>,<hostname>[,<port>[,<sec_tag>]] */
/* similar to ftp://<user>:<password>@<host>:<port> */
static int do_ftp_open(void)
{
	int ret = 0;
	char username[FTP_MAX_USERNAME];
	int sz_username = FTP_MAX_USERNAME;
	char password[FTP_MAX_PASSWORD];
	int sz_password = FTP_MAX_PASSWORD;
	char hostname[FTP_MAX_HOSTNAME];
	int sz_hostname = FTP_MAX_HOSTNAME;
	uint16_t port = CONFIG_SLM_FTP_SERVER_PORT;
	sec_tag_t sec_tag = INVALID_SEC_TAG;
	int param_count = at_params_valid_count_get(&at_param_list);

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, username, &sz_username);
	if (ret || strlen(username) == 0) {
		memcpy(username, CONFIG_SLM_FTP_USER_ANONYMOUS,
			sizeof(CONFIG_SLM_FTP_USER_ANONYMOUS) - 1);
		memcpy(password, CONFIG_SLM_FTP_PASSWORD_ANONYMOUS,
			sizeof(CONFIG_SLM_FTP_PASSWORD_ANONYMOUS) - 1);
	} else {
		ret = util_string_get(&at_param_list, 3, password,
					&sz_password);
		if (ret) {
			return -EINVAL;
		}
	}
	ret = util_string_get(&at_param_list, 4, hostname, &sz_hostname);
	if (ret) {
		return ret;
	}
	if (param_count > 5) {
		ret = at_params_short_get(&at_param_list, 5, &port);
		if (ret) {
			return ret;
		}
	}
	if (param_count > 6) {
		ret = at_params_int_get(&at_param_list, 6, &sec_tag);
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

/* AT#XFTP="close" */
static int do_ftp_close(void)
{
	int ret = ftp_close();

	return (ret == FTP_CODE_221) ? 0 : -1;
}

/* AT#XFTP="status" */
static int do_ftp_status(void)
{
	int ret = ftp_status();

	return (ret == FTP_CODE_211) ? 0 : -1;
}

/* AT#XFTP="ascii" */
static int do_ftp_ascii(void)
{
	int ret = ftp_type(FTP_TYPE_ASCII);

	return (ret == FTP_CODE_200) ? 0 : -1;
}

/* AT#XFTP="binary" */
static int do_ftp_binary(void)
{
	int ret = ftp_type(FTP_TYPE_BINARY);

	return (ret == FTP_CODE_200) ? 0 : -1;
}

/* AT#XFTP="pwd" */
static int do_ftp_pwd(void)
{
	int ret = ftp_pwd();

	return (ret == FTP_CODE_257) ? 0 : -1;
}

/* AT#XFTP="ls"[,<options>[,<folder>]] */
static int do_ftp_ls(void)
{
	int ret;
	char options[FTP_MAX_OPTION] = "";
	int sz_options = FTP_MAX_OPTION;
	char target[FTP_MAX_FILEPATH] = "";
	int sz_target = FTP_MAX_FILEPATH;
	int param_count = at_params_valid_count_get(&at_param_list);

	/* Parse AT command */
	if (param_count > 2) {
		ret = util_string_get(&at_param_list, 2,
				options, &sz_options);
		if (ret) {
			return ret;
		}
	}
	if (param_count > 3) {
		ret = util_string_get(&at_param_list, 3,
				target, &sz_target);
		if (ret) {
			return ret;
		}
	}

	ring_buf_reset(&ftp_data_buf);
	ret = ftp_list(options, target);
	if (ret == FTP_CODE_226) {
		ftp_data_send();
		return 0;
	} else {
		return -1;
	}
}

/* AT#XFTP="cd",<folder> */
static int do_ftp_cd(void)
{
	int ret;
	char folder[FTP_MAX_FILEPATH];
	int sz_folder = FTP_MAX_FILEPATH;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, folder, &sz_folder);
	if (ret) {
		return ret;
	}

	ret = ftp_cwd(folder);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="mkdir",<folder> */
static int do_ftp_mkdir(void)
{
	int ret;
	char folder[FTP_MAX_FILEPATH];
	int sz_folder = FTP_MAX_FILEPATH;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, folder, &sz_folder);
	if (ret) {
		return ret;
	}

	ret = ftp_mkd(folder);
	return (ret == FTP_CODE_257) ? 0 : -1;
}

/* AT#XFTP="rmdir",<folder> */
static int do_ftp_rmdir(void)
{
	int ret;
	char folder[FTP_MAX_FILEPATH];
	int sz_folder = FTP_MAX_FILEPATH;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, folder, &sz_folder);
	if (ret) {
		return ret;
	}

	ret = ftp_rmd(folder);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="rename",<file_old>,<file_new> */
static int do_ftp_rename(void)
{
	int ret;
	char file_old[FTP_MAX_FILEPATH];
	int sz_file_old = FTP_MAX_FILEPATH;
	char file_new[FTP_MAX_FILEPATH];
	int sz_file_new = FTP_MAX_FILEPATH;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, file_old, &sz_file_old);
	if (ret) {
		return ret;
	}
	ret = util_string_get(&at_param_list, 3, file_new, &sz_file_new);
	if (ret) {
		return ret;
	}

	ret = ftp_rename(file_old, file_new);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="delete",<file> */
static int do_ftp_delete(void)
{
	int ret;
	char file[FTP_MAX_FILEPATH];
	int sz_file = 128;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, file, &sz_file);
	if (ret) {
		return ret;
	}

	ret = ftp_delete(file);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="get",<file> */
static int do_ftp_get(void)
{
	int ret;
	char file[FTP_MAX_FILEPATH];
	int sz_file = FTP_MAX_FILEPATH;

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, file, &sz_file);
	if (ret) {
		return ret;
	}

	ring_buf_reset(&ftp_data_buf);
	ret = ftp_get(file);
	if (ret == FTP_CODE_226) {
		ftp_data_send();
		return 0;
	} else {
		return -1;
	}
}

/* AT#XFTP="put",<file>[<datatype>,<data>] */
static int do_ftp_put(void)
{
	int ret;
	char file[FTP_MAX_FILEPATH];
	int sz_file = FTP_MAX_FILEPATH;
	int param_count = at_params_valid_count_get(&at_param_list);

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, file, &sz_file);
	if (ret) {
		return ret;
	}

	if (param_count > 4) {
		uint16_t type;
		char data[NET_IPV4_MTU];
		int size;

		ret = at_params_short_get(&at_param_list, 3, &type);
		if (ret) {
			return ret;
		}
		size = NET_IPV4_MTU;
		ret = util_string_get(&at_param_list, 4, data, &size);
		if (ret) {
			return ret;
		}
		if (type == DATATYPE_HEXADECIMAL) {
			uint8_t data_hex[size / 2];

			ret = slm_util_atoh(data, size, data_hex, size / 2);
			if (ret > 0) {
				ret = ftp_put(file, data_hex, ret);
			}
		} else {
			ret = ftp_put(file, data, size);
		}
	} else {
		ret = ftp_put(file, NULL, 0);
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

/**@brief API to handle FTP AT command
 */
int slm_at_ftp_parse(const char *at_cmd)
{
	int ret = -ENOENT;
	char op_str[16];
	int size = 16;

	if (slm_util_cmd_casecmp(at_cmd, AT_FTP_STR)) {
		ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
		if (ret) {
			LOG_ERR("Failed to parse AT command %d", ret);
			return -EINVAL;
		}
		if (at_parser_cmd_type_get(at_cmd) != AT_CMD_TYPE_SET_COMMAND) {
			return -EINVAL;
		}
		ret = util_string_get(&at_param_list, 1, op_str, &size);
		if (ret) {
			return ret;
		}
		ret = -EINVAL;
		for (int i = 0; i < FTP_OP_MAX; i++) {
			if (slm_util_casecmp(op_str,
				ftp_op_list[i].op_str)) {
				ret = ftp_op_list[i].handler();
				break;
			}
		}
	}

	return ret;
}

/**@brief API to list FTP AT commands
 */
void slm_at_ftp_clac(void)
{
	sprintf(rsp_buf, "%s\r\n", AT_FTP_STR);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

/**@brief API to initialize FTP AT commands handler
 */
int slm_at_ftp_init(void)
{
	return ftp_init(ftp_ctrl_callback, ftp_data_callback);
}

/**@brief API to uninitialize FTP AT commands handler
 */
int slm_at_ftp_uninit(void)
{
	return ftp_uninit();
}
