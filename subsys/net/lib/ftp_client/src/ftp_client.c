/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/ftp_client.h>
#include "ftp_commands.h"

LOG_MODULE_REGISTER(ftp_client, CONFIG_FTP_CLIENT_LOG_LEVEL);

#define INVALID_SOCKET		-1
#define INVALID_SEC_TAG		-1

#define FTP_MAX_HOSTNAME	64
#define FTP_MAX_USERNAME	32
#define FTP_MAX_PASSWORD	32

#define FTP_CODE_ANY		0

#define FTP_STACK_SIZE		KB(2)
#define FTP_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(ftp_stack_area, FTP_STACK_SIZE);

static struct ftp_client {
	int sock; /* Socket descriptor. */
	bool connected; /* Server connected flag */
	struct sockaddr_in remote; /* Server */
	int sec_tag;
	ftp_client_callback_t ctrl_callback;
	ftp_client_callback_t data_callback;
} client;

static struct k_work_q ftp_work_q;
static char ctrl_buf[NET_IPV4_MTU];
static K_SEM_DEFINE(tx_done, 0, 1);
static K_SEM_DEFINE(rx_done, 0, 1);

enum data_task_type {
	TASK_SEND,
	TASK_RECEIVE
};

static struct data_task {
	struct k_work work;
	enum data_task_type task;
	char ctrl_msg[64];	/* PSAV resposne */
	u8_t *data;		/* TX data */
	u16_t length;		/* TX length */
} data_task_param;

static int parse_return_code(const u8_t *message, int success_code)
{
	char code_str[6]; /* max 1xxxx*/
	int ret = FTP_CODE_500;

	sprintf(code_str, "%d ", success_code);
	if (strstr(message, code_str)) {
		ret = success_code;
	}

	return ret;
}

static int establish_data_channel(const char *pasv_msg)
{
	int ret;
	char tmp[16];
	char *tmp1, *tmp2;
	u16_t data_port;
	int data_sock;

	/* Parse Server port from passive message
	 * e.g. "227 Entering Passive Mode (90,130,70,73,86,111)"
	 */
	tmp1 = strstr(pasv_msg, "(");
	tmp1++;
	tmp2 = strstr(tmp1, ",");
	tmp2++;
	tmp1 = strstr(tmp2, ",");
	tmp1++;
	tmp2 = strstr(tmp1, ",");
	tmp2++;
	tmp1 = strstr(tmp2, ",");
	tmp1++;
	tmp2 = strstr(tmp1, ",");
	memset(tmp, 0x00, 16);
	strncpy(tmp, (const char *)tmp1, (size_t)(tmp2 - tmp1));
	data_port = atoi(tmp) << 8;
	tmp2++;
	tmp1 = strstr(tmp2, ")");
	memset(tmp, 0x00, 16);
	strncpy(tmp, (const char *)tmp2, (size_t)(tmp1 - tmp2));
	data_port += atoi(tmp);

	/* Establish the second connect for data */
	if (client.sec_tag <= 0) {
		data_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} else {
		data_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (data_sock < 0) {
		LOG_ERR("socket(data) failed: %d", -errno);
		ret = -errno;
	}

	if (client.sec_tag > 0) {
		sec_tag_t sec_tag_list[1] = { client.sec_tag };

		ret = setsockopt(data_sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			close(data_sock);
			return -errno;
		}

	}
	client.remote.sin_port = htons(data_port);
	ret = connect(data_sock, (struct sockaddr *)&client.remote,
		 sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("connect(data) failed: %d", -errno);
		close(data_sock);
		return -errno;
	}

	return data_sock;
}

/**@brief Send FTP message via socket
 */
static int do_ftp_send_ctrl(const u8_t *message, int length)
{
	int ret;
	u32_t offset = 0;

	LOG_HEXDUMP_DBG(message, length, "TXC");

	while (offset < length) {
		ret = send(client.sock, message + offset, length - offset, 0);
		if (ret < 0) {
			LOG_ERR("send(ctrl) failed: %d", -errno);
			ret = -errno;
			break;
		}
		offset += ret;
		ret = 0;
	}

	LOG_DBG("CMD sent");
	return ret;
}

/**@brief Send FTP data via socket
 */
static int do_ftp_send_data(const char *pasv_msg, u8_t *message, u16_t length)
{
	int data_sock;
	int ret;
	u32_t offset = 0;

	/* Establish data channel */
	ret = establish_data_channel(pasv_msg);
	if (ret < 0) {
		return ret;
	}

	LOG_HEXDUMP_DBG(message, length, "TXD");

	data_sock = ret;
	while (offset < length) {
		ret = send(data_sock, message + offset, length - offset, 0);
		if (ret < 0) {
			LOG_ERR("send(data) failed: %d", -errno);
			ret = -errno;
			break;
		}
		offset += ret;
		ret = 0;
	}

	LOG_DBG("DATA sent");
	close(data_sock);
	return ret;
}

/**@brief Receive FTP message from socket
 */
static int do_ftp_recv_ctrl(bool post_result, int success_code)
{
	int ret;
	struct pollfd fds[1];

	/* Receive FTP control message */
	fds[0].fd = client.sock;
	fds[0].events = POLLIN;
	ret = poll(fds, 1, MSEC_PER_SEC * CONFIG_FTP_CLIENT_LISTEN_TIME);
	if (ret <= 0) {
		LOG_ERR("poll(ctrl) failed: (%d)", -errno);
		return -ETIMEDOUT;
	}
	ret = recv(client.sock, ctrl_buf, sizeof(ctrl_buf), 0);
	if (ret < 0) {
		LOG_ERR("recv(ctrl) failed: (%d)", -errno);
		return -errno;
	}
	if (ret == 0) {
		/* Server close connection */
		return -ECONNRESET;
	}

	LOG_HEXDUMP_DBG(ctrl_buf, ret, "RXC");

	if (post_result) {
		client.ctrl_callback(ctrl_buf, ret);
	}

	LOG_DBG("CTRL received");
	return parse_return_code(ctrl_buf, success_code);
}

/**@brief Receive FTP data from socket
 */
static void do_ftp_recv_data(const char *pasv_msg)
{
	int ret;
	int data_sock;
	struct pollfd fds[1];
	char data_buf[NET_IPV4_MTU];

	/* Establish data channel */
	ret = establish_data_channel(pasv_msg);
	if (ret < 0) {
		return;
	}

	/* Receive FTP data message */
	data_sock = ret;
	fds[0].fd = data_sock;
	fds[0].events = POLLIN;
	do {
		ret = poll(fds, 1,
			MSEC_PER_SEC * CONFIG_FTP_CLIENT_LISTEN_TIME);
		if (ret <= 0) {
			LOG_ERR("poll(data) failed: (%d)", -errno);
			break;
		}
		if ((fds[0].revents & POLLIN) != POLLIN) {
			LOG_INF("No more data");
			break;
		}
		ret = recv(data_sock, data_buf, sizeof(data_buf), 0);
		if (ret < 0) {
			LOG_ERR("recv(data) failed: (%d)", -errno);
			break;
		}
		if (ret == 0) {
			/* Server close connection */
			break;
		}

		LOG_HEXDUMP_DBG(data_buf, ret, "RXD");
		client.data_callback(data_buf, ret);
	} while (true);

	close(data_sock);
	LOG_DBG("DATA received");
}

static void data_task(struct k_work *item)
{
	struct data_task *task_param =
		CONTAINER_OF(item, struct data_task, work);

	if (task_param->task == TASK_RECEIVE) {
		do_ftp_recv_data(task_param->ctrl_msg);
		k_sem_give(&rx_done);
	} else if (task_param->task == TASK_SEND) {
		do_ftp_send_data(task_param->ctrl_msg,
				task_param->data,
				task_param->length);
		k_sem_give(&tx_done);
	}
}

static void keepalive_handler(struct k_work *work)
{
	int ret;

	ret = do_ftp_send_ctrl(CMD_NOOP, sizeof(CMD_NOOP) - 1);
	if (ret == 0) {
		(void)do_ftp_recv_ctrl(false, FTP_CODE_200);
	}
}

K_WORK_DEFINE(keepalive_work, keepalive_handler);

static void keepalive_timeout(struct k_timer *dummy)
{
	k_work_submit_to_queue(&ftp_work_q, &keepalive_work);
}

K_TIMER_DEFINE(keepalive_timer, keepalive_timeout, NULL);

int ftp_open(const char *hostname, u16_t port, int sec_tag)
{
	int ret;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET
	};

	if (client.connected) {
		LOG_ERR("FTP already connected");
		return -EINVAL;
	}
	/* open control socket */
	if (sec_tag <= 0) {
		client.sock = socket(AF_INET, SOCK_STREAM,
				IPPROTO_TCP);
	} else {
		client.sock = socket(AF_INET, SOCK_STREAM,
				IPPROTO_TLS_1_2);
	}
	if (client.sock < 0) {
		LOG_ERR("socket(ctrl) failed: %d", -errno);
		ret = -errno;
	}

	if (sec_tag > 0) {
		sec_tag_t sec_tag_list[1] = { sec_tag };

		ret = setsockopt(client.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			close(client.sock);
			return -errno;
		}
		client.sec_tag = sec_tag;
	}

	/* Connect to remote host */
	ret = getaddrinfo(hostname, NULL, &hints, &result);
	if (ret || result == NULL) {
		LOG_ERR("ERROR: getaddrinfo(ctrl) failed %d", ret);
		close(client.sock);
		return ret;
	}
	client.remote.sin_family = AF_INET;
	client.remote.sin_port = htons(port);
	client.remote.sin_addr.s_addr =
		((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	ret = connect(client.sock, (struct sockaddr *)&client.remote,
		 sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("connect(ctrl) failed: %d", -errno);
		close(client.sock);
		freeaddrinfo(result);
		return -errno;
	}

	freeaddrinfo(result);

	/* Receive server greeting */
	ret = do_ftp_recv_ctrl(true, FTP_CODE_220);
	if (ret != FTP_CODE_220) {
		close(client.sock);
		return ret;
	}

	/* Send UTF8 option */
	sprintf(ctrl_buf, CMD_OPTS, "UTF8 ON");
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret) {
		close(client.sock);
		return ret;
	}
	ret = do_ftp_recv_ctrl(true, FTP_CODE_200);
	if (ret != FTP_CODE_200) {
		close(client.sock);
		return ret;
	}

	LOG_DBG("FTP opened");
	return ret;
}

int ftp_login(const char *username, const char *password)
{
	int ret;
	int keepalive_time = CONFIG_FTP_CLIENT_KEEPALIVE_TIME;

	/* send username */
	sprintf(ctrl_buf, CMD_USER, username);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret) {
		return ret;
	}
	ret = do_ftp_recv_ctrl(true, FTP_CODE_331);
	if (ret == FTP_CODE_331) {
		/* send password if requested */
		sprintf(ctrl_buf, CMD_PASS, password);
		ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
		if (ret) {
			return ret;
		}
		ret = do_ftp_recv_ctrl(true, FTP_CODE_230);
	}
	if (ret != FTP_CODE_230) {
		return ret;
	}

	client.connected = true;
	/* Start keep alive timer */
	if (keepalive_time > 0) {
		k_timer_start(&keepalive_timer, K_SECONDS(1),
			K_SECONDS(keepalive_time));
	}

	return ret;
}

int ftp_close(void)
{
	int ret;

	ret = do_ftp_send_ctrl(CMD_QUIT, sizeof(CMD_QUIT) - 1);
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_221);
		k_timer_stop(&keepalive_timer);
	}

	close(client.sock);
	client.connected = false;
	client.sec_tag = INVALID_SEC_TAG;
	return ret;
}

int ftp_status(void)
{
	int ret;

	/* get server system type */
	ret = do_ftp_send_ctrl(CMD_SYST, sizeof(CMD_SYST) - 1);
	if (ret == 0) {
		do_ftp_recv_ctrl(true, FTP_CODE_215);
	}

	/* get server and connection status */
	ret = do_ftp_send_ctrl(CMD_STAT, sizeof(CMD_STAT) - 1);
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_211);
		if (ret != FTP_CODE_211) {
			do {
				ret = do_ftp_recv_ctrl(true, FTP_CODE_211);
				if (ret == -ETIMEDOUT || ret == FTP_CODE_211) {
					break;
				}
			} while (1);
		}
	}

	return ret;
}

int ftp_type(enum ftp_trasfer_type type)
{
	int ret;

	if (type == FTP_TYPE_ASCII) {
		ret = do_ftp_send_ctrl(CMD_TYPE_A, sizeof(CMD_TYPE_A) - 1);
	} else if (type == FTP_TYPE_BINARY) {
		ret = do_ftp_send_ctrl(CMD_TYPE_I, sizeof(CMD_TYPE_I) - 1);
	} else {
		return -EINVAL;
	}
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_200);
	}

	return ret;
}

int ftp_pwd(void)
{
	int ret;

	ret = do_ftp_send_ctrl(CMD_PWD, sizeof(CMD_PWD) - 1);
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_257);
	}

	return ret;
}

int ftp_list(const char *options, const char *target)
{
	int ret;
	char list_cmd[128];

	/* Always set Passive mode to act as TCP client */
	ret = do_ftp_send_ctrl(CMD_PASV, sizeof(CMD_PASV) - 1);
	if (ret) {
		return -EIO;
	}
	ret = do_ftp_recv_ctrl(true, FTP_CODE_227);
	if (ret != FTP_CODE_227) {
		return ret;
	}
	strcpy(data_task_param.ctrl_msg, ctrl_buf);
	data_task_param.task = TASK_RECEIVE;

	/* Send LIST/NLST command in control channel */
	if (strlen(options) != 0) {
		if (strlen(target) != 0) {
			sprintf(list_cmd, CMD_LIST_OPT_FILE, options, target);
		} else {
			sprintf(list_cmd, CMD_LIST_OPT, options);
		}
	} else {
		if (strlen(target) != 0) {
			sprintf(list_cmd, CMD_LIST_FILE, target);
		} else {
			memcpy(list_cmd, CMD_NLST, sizeof(CMD_NLST) - 1);
		}
	}
	ret = do_ftp_send_ctrl(list_cmd, strlen(list_cmd));

	/* Set up data connection */
	k_work_submit_to_queue(&ftp_work_q, &data_task_param.work);

	if (ret == 0) {
		do {
			ret = do_ftp_recv_ctrl(true, FTP_CODE_226);
			if (ret == -ETIMEDOUT || ret == FTP_CODE_226) {
				break;
			}
		} while (1);
	}

	return ret;
}

int ftp_cwd(const char *folder)
{
	int ret;

	if (strcmp(folder, "..") == 0) {
		ret = do_ftp_send_ctrl(CMD_CDUP, sizeof(CMD_CDUP) - 1);
	} else {
		sprintf(ctrl_buf, CMD_CWD, folder);
		ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	}
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_250);
	}

	return ret;
}

int ftp_mkd(const char *folder)
{
	int ret;

	sprintf(ctrl_buf, CMD_MKD, folder);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_257);
	}

	return ret;
}

int ftp_rmd(const char *folder)
{
	int ret;

	sprintf(ctrl_buf, CMD_RMD, folder);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_250);
	}

	return ret;
}

int ftp_rename(const char *old_name, const char *new_name)
{
	int ret;

	sprintf(ctrl_buf, CMD_RNFR, old_name);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_350);
	}
	if (ret != FTP_CODE_350) {
		return ret;
	}

	sprintf(ctrl_buf, CMD_RNTO, new_name);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_250);
	}

	return ret;
}

int ftp_delete(const char *file)
{
	int ret;

	sprintf(ctrl_buf, CMD_DELE, file);
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret == 0) {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_250);
	}

	return ret;
}

int ftp_get(const char *file)
{
	int ret;
	char get_cmd[128];

	/* Always set Passive mode to act as TCP client */
	ret = do_ftp_send_ctrl(CMD_PASV, sizeof(CMD_PASV) - 1);
	if (ret) {
		return -EIO;
	}
	ret = do_ftp_recv_ctrl(true, FTP_CODE_227);
	if (ret != FTP_CODE_227) {
		return ret;
	}
	strcpy(data_task_param.ctrl_msg, ctrl_buf);
	data_task_param.task = TASK_RECEIVE;

	/* Send RETR command in control channel */
	sprintf(get_cmd, CMD_RETR, file);
	ret = do_ftp_send_ctrl(get_cmd, strlen(get_cmd));
	if (ret) {
		return -EIO;
	}

	/* Set up data connection */
	k_work_submit_to_queue(&ftp_work_q, &data_task_param.work);
	k_sem_take(&rx_done, K_FOREVER);
	client.ctrl_callback("\r\n", 2);

	/* Receive control */
	do {
		ret = do_ftp_recv_ctrl(false, FTP_CODE_226);
		if (ret == -ETIMEDOUT || ret == FTP_CODE_226) {
			break;
		}
	} while (1);

	if (ret == FTP_CODE_226) {
		client.ctrl_callback(ctrl_buf, strlen(ctrl_buf));
	}

	return ret;
}

int ftp_put(const char *file, const u8_t *data, u16_t length)
{
	int ret;
	char put_cmd[128];

	if (data && length) {
		/* Always set Passive mode to act as TCP client */
		ret = do_ftp_send_ctrl(CMD_PASV, sizeof(CMD_PASV) - 1);
		if (ret) {
			return -EIO;
		}
		ret = do_ftp_recv_ctrl(true, FTP_CODE_227);
		if (ret != FTP_CODE_227) {
			return ret;
		}
		strcpy(data_task_param.ctrl_msg, ctrl_buf);
		data_task_param.task = TASK_SEND;
		data_task_param.data = (u8_t *)data;
		data_task_param.length = length;
	}

	/* Send STOR command in control channel */
	sprintf(put_cmd, CMD_STOR, file);
	ret = do_ftp_send_ctrl(put_cmd, strlen(put_cmd));

	/* Now send data */
	if (data && length) {
		k_work_submit_to_queue(&ftp_work_q, &data_task_param.work);
		k_sem_take(&tx_done, K_FOREVER);
	}

	if (ret == 0) {
		do {
			ret = do_ftp_recv_ctrl(true, FTP_CODE_226);
			if (ret == -ETIMEDOUT || ret == FTP_CODE_226) {
				break;
			}
		} while (1);
	}

	return ret;
}

int ftp_init(ftp_client_callback_t ctrl_callback,
	ftp_client_callback_t data_callback)
{
	if (ctrl_callback == NULL || data_callback == NULL) {
		return -EINVAL;
	}
	client.sock = INVALID_SOCKET;
	client.connected = false;
	client.sec_tag = INVALID_SEC_TAG;
	client.ctrl_callback = ctrl_callback;
	client.data_callback = data_callback;

	k_work_q_start(&ftp_work_q, ftp_stack_area,
		K_THREAD_STACK_SIZEOF(ftp_stack_area), FTP_PRIORITY);
	k_work_init(&data_task_param.work, data_task);

	return 0;
}

int ftp_uninit(void)
{
	return ftp_close();
}
