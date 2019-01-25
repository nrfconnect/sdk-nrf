/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <init.h>
#include <bsd_limits.h>
#include <at_cmd.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(at_cmd, CONFIG_LOG_DEFAULT_LEVEL);

#define INVALID_DESCRIPTOR	-1
#define INVALID_CLIENT_IDX	0xBEEF
#define SOCKET_COUNT		1
#define CLIENT_COUNT		CONFIG_AT_CMD_MAX_CLIENTS
#define THREAD_STACK_SIZE	(CMD_MAX_LEN + 256)
#define THREAD_PRIORITY		K_PRIO_PREEMPT(CONFIG_AT_CMD_THREAD_PRIO)

#define AT_CMD_OK		"OK"
#define AT_CMD_ERROR		"ERROR"
#define AT_CMD_CESQ_ON		"AT%CESQ=1"
#define AT_CMD_CESQ_OFF		"AT%CESQ=0"
#define AT_CMD_CESQ_RESP	"%CESQ"

#define AT_CMD_LEN(x)           (sizeof(x) - 1)

static struct pollfd poll_fds[SOCKET_COUNT];
static u32_t nfds;
static int common_socket_fd;
static struct k_sem thread_start_sem;
static struct k_mutex clients_mutex;
static struct k_mutex common_socket_mutex;

static struct k_thread socket_thread;
static struct k_thread at_cmd_thread;
static K_THREAD_STACK_DEFINE(socket_thread_stack, 2048);
static K_THREAD_STACK_DEFINE(at_cmd_thread_stack, 2048);
static K_FIFO_DEFINE(at_cmd_fifo);
static struct at_cmd_client *clients[CLIENT_COUNT];
static struct k_poll_signal cmd_done_signal =
	K_POLL_SIGNAL_INITIALIZER(cmd_done_signal);
static atomic_t cesq_subscribers;

struct at_cmd_fifo_item {
	void *fifo_reserved;
	struct at_cmd_msg cmd;
};

static void dispatch_handler(struct k_work *work)
{
	struct at_cmd_client *client =
		CONTAINER_OF(work, struct at_cmd_client, work);

	client->handler(client->cmd, client->cmd_len,
			client->response, client->response_len);
}

static u32_t client_id_create(void)
{
	static atomic_t client_id_counter;

	atomic_inc(&client_id_counter);

	return (u32_t)client_id_counter;
}

static size_t client_idx_from_id(u32_t id)
{
	for (size_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (id == clients[i]->id) {
			return i;
		}
	}

	return INVALID_CLIENT_IDX;
}

static struct at_cmd_client *client_add(size_t client_idx,
					at_cmd_handler_t handler)
{
	struct at_cmd_client *client;

	client = k_malloc(sizeof(struct at_cmd_client));
	if (client == NULL) {
		LOG_ERR("Could not allocate memory for client.");
		return NULL;
	}

	clients[client_idx] = client;
	client->id = client_id_create();
	client->handler = handler;
	client->cmd_pending = false;

	k_work_init(&client->work, dispatch_handler);

	return client;
}

struct at_cmd_client *at_cmd_client_init(at_cmd_handler_t handler)
{
	struct at_cmd_client *client = NULL;

	__ASSERT_NO_MSG(handler);

	k_mutex_lock(&clients_mutex, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i]) {
			continue;
		}

		client = client_add(i, handler);
		break;
	}

	k_mutex_unlock(&clients_mutex);

	if (!client) {
		LOG_ERR("Could not register AT command client.");
	}

	return client;
}

static void client_free(size_t client_idx)
{
	k_free(&clients[client_idx]);
	clients[client_idx] = NULL;
}

void at_cmd_client_uninit(struct at_cmd_client *client)
{
	size_t client_idx;

	__ASSERT_NO_MSG(client);

	k_mutex_lock(&clients_mutex, K_FOREVER);

	client_idx = client_idx_from_id(client->id);
	if (client_idx == INVALID_CLIENT_IDX) {
		LOG_ERR("Client could not be removed, client ID not found.");
	} else {
		client_free(client_idx);
	}

	k_mutex_unlock(&clients_mutex);
}

int at_cmd_send(struct at_cmd_client *client, struct at_cmd_msg *cmd)
{
	struct at_cmd_fifo_item *fifo_cmd_data;
	char *buf;

	if (cmd->len > CMD_MAX_LEN) {
		LOG_ERR("AT command length exceeds maximum allowed length.");
		return -EMSGSIZE;
	}

	/* Allocate memory for the command struct. */
	fifo_cmd_data = k_malloc(sizeof(struct at_cmd_fifo_item));
	if (fifo_cmd_data == NULL) {
		LOG_ERR("Memory could not be allocated for the command.");
		return -ENOMEM;
	}

	/* Allocate memory for the comamnd buffer. */
	buf = k_malloc(cmd->len);
	if (buf == NULL) {
		k_free(fifo_cmd_data);
		LOG_ERR("Memory could not be allocated for command buffer.");
		return -ENOMEM;
	}

	fifo_cmd_data->cmd.client = client;
	fifo_cmd_data->cmd.timestamp = k_uptime_get_32();
	fifo_cmd_data->cmd.buf = buf;
	fifo_cmd_data->cmd.len = cmd->len;

	memcpy(fifo_cmd_data->cmd.buf, cmd->buf, cmd->len);
	memcpy(client->cmd, cmd->buf, cmd->len);

	k_fifo_put(&at_cmd_fifo, fifo_cmd_data);

	return 0;
}

static bool cmd_is_done(char *buf, size_t len)
{
	return strstr(buf, AT_CMD_OK) || strstr(buf, AT_CMD_ERROR);
}

/* @brief Function to get index of client with pending command.
 * @return Client index on success or 0xBEEF on error.
 */
static size_t client_cmd_pending_get_idx(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (!clients[i]) {
			continue;
		}

		if (clients[i]->cmd_pending) {
			return i;
		}
	}

	return INVALID_CLIENT_IDX;
}

static bool is_cesq_notification(char *buf, size_t len)
{
	/* Performing quick screening to see if it's worth doing more thorough
	 * and cycle-demanding parsing of the received sring to determine if
	 * it's a supported notification type.
	 */
	if ((len < AT_CMD_LEN(AT_CMD_CESQ_RESP)) ||
	    (buf[4] != AT_CMD_CESQ_RESP[4])) {
		return false;
	}

	return strstr(buf, AT_CMD_CESQ_RESP) ? true : false;
}

static bool is_cesq_subscribe(char *buf, size_t len)
{
	if (len < AT_CMD_LEN(AT_CMD_CESQ_ON)) {
		return false;
	}

	return strstr(buf, AT_CMD_CESQ_ON) ? true : false;
}

static void cesq_notify(char *buf, size_t len)
{
	for (size_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (atomic_test_bit(&cesq_subscribers, i)) {
			clients[i] = clients[i];
			memcpy(clients[i]->response, buf, len);
			clients[i]->response_len = len;

			/* Dispatch handler through client's work. */
			k_work_submit(&clients[i]->work);
		}
	}
}

static void socket_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int err;
	int bytes_read;
	char buf[RESPONSE_MAX_LEN];
	size_t client_idx;
	struct at_cmd_client *client;
	bool done;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_take(&thread_start_sem, K_FOREVER);

	for (;;) {
		/* Workaround for current poll() implementation, that would
		 * cause this loop to spin without yielding to lower priority
		 * threads until data is received.
		 */
		k_sleep(10);

		/* Poll the socket for incoming data. */
		err = poll(poll_fds, nfds, K_FOREVER);
		if (err < 0) {
			LOG_ERR("Poll error: %d", err);
			continue;
		} else if (err == 0) {
			LOG_DBG("Poll timed out.");
			continue;
		}

		k_mutex_lock(&common_socket_mutex, K_FOREVER);

		bytes_read = recv(common_socket_fd, buf,
				  sizeof(buf), MSG_DONTWAIT);

		k_mutex_unlock(&common_socket_mutex);

		if (bytes_read <= 0) {
			/* Current implementation of poll() will cause us to
			 * get here many times before receiving data.
			 */
			continue;
		}

		if (is_cesq_notification(buf, bytes_read)) {
			cesq_notify(buf, bytes_read);
			continue;
		}

		/* If it's not a notification, find out which client has a
		 * pending command, and dispatch the response to it.
		 */
		client_idx = client_cmd_pending_get_idx();
		if (client_idx == INVALID_CLIENT_IDX) {
			LOG_DBG("No client with pending command.");
			continue;
		}

		client = clients[client_idx];

		memcpy(client->response, buf, bytes_read);
		client->response_len = bytes_read;

		/* Dispatch handler through client's work. */
		LOG_DBG("\t <- SUBMITTING WORK TO CLIENT %d", client->id);
		k_work_submit(&client->work);

		done = cmd_is_done(client->response, bytes_read);
		if (done) {
			k_poll_signal_raise(&cmd_done_signal, 1);
			LOG_DBG("\t <- SIGNALED THAT COMMAND IS PROCESSED");
		}
	}
}

static void at_cmd_thread_fn(void *arg1, void *arg2, void *arg3)
{
	struct at_cmd_fifo_item *fifo_cmd_data = NULL;
	struct at_cmd_msg *cmd = NULL;
	struct at_cmd_client *client = NULL;
	int bytes_sent;
	int err;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct k_poll_event fifo_event[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &at_cmd_fifo)
	};

	struct k_poll_event signal_event[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &cmd_done_signal)
	};

	for (;;) {
		err = k_poll(fifo_event, 1, K_FOREVER);
		if (err) {
			LOG_ERR("k_poll returned error: %d", err);
			continue;
		}

		if (fifo_event[0].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			fifo_cmd_data = k_fifo_get(&at_cmd_fifo, K_FOREVER);
		} else {
			continue;
		}

		if (fifo_cmd_data == NULL) {
			continue;
		}

		fifo_event[0].state = K_POLL_STATE_NOT_READY;

		cmd = &fifo_cmd_data->cmd;
		client = cmd->client;
		client->cmd_pending = true;

		k_mutex_lock(&common_socket_mutex, K_FOREVER);

		bytes_sent = send(common_socket_fd, cmd->buf, cmd->len, 0);

		k_mutex_unlock(&common_socket_mutex);

		if (bytes_sent <= 0) {
			LOG_ERR("Could not send AT command to modem: %d",
								bytes_sent);
			goto error;
		}

		if (is_cesq_subscribe(cmd->buf, cmd->len)) {
			size_t client_idx;

			client_idx = client_idx_from_id(client->id);
			atomic_set_bit(&cesq_subscribers, client_idx);
		}

		/* Command is now pending a response. Wait here until it's
		 * signaled from the socket thread.
		 */
		err = k_poll(signal_event, 1, CONFIG_AT_CMD_RESP_TIMEOUT);
		if (err) {
			LOG_ERR("Poll failed with error: %d", err);
			goto error;
		}

error:
		client->cmd_pending = false;
		k_free(cmd->buf);
		k_free(fifo_cmd_data);
		signal_event[0].signal->signaled = 0;
		signal_event[0].state = K_POLL_STATE_NOT_READY;
	}
}

static int at_cmd_init(struct device *arg)
{
	ARG_UNUSED(arg);

	for (size_t i = 0; i < ARRAY_SIZE(poll_fds); i++) {
		poll_fds[i].fd = INVALID_DESCRIPTOR;
		poll_fds[i].events = ZSOCK_POLLNVAL;
	}

	common_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (common_socket_fd == -1) {
		LOG_ERR("Socket could not be established.");
		return -EFAULT;
	}

	poll_fds[0].fd = common_socket_fd;
	poll_fds[0].events = ZSOCK_POLLIN;
	nfds = 1;

	k_mutex_init(&clients_mutex);
	k_mutex_init(&common_socket_mutex);
	k_sem_init(&thread_start_sem, 0, 2);

	k_thread_create(&socket_thread, socket_thread_stack,
			K_THREAD_STACK_SIZEOF(socket_thread_stack),
			socket_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&at_cmd_thread, at_cmd_thread_stack,
			K_THREAD_STACK_SIZEOF(at_cmd_thread_stack),
			at_cmd_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sem_give(&thread_start_sem);

	return 0;
}

SYS_INIT(at_cmd_init, APPLICATION, 89);
