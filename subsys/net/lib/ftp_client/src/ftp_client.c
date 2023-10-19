/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <net/ftp_client.h>
#include "ftp_commands.h"

LOG_MODULE_REGISTER(ftp_client, CONFIG_FTP_CLIENT_LOG_LEVEL);

#define INVALID_SOCKET		-1
#define INVALID_SEC_TAG		-1

#define FTP_MAX_BUFFER_SIZE	708 /* align with MSS on modem side */
#define FTP_DATA_TIMEOUT_SEC	60  /* time in seconds to wait for "Transfer complete" */

#define FTP_CODE_ANY		0

#define FTP_STACK_SIZE		KB(2)
#define FTP_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(ftp_stack_area, FTP_STACK_SIZE);

static struct ftp_client {
	int family;     /* Socket address family */
	bool connected; /* Server connected flag */
	int cmd_sock;
	int data_sock;
	int sec_tag;
	ftp_client_callback_t ctrl_callback;
	ftp_client_callback_t data_callback;
	union {
		struct sockaddr_in  remote;  /* IPv4 server */
		struct sockaddr_in6 remote6; /* IPv6 server */
	};
} client;

static struct k_work_q ftp_work_q;
static char ctrl_buf[FTP_MAX_BUFFER_SIZE];

enum data_task_type {
	TASK_SEND,
	TASK_RECEIVE
};

static struct data_task {
	struct k_work work;
	enum data_task_type task;
	char *ctrl_msg;		/* PSAV resposne */
	uint8_t *data;		/* TX data */
	uint16_t length;	/* TX length */
} data_task_param;

static bool ftp_inactivity;
static atomic_t data_task_running;

static int parse_return_code(const uint8_t *message, int success_code)
{
	if (success_code == FTP_CODE_ANY) {
		return success_code;
	}
	int ret = FTP_CODE_500;

	while (true) {
		char *endptr;
		int reply_code = strtol(message, &endptr, 10);

		/* Strict parsing of "%d ". */
		if (reply_code && endptr && *endptr == ' ') {
			ret = reply_code;
			if (ret == success_code) {
				break;
			}
		}

		/* No reply code on this line. Go to the next one. */
		message = strstr(message, "\r\n");
		if (!message) {
			break;
		}
		message += 2;
	}

	return ret;
}

static int establish_data_channel(const char *pasv_msg)
{
	int ret;
	char tmp[8];
	char *tmp1, *tmp2;
	uint16_t data_port;

	/* Parse Server port from passive message
	 * e.g. "227 Entering Passive Mode (90,130,70,73,86,111)" in case of IPv4
	 * e.g. "227 Entering Passive Mode (0,0,0,0,97,78)" in case of IPv6
	 * NOTE assume no IP address change from the Control channel
	 */
	tmp1 = strrchr(pasv_msg, ')');
	if (tmp1 == NULL) {
		return -EINVAL;
	}
	tmp2 = strrchr(pasv_msg, ',');
	if (tmp2 == NULL) {
		return -EINVAL;
	}
	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, (const char *)(tmp2 + 1), (size_t)(tmp1 - tmp2 - 1));
	data_port = atoi(tmp);
	tmp1 = tmp2 - 1;
	while (isdigit((int)(*tmp1))) {
		tmp1--;
	}
	if (*tmp1 != ',') {
		return -EINVAL;
	}
	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, (const char *)(tmp1 + 1), (size_t)(tmp2 - tmp1 - 1));
	data_port += atoi(tmp) << 8;
	LOG_DBG("data port: %d", data_port);

	/* open data socket */
	if (client.sec_tag <= 0) {
		client.data_sock = socket(client.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
		client.data_sock = socket(client.family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (client.data_sock < 0) {
		LOG_ERR("socket(data) failed: %d", -errno);
		return -errno;
	}
	if (client.sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[] = { client.sec_tag };

		ret = setsockopt(client.data_sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			close(client.data_sock);
			return -errno;
		}

	}

	/* Connect to remote host */
	if (client.family == AF_INET) {
		client.remote.sin_port = htons(data_port);
		ret = connect(client.data_sock, (struct sockaddr *)&client.remote,
			sizeof(struct sockaddr_in));
	} else {
		client.remote6.sin6_port = htons(data_port);
		ret = connect(client.data_sock, (struct sockaddr *)&client.remote6,
			sizeof(struct sockaddr_in6));
	}
	if (ret < 0) {
		LOG_ERR("connect(data) failed: %d", -errno);
		close(client.data_sock);
		return -errno;
	}

	return 0;
}

static void close_connection(int code, int error)
{
	if (FTP_PROPRIETARY(code)) {
		switch (code) {
		case FTP_CODE_901:
			sprintf(ctrl_buf, "901 Disconnected(%d).\r\n", error);
			break;
		case FTP_CODE_902:
			sprintf(ctrl_buf, "902 Connection aborted(%d).\r\n", error);
			break;
		case FTP_CODE_903:
			sprintf(ctrl_buf, "903 Poll error(%d).\r\n", error);
			break;
		case FTP_CODE_904:
			sprintf(ctrl_buf, "904 Unexpected poll event(%d).\r\n", error);
			break;
		case FTP_CODE_905:
			sprintf(ctrl_buf, "905 Network down (%d).\r\n", error);
			break;
		default:
			sprintf(ctrl_buf, "900 Unknown error(%d).\r\n", -ENOEXEC);
			break;
		}
		client.ctrl_callback(ctrl_buf, strlen(ctrl_buf));
	}
	/* Should be impossble, just in case */
	if (client.data_sock != INVALID_SOCKET) {
		close(client.data_sock);
	}
	if (client.cmd_sock != INVALID_SOCKET) {
		close(client.cmd_sock);
		client.cmd_sock = INVALID_SOCKET;
		client.data_sock = INVALID_SOCKET;
		client.connected = false;
		client.sec_tag = INVALID_SEC_TAG;
	}
}

/**@brief Send FTP message via socket
 */
static int do_ftp_send_ctrl(const uint8_t *message, int length)
{
	int ret = 0;
	uint32_t offset = 0;

	LOG_DBG("%s", (char *)message);
	while (offset < length) {
		ret = send(client.cmd_sock, message + offset, length - offset, 0);
		if (ret < 0) {
			LOG_ERR("send cmd failed: %d", -errno);
			ret = -errno;
			break;
		}
		offset += ret;
		ret = 0;
	}
	/* Detect network down */
	if (ret == -ENETDOWN) {
		close_connection(FTP_CODE_905, ret);
	} else {
		LOG_DBG("CMD sent");
	}
	ftp_inactivity = false;
	return ret;
}

/**@brief Send FTP data via socket
 */
static int do_ftp_send_data(const char *pasv_msg, uint8_t *message, uint16_t length)
{
	int ret;
	uint32_t offset = 0;

	/* Establish data channel */
	ret = establish_data_channel(pasv_msg);
	if (ret) {
		return ret;
	}

	while (offset < length) {
		ret = send(client.data_sock, message + offset, length - offset, 0);
		if (ret < 0) {
			LOG_ERR("send data failed: %d", -errno);
			ret = -errno;
			break;
		}
		LOG_DBG("DATA sent %d", ret);
		offset += ret;
		ret = 0;
	}

	close(client.data_sock);
	ftp_inactivity = false;
	return ret;
}

/**@brief Receive FTP message from socket
 */
static int do_ftp_recv_ctrl(bool post_result, int success_code)
{
	int ret;
	struct pollfd fds[1];

	/* Receive FTP control message */
	fds[0].fd = client.cmd_sock;
	fds[0].events = POLLIN;
	ret = poll(fds, 1, MSEC_PER_SEC * CONFIG_FTP_CLIENT_LISTEN_TIME);
	if (ret < 0) {
		LOG_ERR("poll(ctrl) failed: (%d)", -errno);
		close_connection(FTP_CODE_903, -errno);
		return -EIO;
	}
	if (ret == 0) {
		LOG_DBG("poll(ctrl) timeout");
		return -ETIMEDOUT;   /* allow retry */
	}
	if ((fds[0].revents & POLLHUP) == POLLHUP) {
		LOG_ERR("POLLHUP");
		close_connection(FTP_CODE_901, -ECONNRESET);
		return -ECONNRESET;
	}
	if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
		LOG_ERR("POLLNVAL");
		close_connection(FTP_CODE_902, -ECONNABORTED);
		return -ECONNABORTED;
	}
	if ((fds[0].revents & POLLIN) != POLLIN) {
		LOG_ERR("POLL 0x%08x", fds[0].revents);
		close_connection(FTP_CODE_904, -EAGAIN);
		return -EAGAIN;
	}
	ret = recv(client.cmd_sock, ctrl_buf, sizeof(ctrl_buf), 0);
	if (ret <= 0) {
		LOG_ERR("recv(ctrl) failed: (%d)", -errno);
		return -errno;   /* allow retry */
	}

	ctrl_buf[ret] = 0x00;
	if (post_result) {
		client.ctrl_callback(ctrl_buf, ret);
	}

	LOG_DBG("%s", ctrl_buf);
	ftp_inactivity = false;
	return parse_return_code(ctrl_buf, success_code);
}

/**@brief Receive FTP data from socket
 */
static void do_ftp_recv_data(const char *pasv_msg)
{
	int ret;
	struct pollfd fds[1];
	char data_buf[FTP_MAX_BUFFER_SIZE];

	/* Establish data channel */
	ret = establish_data_channel(pasv_msg);
	if (ret) {
		return;
	}

	/* Receive FTP data message */
	fds[0].fd = client.data_sock;
	fds[0].events = POLLIN;
	do {
		ret = poll(fds, 1, MSEC_PER_SEC * CONFIG_FTP_CLIENT_LISTEN_TIME);
		if (ret <= 0) {
			LOG_ERR("poll(data) failed: (%d)", -errno);
			break;
		}
		if ((fds[0].revents & POLLIN) != POLLIN) {
			LOG_DBG("No more data");
			break;
		}
		ret = recv(client.data_sock, data_buf, sizeof(data_buf), 0);
		if (ret < 0) {
			LOG_ERR("recv(data) failed: (%d)", -errno);
			break;
		}
		if (ret == 0) {
			/* Server close connection */
			break;
		}
		client.data_callback(data_buf, ret);
		LOG_DBG("DATA received %d", ret);
	} while (true);

	close(client.data_sock);
	ftp_inactivity = false;
}

static int poll_data_task_done(void)
{
	int ret;
	unsigned int wait_time_ms = 0;

	do {
		ret = do_ftp_recv_ctrl(true, FTP_CODE_226);
		if (ret == -ETIMEDOUT) {
			wait_time_ms += CONFIG_FTP_CLIENT_LISTEN_TIME * MSEC_PER_SEC;
			if (wait_time_ms / MSEC_PER_SEC >= FTP_DATA_TIMEOUT_SEC) {
				break;
			}
		}
		/* Wait for 226 Transfer complete. */
	} while ((ret >= 100 && ret < 200) || ret == -ETIMEDOUT);

	if (ret == FTP_CODE_226) {
		/* Wait for the data transfer to complete. */
		while (atomic_get(&data_task_running) == true) {
			k_sleep(K_MSEC(100));
			wait_time_ms += 100;
			if (wait_time_ms / MSEC_PER_SEC >= FTP_DATA_TIMEOUT_SEC) {
				ret = -ETIMEDOUT;
				break;
			}
		}
	}

	return ret;
}

static void data_task(struct k_work *item)
{
	struct data_task *task_param = CONTAINER_OF(item, struct data_task, work);

	if (task_param->task == TASK_RECEIVE) {
		do_ftp_recv_data(task_param->ctrl_msg);
	} else if (task_param->task == TASK_SEND) {
		do_ftp_send_data(task_param->ctrl_msg, task_param->data, task_param->length);
	}
	atomic_set(&data_task_running, false);
}

static int run_data_task(void)
{
	atomic_set(&data_task_running, true);
	k_work_submit_to_queue(&ftp_work_q, &data_task_param.work);
	return poll_data_task_done();
}

static void keepalive_handler(struct k_work *work)
{
	int ret;

	if (ftp_inactivity) {
		ret = do_ftp_send_ctrl(CMD_NOOP, sizeof(CMD_NOOP) - 1);
		if (ret == 0) {
			(void)do_ftp_recv_ctrl(false, FTP_CODE_200);
		}
	} else {
		ftp_inactivity = true;
	}
}

static void keepalive_timeout(struct k_timer *dummy);
K_WORK_DEFINE(keepalive_work, keepalive_handler);
K_TIMER_DEFINE(keepalive_timer, keepalive_timeout, NULL);

static void keepalive_timeout(struct k_timer *dummy)
{
	if (client.cmd_sock != INVALID_SOCKET) {
		k_work_submit_to_queue(&ftp_work_q, &keepalive_work);
	} else {
		k_timer_stop(&keepalive_timer);
	}
}

int ftp_open(const char *hostname, uint16_t port, int sec_tag)
{
	if (client.connected) {
		LOG_ERR("FTP already connected");
		return -EINVAL;
	}
	int ret;
	struct addrinfo *ai;

	/* Resolve the hostname in the preferred IP version .*/
	ret = getaddrinfo(hostname, NULL, NULL, &ai);
	if (ret) {
		LOG_ERR("Failed to resolve hostname (\"%s\"): %s", hostname, gai_strerror(ret));
		return -EHOSTUNREACH;
	}
	client.family = ai->ai_family;
	if (client.family == AF_INET) {
		memcpy(&client.remote, ai->ai_addr, sizeof(client.remote));
	} else {
		memcpy(&client.remote6, ai->ai_addr, sizeof(client.remote6));
	}
	freeaddrinfo(ai);

	/* open control socket */
	if (sec_tag == INVALID_SEC_TAG) {
		client.cmd_sock = socket(client.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
		client.cmd_sock = socket(client.family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (client.cmd_sock < 0) {
		LOG_ERR("socket(ctrl) failed: %d", -errno);
		ret = -errno;
	}
	if (sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[] = { sec_tag };

		ret = setsockopt(client.cmd_sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			close(client.cmd_sock);
			return -errno;
		}
		client.sec_tag = sec_tag;
	}

	/* Connect to remote host */
	if (client.family == AF_INET) {
		client.remote.sin_port = htons(port);
		ret = connect(client.cmd_sock, (struct sockaddr *)&client.remote,
			sizeof(struct sockaddr_in));
	} else {
		client.remote6.sin6_port = htons(port);
		ret = connect(client.cmd_sock, (struct sockaddr *)&client.remote6,
			sizeof(struct sockaddr_in6));
	}
	if (ret < 0) {
		LOG_ERR("connect(ctrl) failed: %d", -errno);
		close(client.cmd_sock);
		return -errno;
	}

	/* Receive server greeting */
	ret = do_ftp_recv_ctrl(true, FTP_CODE_220);
	if (ret != FTP_CODE_220) {
		close(client.cmd_sock);
		return ret;
	}

	/* Send UTF8 option */
	sprintf(ctrl_buf, CMD_OPTS, "UTF8 ON");
	ret = do_ftp_send_ctrl(ctrl_buf, strlen(ctrl_buf));
	if (ret) {
		close(client.cmd_sock);
		return ret;
	}
	(void)do_ftp_recv_ctrl(true, FTP_CODE_ANY);

	LOG_DBG("FTP opened");
	return FTP_CODE_200;
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
		k_timer_start(&keepalive_timer, K_SECONDS(1), K_SECONDS(keepalive_time));
	}

	return ret;
}

int ftp_close(void)
{
	int ret = 0;

	if (client.connected) {
		ret = do_ftp_send_ctrl(CMD_QUIT, sizeof(CMD_QUIT) - 1);
		if (ret == 0) {
			/* Some FTP servers do not reply QUIT */
			(void)do_ftp_recv_ctrl(true, FTP_CODE_221);
		} else {
			return ret;
		}
	}
	close_connection(FTP_CODE_200, 0);
	return FTP_CODE_221;
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
	data_task_param.ctrl_msg = ctrl_buf;
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
			strcpy(list_cmd, CMD_NLST);
		}
	}
	ret = do_ftp_send_ctrl(list_cmd, strlen(list_cmd));

	return run_data_task();
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
	data_task_param.ctrl_msg = ctrl_buf;
	data_task_param.task = TASK_RECEIVE;

	/* Send RETR command in control channel */
	sprintf(get_cmd, CMD_RETR, file);
	ret = do_ftp_send_ctrl(get_cmd, strlen(get_cmd));
	if (ret) {
		return -EIO;
	}

	return run_data_task();
}

int ftp_put(const char *file, const uint8_t *data, uint16_t length, int type)
{
	int ret;
	char put_cmd[128];

	if (type != FTP_PUT_NORMAL && type != FTP_PUT_UNIQUE && type != FTP_PUT_APPEND) {
		return -EINVAL;
	}
	if ((type == FTP_PUT_NORMAL || type == FTP_PUT_APPEND) &&
	   ((file == NULL) || data == NULL)) {
		return -EINVAL;
	}

	/** Typical sequence:
	 * FTP		51	Request: PASV
	 * FTP		96	Response: 227 Entering Passive Mode (90,130,70,73,105,177).
	 * FTP		63	Request: STOR upload2.txt
	 * FTP-DATA	53	FTP Data: 8 bytes (PASV) (STOR upload2.txt)
	 * FTP		67	Response: 150 Ok to send data.
	 * FTP		69	Response: 226 Transfer complete.
	 */

	if (data != NULL && length > 0) {
		/* Always set Passive mode to act as TCP client */
		ret = do_ftp_send_ctrl(CMD_PASV, sizeof(CMD_PASV) - 1);
		if (ret) {
			return -EIO;
		}
		ret = do_ftp_recv_ctrl(true, FTP_CODE_227);
		if (ret != FTP_CODE_227) {
			return ret;
		}
		data_task_param.ctrl_msg = ctrl_buf;
		data_task_param.task = TASK_SEND;
		data_task_param.data = (uint8_t *)data;
		data_task_param.length = length;
	}

	if (type == FTP_PUT_NORMAL) {
		/* Send STOR command in control channel */
		sprintf(put_cmd, CMD_STOR, file);
		ret = do_ftp_send_ctrl(put_cmd, strlen(put_cmd));
	} else if (type == FTP_PUT_UNIQUE) {
		/* Send STOU command in control channel */
		sprintf(put_cmd, CMD_STOU);
		ret = do_ftp_send_ctrl(put_cmd, strlen(put_cmd));
	} else {
		/* Send APPE command in control channel */
		sprintf(put_cmd, CMD_APPE, file);
		ret = do_ftp_send_ctrl(put_cmd, strlen(put_cmd));
	}
	if (ret != 0) {
		return ret;
	}

	/* Now send data if any */
	if (data != NULL && length > 0) {
		ret = run_data_task();
	} else {
		ret = FTP_CODE_226;
	}

	return ret;
}

int ftp_init(ftp_client_callback_t ctrl_callback, ftp_client_callback_t data_callback)
{
	if (ctrl_callback == NULL || data_callback == NULL) {
		return -EINVAL;
	}
	client.cmd_sock = INVALID_SOCKET;
	client.data_sock = INVALID_SOCKET;
	client.connected = false;
	client.sec_tag = INVALID_SEC_TAG;
	client.ctrl_callback = ctrl_callback;
	client.data_callback = data_callback;

	k_work_queue_start(&ftp_work_q, ftp_stack_area,
			   K_THREAD_STACK_SIZEOF(ftp_stack_area),
			   FTP_PRIORITY, NULL);
	k_work_init(&data_task_param.work, data_task);

	return 0;
}

int ftp_uninit(void)
{
	int ret = 0;

	if (client.cmd_sock != INVALID_SOCKET) {
		ret = ftp_close();
	}

	return ret;
}
