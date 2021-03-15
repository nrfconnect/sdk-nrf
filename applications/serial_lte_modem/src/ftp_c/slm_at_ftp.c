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

/**@brief Socketopt operations. */
enum slm_ftp_operation {
	/* FTP Session Management */
	FTP_OP_CLOSE,
	FTP_OP_OPEN,
	FTP_OP_STATUS,
	FTP_OP_ASCII,
	FTP_OP_BINARY,
	FTP_OP_VERBOSE,
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
	FTP_OP_UPUT,
	FTP_OP_MPUT,
	/* count */
	FTP_OP_MAX
};

typedef int (*ftp_op_handler_t) (void);

typedef struct ftp_op_list {
	uint8_t op_code;
	char *op_str;
	ftp_op_handler_t handler;
} ftp_op_list_t;

static bool ftp_verbose_on;
static char filepath[FTP_MAX_FILEPATH];
static int sz_filepath;
static int (*ftp_data_mode_handler)(const uint8_t *data, int len);

/** forward declaration of cmd handlers **/
static int do_ftp_close(void);
static int do_ftp_open(void);
static int do_ftp_status(void);
static int do_ftp_ascii(void);
static int do_ftp_binary(void);
static int do_ftp_verbose(void);

static int do_ftp_pwd(void);
static int do_ftp_ls(void);
static int do_ftp_cd(void);
static int do_ftp_mkdir(void);
static int do_ftp_rmdir(void);

static int do_ftp_rename(void);
static int do_ftp_delete(void);
static int do_ftp_get(void);
static int do_ftp_put(void);
static int do_ftp_uput(void);
static int do_ftp_mput(void);

/**@brief SLM AT Command list type. */
static ftp_op_list_t ftp_op_list[FTP_OP_MAX] = {
	{FTP_OP_CLOSE, "close", do_ftp_close},
	{FTP_OP_OPEN, "open", do_ftp_open},
	{FTP_OP_STATUS, "status", do_ftp_status},
	{FTP_OP_ASCII, "ascii", do_ftp_ascii},
	{FTP_OP_BINARY, "binary", do_ftp_binary},
	{FTP_OP_VERBOSE, "verbose", do_ftp_verbose},
	{FTP_OP_PWD, "pwd", do_ftp_pwd},
	{FTP_OP_CD, "cd", do_ftp_cd},
	{FTP_OP_LS, "ls", do_ftp_ls},
	{FTP_OP_MKDIR, "mkdir", do_ftp_mkdir},
	{FTP_OP_RMDIR, "rmdir", do_ftp_rmdir},
	{FTP_OP_RENAME, "rename", do_ftp_rename},
	{FTP_OP_DELETE, "delete", do_ftp_delete},
	{FTP_OP_GET, "get", do_ftp_get},
	{FTP_OP_PUT, "put", do_ftp_put},
	{FTP_OP_UPUT, "uput", do_ftp_uput},
	{FTP_OP_MPUT, "mput", do_ftp_mput},
};

RING_BUF_DECLARE(ftp_data_buf, CONFIG_SLM_SOCKET_RX_MAX * 2);

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);
int enter_datamode(slm_datamode_handler_t handler);
bool exit_datamode(void);
bool check_uart_flowcontrol(void);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern uint8_t rx_data[CONFIG_SLM_SOCKET_RX_MAX];

void ftp_ctrl_callback(const uint8_t *msg, uint16_t len)
{
	char code_str[4];  /* Proprietary code 900 ~ 999 */
	int code;

	strncpy(code_str, msg, 3);
	code = atoi(code_str);
	if (FTP_PROPRIETARY(code)) {
		switch (code) {
		case FTP_CODE_901:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -ECONNRESET);
			break;
		case FTP_CODE_902:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -ECONNABORTED);
			break;
		case FTP_CODE_903:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -EIO);
			break;
		case FTP_CODE_904:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -EAGAIN);
			break;
		case FTP_CODE_905:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -ENETDOWN);
			break;
		default:
			sprintf(rsp_buf, "\r\n#XFTP: %d,\"disconnected\"\r\n", -ENOEXEC);
			break;
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
		if (ftp_data_mode_handler && exit_datamode()) {
			sprintf(rsp_buf, "\r\n#XFTP: 0,\"datamode\"\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
			ftp_data_mode_handler = NULL;
		}
	}

	if (ftp_verbose_on) {
		rsp_send((uint8_t *)msg, len);
	}
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
	char username[FTP_MAX_USERNAME] = "";  /* DO initialize, in case of login error */
	int sz_username = FTP_MAX_USERNAME;
	char password[FTP_MAX_PASSWORD] = "";  /* DO initialize, in case of login error */
	int sz_password = FTP_MAX_PASSWORD;
	char hostname[FTP_MAX_HOSTNAME];
	int sz_hostname = FTP_MAX_HOSTNAME;
	int32_t port = CONFIG_SLM_FTP_SERVER_PORT;
	sec_tag_t sec_tag = INVALID_SEC_TAG;
	int param_count = at_params_valid_count_get(&at_param_list);

	/* Parse AT command */
	ret = util_string_get(&at_param_list, 2, username, &sz_username);
	if (ret || strlen(username) == 0) {
		strcpy(username, CONFIG_SLM_FTP_USER_ANONYMOUS);
		strcpy(password, CONFIG_SLM_FTP_PASSWORD_ANONYMOUS);
	} else {
		ret = util_string_get(&at_param_list, 3, password, &sz_password);
		if (ret) {
			return -EINVAL;
		}
	}
	ret = util_string_get(&at_param_list, 4, hostname, &sz_hostname);
	if (ret) {
		return ret;
	}
	if (param_count > 5) {
		ret = at_params_int_get(&at_param_list, 5, &port);
		if (ret) {
			return ret;
		}
	}
	if (!check_port_range(port)) {
		LOG_ERR("Invalid port");
		return -EINVAL;
	}
	if (param_count > 6) {
		ret = at_params_int_get(&at_param_list, 6, &sec_tag);
		if (ret) {
			return ret;
		}
	}

	/* FTP open */
	ret = ftp_open(hostname, (uint16_t)port, sec_tag);
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

/* AT#XFTP="verbose","on|off" */
static int do_ftp_verbose(void)
{
	int ret = 0;
	char vb_mode[4];
	int sz_mode = 4;

	ret = util_string_get(&at_param_list, 2, vb_mode, &sz_mode);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(vb_mode, "ON")) {
		ftp_verbose_on = true;
		sprintf(rsp_buf, "\r\nVerbose mode on\r\n");
	} else if (slm_util_cmd_casecmp(vb_mode, "OFF")) {
		ftp_verbose_on = false;
		sprintf(rsp_buf, "\r\nVerbose mode off\r\n");
	} else {
		return -EINVAL;
	}
	rsp_send(rsp_buf, strlen(rsp_buf));

	return 0;
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
	int param_count = at_params_valid_count_get(&at_param_list);

	if (param_count > 2) {
		ret = util_string_get(&at_param_list, 2, options, &sz_options);
		if (ret) {
			return ret;
		}
	}
	memset(filepath, 0x00, FTP_MAX_FILEPATH);
	sz_filepath = FTP_MAX_FILEPATH;
	if (param_count > 3) {
		ret = util_string_get(&at_param_list, 3, filepath, &sz_filepath);
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

/* AT#XFTP="cd",<folder> */
static int do_ftp_cd(void)
{
	int ret;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_cwd(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="mkdir",<folder> */
static int do_ftp_mkdir(void)
{
	int ret;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_mkd(filepath);
	return (ret == FTP_CODE_257) ? 0 : -1;
}

/* AT#XFTP="rmdir",<folder> */
static int do_ftp_rmdir(void)
{
	int ret;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_rmd(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="rename",<file_old>,<file_new> */
static int do_ftp_rename(void)
{
	int ret;
	char file_new[FTP_MAX_FILEPATH];
	int sz_file_new = FTP_MAX_FILEPATH;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}
	ret = util_string_get(&at_param_list, 3, file_new, &sz_file_new);
	if (ret) {
		return ret;
	}

	ret = ftp_rename(filepath, file_new);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="delete",<file> */
static int do_ftp_delete(void)
{
	int ret;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = ftp_delete(filepath);
	return (ret == FTP_CODE_250) ? 0 : -1;
}

/* AT#XFTP="get",<file> */
static int do_ftp_get(void)
{
	int ret;

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
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

static int ftp_datamode_callback(uint8_t op, const uint8_t *data, int len)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if (ftp_data_mode_handler) {
			ret = ftp_data_mode_handler(data, len);
			LOG_DBG("datamode send: %d", ret);
		} else {
			LOG_ERR("no datamode send handler");
		}
	} else if (op == DATAMODE_EXIT) {
		ftp_data_mode_handler = NULL;
	}

	return ret;
}

/* FTP PUT data mode handler */
static int ftp_put_handler(const uint8_t *data, int len)
{
	int ret = -1;

	if (strlen(filepath) > 0 && data != NULL) {
		ret = ftp_put(filepath, data, len, FTP_PUT_NORMAL);
	}

	if (exit_datamode()) {
		sprintf(rsp_buf, "\r\n#XFTP: 0,\"datamode\"\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		ftp_data_mode_handler = NULL;
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

/* AT#XFTP="put",<file>,<datatype>[,<data>] */
static int do_ftp_put(void)
{
	int ret;
	uint16_t datatype;
	int param_count = at_params_valid_count_get(&at_param_list);

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = at_params_short_get(&at_param_list, 3, &datatype);
	if (ret) {
		return ret;
	}

	if (datatype == DATATYPE_ARBITRARY) {
#if defined(CONFIG_SLM_DATAMODE_HWFC)
		if (!check_uart_flowcontrol()) {
			LOG_ERR("Data mode requires HWFC.");
			return -EINVAL;
		}
#endif
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}

		ftp_data_mode_handler = ftp_put_handler;
		return 0;
	}

	if (param_count > 4) {
		int size = CONFIG_SLM_SOCKET_RX_MAX;

		ret = util_string_get(&at_param_list, 4, rx_data, &size);
		if (ret) {
			return ret;
		}
		if (datatype == DATATYPE_HEXADECIMAL) {
			uint8_t data_hex[size / 2];

			ret = slm_util_atoh(rx_data, size, data_hex, size / 2);
			if (ret > 0) {
				ret = ftp_put(filepath, data_hex, ret, FTP_PUT_NORMAL);
			}
		} else {
			ret = ftp_put(filepath, rx_data, size, FTP_PUT_NORMAL);
		}

		ret = (ret == FTP_CODE_226) ? 0 : -1;
	} else {
		/* must carry data to send */
		ret = -EINVAL;
	}

	return ret;
}

/* FTP UPUT data mode handler */
static int ftp_uput_handler(const uint8_t *data, int len)
{
	int ret = -1;

	if (data != NULL) {
		ret = ftp_put(NULL, data, len, FTP_PUT_UNIQUE);
	}

	if (exit_datamode()) {
		sprintf(rsp_buf, "\r\n#XFTP: 0,\"datamode\"\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		ftp_data_mode_handler = NULL;
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

/* AT#XFTP="uput",<datatype>[,<data>] */
static int do_ftp_uput(void)
{
	int ret;
	uint16_t datatype;
	int param_count = at_params_valid_count_get(&at_param_list);

	ret = at_params_short_get(&at_param_list, 2, &datatype);
	if (ret) {
		return ret;
	}

	if (datatype == DATATYPE_ARBITRARY) {
#if defined(CONFIG_SLM_DATAMODE_HWFC)
		if (!check_uart_flowcontrol()) {
			LOG_ERR("Data mode requires HWFC.");
			return -EINVAL;
		}
#endif
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}

		ftp_data_mode_handler = ftp_uput_handler;
		return 0;
	}

	if (param_count > 3) {
		int size = CONFIG_SLM_SOCKET_RX_MAX;

		ret = util_string_get(&at_param_list, 3, rx_data, &size);
		if (ret) {
			return ret;
		}
		if (datatype == DATATYPE_HEXADECIMAL) {
			uint8_t data_hex[size / 2];

			ret = slm_util_atoh(rx_data, size, data_hex, size / 2);
			if (ret > 0) {
				ret = ftp_put(NULL, data_hex, ret, FTP_PUT_UNIQUE);
			}
		} else {
			ret = ftp_put(NULL, rx_data, size, FTP_PUT_UNIQUE);
		}

		ret = (ret == FTP_CODE_226) ? 0 : -1;
	} else {
		/* must carry data to send */
		ret = -EINVAL;
	}

	return ret;
}

/* FTP UPUT data mode handler */
static int ftp_mput_handler(const uint8_t *data, int len)
{
	int ret = -1;

	if (strlen(filepath) > 0 && data != NULL) {
		ret = ftp_put(filepath, data, len, FTP_PUT_APPEND);
	}

	return (ret == FTP_CODE_226) ? 0 : -1;
}

/* AT#XFTP="mput",<file>,<datatype>[,<data>] */
static int do_ftp_mput(void)
{
	int ret;
	uint16_t datatype;
	int param_count = at_params_valid_count_get(&at_param_list);

	sz_filepath = FTP_MAX_FILEPATH;
	ret = util_string_get(&at_param_list, 2, filepath, &sz_filepath);
	if (ret) {
		return ret;
	}

	ret = at_params_short_get(&at_param_list, 3, &datatype);
	if (ret) {
		return ret;
	}
	if (datatype == DATATYPE_ARBITRARY) {
#if defined(CONFIG_SLM_DATAMODE_HWFC)
		if (!check_uart_flowcontrol()) {
			LOG_ERR("Data mode requires HWFC.");
			return -EINVAL;
		}
#endif
		/* enter data mode */
		ret = enter_datamode(ftp_datamode_callback);
		if (ret) {
			return ret;
		}

		ftp_data_mode_handler = ftp_mput_handler;
		return 0;
	}

	if (param_count > 4) {
		int size = CONFIG_SLM_SOCKET_RX_MAX;

		ret = util_string_get(&at_param_list, 4, rx_data, &size);
		if (ret) {
			return ret;
		}
		if (datatype == DATATYPE_HEXADECIMAL) {
			uint8_t data_hex[size / 2];

			ret = slm_util_atoh(rx_data, size, data_hex, size / 2);
			if (ret > 0) {
				ret = ftp_put(filepath, data_hex, ret, FTP_PUT_APPEND);
			}
		} else {
			ret = ftp_put(filepath, rx_data, size, FTP_PUT_APPEND);
		}

		ret = (ret == FTP_CODE_226) ? 0 : -1;
	} else {
		/* must carry data to send */
		ret = -EINVAL;
	}

	return ret;
}

/**@brief API to handle FTP AT command
 */
int handle_at_ftp(enum at_cmd_type cmd_type)
{
	int ret;
	char op_str[16];
	int size = 16;

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	ret = util_string_get(&at_param_list, 1, op_str, &size);
	if (ret) {
		return ret;
	}
	ret = -EINVAL;
	for (int i = 0; i < FTP_OP_MAX; i++) {
		if (slm_util_casecmp(op_str, ftp_op_list[i].op_str)) {
			ret = ftp_op_list[i].handler();
			break;
		}
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
