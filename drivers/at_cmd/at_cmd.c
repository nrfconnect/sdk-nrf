/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <init.h>
#include <bsd_limits.h>

#include <at_cmd.h>

LOG_MODULE_REGISTER(at_cmd, CONFIG_AT_CMD_LOG_LEVEL);

#define THREAD_PRIORITY   K_PRIO_PREEMPT(CONFIG_AT_CMD_THREAD_PRIO)

#define AT_CMD_OK_STR    "OK"
#define AT_CMD_ERROR_STR "ERROR"
#define AT_CMD_CMS_STR   "+CMS:"
#define AT_CMD_CME_STR   "+CME:"

static K_THREAD_STACK_DEFINE(socket_thread_stack, \
				CONFIG_AT_CMD_THREAD_STACK_SIZE);

static K_SEM_DEFINE(cmd_pending, 1, 1);

static int              common_socket_fd;
static char             *response_buf;
static u32_t            response_buf_len;

static struct k_thread  socket_thread;
static at_cmd_handler_t notification_handler;
static at_cmd_handler_t current_cmd_handler;

struct return_state_object {
	int               code;
	enum at_cmd_state state;
};

K_MSGQ_DEFINE(return_code_msq, sizeof(struct return_state_object), 1, 4);

struct callback_work_item {
	struct k_work    work;
	char             data[CONFIG_AT_CMD_RESPONSE_MAX_LEN];
	at_cmd_handler_t callback;
};

K_MEM_SLAB_DEFINE(rsp_work_items, sizeof(struct callback_work_item),
		  CONFIG_AT_CMD_RESPONSE_BUFFER_COUNT, 4);

static int open_socket(void)
{
	common_socket_fd = socket(AF_LTE, 0, NPROTO_AT);

	if (common_socket_fd == -1) {
		return -errno;
	}

	return 0;
}

static int get_return_code(char *buf, struct return_state_object *ret)
{
	char *tmpstr = NULL;
	int new_len  = 0;

	ret->state = AT_CMD_NOTIFICATION;

	do {
		tmpstr = strstr(buf, AT_CMD_OK_STR);
		if (tmpstr) {
			ret->state = AT_CMD_OK;
			ret->code  = 0;
			break;
		}

		tmpstr = strstr(buf, AT_CMD_ERROR_STR);
		if (tmpstr) {
			ret->state = AT_CMD_ERROR;
			ret->code  = -ENOEXEC;
			break;
		}

		tmpstr = strstr(buf, AT_CMD_CMS_STR);
		if (tmpstr) {
			ret->state = AT_CMD_ERROR_CMS;
			ret->code = atoi(&buf[ARRAY_SIZE(AT_CMD_CMS_STR) - 1]);
			break;
		}

		tmpstr = strstr(buf, AT_CMD_CME_STR);
		if (tmpstr) {
			ret->state = AT_CMD_ERROR_CME;
			ret->code = atoi(&buf[ARRAY_SIZE(AT_CMD_CMS_STR) - 1]);
			break;
		}
	} while (0);

	if (tmpstr) {
		new_len = tmpstr - buf;
		buf[new_len++] = '\0';
	} else {
		new_len = strlen(buf) + 1;
	}

	return new_len;
}

static void callback_worker(struct k_work *item)
{
	struct callback_work_item *data =
		CONTAINER_OF(item, struct callback_work_item, work);

	if (data != NULL) {
		data->callback(data->data);
	}

	k_mem_slab_free(&rsp_work_items, (void **)&data);
}


static void socket_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int                        bytes_read;
	int                        payload_len;
	static char                buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];
	bool                       callback = true;
	struct return_state_object ret = {
						.state = AT_CMD_OK,
						.code  = 0,
					};

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("AT socket thread started");

	for (;;) {
		bytes_read = recv(common_socket_fd, buf, sizeof(buf), 0);

		if (bytes_read < 0) {
			LOG_ERR("AT socket recv failed with err %d",
				bytes_read);

			if ((close(common_socket_fd) == 0) &&
			    (open_socket() == 0)) {
				LOG_INF("AT socket recovered");

				if (k_sem_count_get(&cmd_pending) == 0) {
					ret.state = AT_CMD_ERROR;
					ret.code  = -errno;
					goto next;
				} else {
					continue;
				}
			}

			LOG_ERR("Unrecoverable reception error (err: %d), "
				"thread killed", errno);
			close(common_socket_fd);
			return;
		} else if (bytes_read == sizeof(buf) ||
			   buf[bytes_read - 1] != '\0') {

			LOG_ERR("AT message to large for reception buffer or "
				"missing termination character");

			ret.code  = -ENOBUFS;
			goto next;
		}

		payload_len = get_return_code(buf, &ret);

		if (ret.state != AT_CMD_NOTIFICATION) {
			if ((response_buf_len > 0) &&
			    (response_buf != NULL)) {
				if (response_buf_len > payload_len) {
					memcpy(response_buf,
						buf, payload_len);
				} else {
					LOG_ERR("Response buffer not large "
						"enough");

					ret.code  = -EMSGSIZE;
				}

				callback         = false;
				response_buf_len = 0;
				response_buf     = NULL;
			}
		}

		if (payload_len == 0) {
			goto next;
		}

		if (callback) {
			at_cmd_handler_t tmp_callback;

			if (ret.state == AT_CMD_NOTIFICATION) {
				tmp_callback = notification_handler;
			} else {
				tmp_callback = current_cmd_handler;
			}

			if (!tmp_callback) {
				goto next;
			}

			struct callback_work_item *item;


			if (k_mem_slab_alloc(&rsp_work_items,
					     (void **)&item, K_NO_WAIT) != 0) {
				LOG_DBG("Failed to allocate work item");

				ret.code  = -ENOMEM;

				goto next;
			}

			item->callback = tmp_callback;

			k_work_init(&item->work, callback_worker);

			memcpy(item->data, buf, payload_len);

			k_work_submit(&item->work);
		}
next:
		callback            = true;
		current_cmd_handler = NULL;

		if (ret.state != AT_CMD_NOTIFICATION) {
			struct return_state_object ret_copy = {
				.state = ret.state,
				.code  = ret.code,
			};

			ret.code  = 0;
			ret.state = AT_CMD_OK;

			k_msgq_put(&return_code_msq, &ret_copy, K_FOREVER);
		}
	}
}

static inline int at_write(const char *const cmd, enum at_cmd_state *state)
{
	int bytes_sent;
	int bytes_to_send = strlen(cmd);
	struct return_state_object ret;

	LOG_DBG("Sending command %s", log_strdup(cmd));

	bytes_sent = send(common_socket_fd, cmd, bytes_to_send, 0);

	if (bytes_sent == -1) {
		LOG_ERR("Failed to send AT command (err:%d)", errno);
		ret.code  = -errno;
		ret.state = AT_CMD_ERROR;
	} else {
		k_msgq_get(&return_code_msq, &ret, K_FOREVER);
		LOG_DBG("Bytes sent: %d", bytes_sent);

		if (bytes_sent != bytes_to_send) {
			LOG_ERR("Bytes sent (%d) was not the "
				"same as expected (%d)",
				bytes_sent, bytes_to_send);
		}
	}

	if (state) {
		*state = ret.state;
	}

	return ret.code;
}

int at_cmd_write_with_callback(const char *const cmd,
			       at_cmd_handler_t  handler,
			       enum at_cmd_state *state)
{
	k_sem_take(&cmd_pending, K_FOREVER);

	current_cmd_handler = handler;

	int return_code = at_write(cmd, state);

	k_sem_give(&cmd_pending);

	return return_code;
}

int at_cmd_write(const char *const cmd,
		 char *buf,
		 size_t buf_len,
		 enum at_cmd_state *state)
{
	k_sem_take(&cmd_pending, K_FOREVER);

	response_buf     = buf;
	response_buf_len = buf_len;

	int return_code = at_write(cmd, state);

	k_sem_give(&cmd_pending);

	return return_code;
}

void at_cmd_set_notification_handler(at_cmd_handler_t handler)
{

	k_sem_take(&cmd_pending, K_FOREVER);

	notification_handler = handler;

	k_sem_give(&cmd_pending);
}

static int at_cmd_driver_init(struct device *dev)
{
	int err;

	ARG_UNUSED(dev);

	err = open_socket();
	if (err) {
		LOG_ERR("Failed to open AT socket (err:%d)", err);
		return err;
	}

	LOG_DBG("Common AT socket created");

	k_thread_create(&socket_thread, socket_thread_stack,
			K_THREAD_STACK_SIZEOF(socket_thread_stack),
			socket_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

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
