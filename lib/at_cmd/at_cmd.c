/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <init.h>
#include <nrf_modem_limits.h>

#include <modem/at_cmd.h>
#include <modem/nrf_modem_lib.h>

LOG_MODULE_REGISTER(at_cmd, CONFIG_AT_CMD_LOG_LEVEL);

#define THREAD_PRIORITY   K_PRIO_PREEMPT(CONFIG_AT_CMD_THREAD_PRIO)

#define AT_CMD_OK_STR    "OK\r\n"
#define AT_CMD_ERROR_STR "ERROR\r\n"
#define AT_CMD_CMS_STR   "+CMS ERROR:"
#define AT_CMD_CME_STR   "+CME ERROR:"

/* Flags describing an AT command request */
enum at_cmd_flags {
	AT_CMD_BUF_CMD = 1 << 0,	/* Command is buffered by at_cmd */
	AT_CMD_SYNC = 1 << 1,		/* Command is synchronous */
};

/* Metadata for a queued AT command */
struct cmd_item  {
	char *cmd;			/* Pointer to 0-terminated command */
	char *resp;			/* Pointer to response buffer */
	at_cmd_handler_t callback;	/* Callback to execute on result */
	size_t resp_size;		/* Size of response buffer */
	enum at_cmd_flags flags;	/* Flags describing the request */
};

/* Metadata for an AT response */
struct resp_item  {
	int code;			/* Return code of AT command */
	enum at_cmd_state state;	/* State of AT command */
};

static K_THREAD_STACK_DEFINE(socket_thread_stack,
			     CONFIG_AT_CMD_THREAD_STACK_SIZE);

static int common_socket_fd;
static k_tid_t socket_tid;
static struct k_thread socket_thread;
static at_cmd_handler_t notification_handler;
static atomic_t shutdown_mode;


/* Structure holding current command. current_cmd.cmd=NULL signifies no cmd. */
static struct cmd_item current_cmd;
K_MUTEX_DEFINE(current_cmd_mutex);

/* Queue for queued command metadata */
K_MSGQ_DEFINE(commands, sizeof(struct cmd_item), CONFIG_AT_CMD_QUEUE_LEN, 4);

/* Message queue to return the result in the case of a synchronous call */
K_MSGQ_DEFINE(response_sync, sizeof(struct resp_item), 1, 4);
K_MUTEX_DEFINE(response_sync_get);

static int open_socket(void)
{
	common_socket_fd = socket(AF_LTE, SOCK_DGRAM, NPROTO_AT);

	if (common_socket_fd == -1) {
		return -errno;
	}

	return 0;
}

/*
 * Do any validation of an AT command not performed by the lower layers or
 * by the modem.
 */
static int check_cmd(const char *cmd)
{
	if (cmd == NULL) {
		return -EINVAL;
	}

	/* Check for the presence one printable non-whitespace character */
	for (const char *c = cmd; *c != '\0'; c++) {
		if (*c > ' ') {
			return 0;
		}
	}
	return -EINVAL;
}

static int get_return_code(char *buf, size_t bytes_read, struct resp_item *ret)
{
	bool match;
	char *tmpstr = NULL;
	int new_len  = 0;

	ret->state = AT_CMD_NOTIFICATION;

	do {
		/* must match `OK` at the end of the response */
		tmpstr = buf + bytes_read - ARRAY_SIZE(AT_CMD_OK_STR);
		match = !strncmp(tmpstr, AT_CMD_OK_STR, strlen(AT_CMD_OK_STR));
		if (match) {
			ret->state = AT_CMD_OK;
			ret->code  = 0;
			break;
		}

		tmpstr = strstr(buf, AT_CMD_CMS_STR);
		if (tmpstr) {
			match = true;
			ret->state = AT_CMD_ERROR_CMS;
			ret->code = atoi(&buf[ARRAY_SIZE(AT_CMD_CMS_STR) - 1]);
			break;
		}

		tmpstr = strstr(buf, AT_CMD_CME_STR);
		if (tmpstr) {
			match = true;
			ret->state = AT_CMD_ERROR_CME;
			ret->code = atoi(&buf[ARRAY_SIZE(AT_CMD_CME_STR) - 1]);
			break;
		}

		/* must match `ERROR` at the end of the response */
		tmpstr = buf + bytes_read - ARRAY_SIZE(AT_CMD_ERROR_STR);
		match = !strncmp(tmpstr, AT_CMD_ERROR_STR, strlen(AT_CMD_ERROR_STR));
		if (match) {
			ret->state = AT_CMD_ERROR;
			ret->code  = -ENOEXEC;
			break;
		}
	} while (0);

	if (match) {
		new_len = tmpstr - buf;
		buf[new_len++] = '\0';
	} else {
		new_len = strlen(buf) + 1;
	}

	return new_len;
}

static inline int at_write(const char *const cmd)
{
	int bytes_sent;
	int bytes_to_send = strlen(cmd);

	LOG_DBG("Sending command %s", log_strdup(cmd));

	bytes_sent = send(common_socket_fd, cmd, bytes_to_send, 0);

	if (bytes_sent == -1) {
		LOG_ERR("Failed to send AT command (err:%d)", errno);
		return -errno;
	}

	if (bytes_sent != bytes_to_send) {
		LOG_WRN("Bytes sent (%d) was not the same as expected (%d)",
			bytes_sent, bytes_to_send);
	}

	return 0;
}

/* Clear the current command safely */
static void complete_cmd(void)
{
	k_mutex_lock(&current_cmd_mutex, K_FOREVER);
	current_cmd.cmd = NULL;
	k_mutex_unlock(&current_cmd_mutex);
}

/*
 * Atomically load a new command if appropriate, then write it to the socket.
 * The operations are repeated until the queue is empty or a command is pending
 * a response. This function is called both from the socket thread and calling
 * context.
 */
static void load_cmd_and_write(void)
{
	int ret;
	struct resp_item resp;

	k_mutex_lock(&current_cmd_mutex, K_FOREVER);
	do {
		ret = 0;

		/* Do not load a new command if already loaded or none queued */
		if (current_cmd.cmd != NULL ||
		    k_msgq_get(&commands, &current_cmd, K_NO_WAIT) != 0) {
			break;
		}

		ret = at_write(current_cmd.cmd);

		if (current_cmd.flags & AT_CMD_BUF_CMD) {
			k_free(current_cmd.cmd);
		}

		/* If write failed, make an error response and complete cmd */
		if (ret != 0) {
			resp.state = AT_CMD_ERROR_WRITE;
			resp.code = ret;
			if (current_cmd.flags & AT_CMD_SYNC) {
				k_msgq_put(&response_sync, &resp, K_FOREVER);
			}
			complete_cmd();
		}
	} while (ret != 0);
	k_mutex_unlock(&current_cmd_mutex);
}

static void socket_thread_fn(void *arg1, void *arg2, void *arg3)
{
	static int bytes_read;
	static size_t payload_len;
	static struct resp_item ret;
	static char buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("AT socket thread started");

	for (;;) {
		LOG_DBG("Writing any pending command");
		load_cmd_and_write();

		LOG_DBG("Listening on socket");
		bytes_read = recv(common_socket_fd, buf, sizeof(buf), 0);

		/* Initialize the response */
		ret.code  = 0;
		ret.state = AT_CMD_OK;

		/* Handle possible socket-level errors */

		if (bytes_read < 0) {
			if (errno == EHOSTDOWN) {
				LOG_DBG("AT host is going down, sleeping");
				atomic_set(&shutdown_mode, 1);
				close(common_socket_fd);
				nrf_modem_lib_shutdown_wait();
				LOG_DBG("AT host available, "
					"starting the thread again");
				atomic_clear(&shutdown_mode);
				if (open_socket() != 0) {
					LOG_ERR("Failed to open AT socket "
						"after nrf_modem_lib init, "
						"err: %d", errno);
				}


				continue;
			} else {
				LOG_ERR("AT socket recv failed with err %d",
					errno);
			}

			if ((close(common_socket_fd) == 0) &&
			    (open_socket() == 0)) {
				LOG_INF("AT socket recovered");
				ret.state = AT_CMD_ERROR_READ;
				ret.code  = -errno;
				goto next;
			}

			LOG_ERR("Unrecoverable reception error (err: %d), "
				"thread killed", errno);
			close(common_socket_fd);
			return;
		} else if (bytes_read == 0) {
			LOG_ERR("AT message empty");
			ret.state = AT_CMD_ERROR_READ;
			ret.code  = -EBADMSG;
			goto next;
		} else if (buf[bytes_read - 1] != '\0') {
			LOG_ERR("AT message too large for reception buffer or "
				"missing termination character");
			ret.state = AT_CMD_ERROR_READ;
			ret.code  = -ENOBUFS;
			goto next;
		}

		LOG_DBG("at_cmd_rx %d bytes, %s", bytes_read, log_strdup(buf));

		payload_len = get_return_code(buf, bytes_read, &ret);

		/* Verify the buffer size if provided, and copy the message */
		if (current_cmd.cmd != NULL &&
		    current_cmd.resp != NULL &&
		    ret.state != AT_CMD_NOTIFICATION) {
			if (current_cmd.resp_size < payload_len) {
				LOG_ERR("Response buffer not large enough");
				ret.code  = -EMSGSIZE;
				goto next;
			}
			memcpy(current_cmd.resp, buf, payload_len);
		}

		/* Call the relevant callback, if any */
		if (ret.state == AT_CMD_NOTIFICATION &&
		    notification_handler != NULL) {
			notification_handler(buf);
		} else if (current_cmd.callback != NULL) {
			current_cmd.callback(buf);
		}

next:
		/* Dispatch response for sync call */
		if (current_cmd.cmd != NULL &&
		    current_cmd.flags & AT_CMD_SYNC &&
		    ret.state != AT_CMD_NOTIFICATION) {
			LOG_DBG("Enqueueing response for sync call");
			k_msgq_put(&response_sync, &ret, K_FOREVER);
		}

		/* We have now handled a command if it was not a notification */
		if (ret.state != AT_CMD_NOTIFICATION) {
			complete_cmd();
		}
	}
}

int at_cmd_write_with_callback(const char *const cmd,
			       at_cmd_handler_t  handler)
{
	struct cmd_item command;
	int ret;

	if (atomic_get(&shutdown_mode) == 1) {
		return -EHOSTDOWN;
	}

	if (cmd == NULL) {
		LOG_ERR("cmd is NULL");
		return -EINVAL;
	}

	if (check_cmd(cmd)) {
		LOG_ERR("Invalid command");
		return -EINVAL;
	}

	command.cmd = k_malloc(strlen(cmd) + 1);
	if (command.cmd == NULL) {
		return -ENOMEM;
	}
	strcpy(command.cmd, cmd);

	command.resp = NULL;
	command.callback = handler;
	command.flags = AT_CMD_BUF_CMD;

	ret = k_msgq_put(&commands, &command, K_FOREVER);
	if (ret) {
		return ret;
	}

	load_cmd_and_write();
	return 0;
}

int at_cmd_write(const char *const cmd,
		 char *buf,
		 size_t buf_len,
		 enum at_cmd_state *state)
{
	struct cmd_item command;
	struct resp_item ret;

	if (atomic_get(&shutdown_mode) == 1) {
		return -EHOSTDOWN;
	}

	__ASSERT(k_current_get() != socket_tid,
		 "at_cmd deadlock: socket thread blocking self\n");

	if (cmd == NULL) {
		LOG_ERR("cmd is NULL");
		if (state) {
			*state = AT_CMD_ERROR_QUEUE;
		}
		return -EINVAL;
	}

	if (check_cmd(cmd)) {
		LOG_ERR("Invalid command");
		if (state) {
			*state = AT_CMD_ERROR_QUEUE;
		}
		return -EINVAL;
	}

	/* This cast is safe; we do not free cmd without AT_CMD_BUF_CMD */
	command.cmd = (char *)cmd;
	command.resp = buf;
	command.resp_size = buf_len;
	command.callback = NULL;
	command.flags = AT_CMD_SYNC;

	/* Ensure we get our own AT response, not an old one */
	k_mutex_lock(&response_sync_get, K_FOREVER);

	/* We borrow the return code field from the currently unused response */
	ret.code = k_msgq_put(&commands, &command, K_FOREVER);
	if (ret.code) {
		LOG_ERR("Could not enqueue cmd, error %d", ret.code);
		if (state) {
			*state = AT_CMD_ERROR_QUEUE;
		}
		return ret.code;
	}

	load_cmd_and_write();

	LOG_DBG("Awaiting response for %s", log_strdup(cmd));
	k_msgq_get(&response_sync, &ret, K_FOREVER);
	k_mutex_unlock(&response_sync_get);

	if (state) {
		*state = ret.state;
	}

	return ret.code;
}

void at_cmd_set_notification_handler(at_cmd_handler_t handler)
{
	LOG_DBG("Setting notification handler to %p", handler);
	if (notification_handler != NULL && handler != notification_handler) {
		LOG_WRN("Forgetting prior notification handler %p",
			notification_handler);
	}
	notification_handler = handler;
}

static int at_cmd_driver_init(const struct device *dev)
{
	static bool initialized;

	if (initialized) {
		return 0;
	}

	int err;

	ARG_UNUSED(dev);

	err = open_socket();
	if (err) {
		LOG_ERR("Failed to open AT socket (err:%d)", err);
		return err;
	}

	LOG_DBG("Common AT socket created");

	socket_tid = k_thread_create(&socket_thread, socket_thread_stack,
				     K_THREAD_STACK_SIZEOF(socket_thread_stack),
				     socket_thread_fn,
				     NULL, NULL, NULL,
				     THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(socket_tid, "at_cmd_socket_thread");

	initialized = true;
	LOG_DBG("Common AT socket processing thread created");

	return 0;
}

int at_cmd_init(void)
{
	return at_cmd_driver_init(NULL);
}


#ifdef CONFIG_AT_CMD_SYS_INIT
SYS_INIT(at_cmd_driver_init, APPLICATION, CONFIG_AT_CMD_INIT_PRIORITY);
#endif
