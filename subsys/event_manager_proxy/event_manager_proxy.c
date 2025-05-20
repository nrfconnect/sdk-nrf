/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <app_event_manager.h>
#include <event_manager_proxy.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>

LOG_MODULE_REGISTER(event_manager_proxy, CONFIG_APP_EVENT_MANAGER_LOG_LEVEL);


#define EMP_BIND_TIMEOUT K_MSEC(CONFIG_EVENT_MANAGER_PROXY_BIND_TIMEOUT_MS)

/* Helpers - allow linker to get information about these structure sizes. */
static struct event_type _emp_event_type_size_check
	__used __attribute__((__section__("event_manager_proxy_event_type_size")));
static struct event_type *_emp_event_type_pointer_size_check
	__used __attribute__((__section__("event_manager_proxy_event_type_pointer_size")));

/* Array used for inter-core event type mapping. */
extern struct event_type *event_manager_proxy_array[];
extern struct event_type *_event_manager_proxy_array_list_end[];


/** @brief Command codes used by the proxy. */
enum emp_cmd_code {
	EMP_CMD_SUBSCRIBE,
	EMP_CMD_START,
	EMP_CMD_COUNT,
	EMP_CMD_FORCE_INT_SIZE = INT_MAX
};

/**
 * @brief The command base structure.
 */
struct emp_cmd {
	enum emp_cmd_code code;
};

/**
 * @brief The command structure used to subscribe.
 */
struct emp_cmd_subscribe {
	enum emp_cmd_code code;
	const struct event_type *id;
	char name[];
};

/** @brief Inter-core communication data. */
struct emp_ipc_data {
	struct ipc_ept ept;
	struct ipc_ept_cfg ept_cfg;
	bool used;
	bool started;
	struct k_event bound;
	const struct event_type **event_type_map;
};


/** @brief True if proxy was started. */
static bool emp_started;

/** @brief Event informing all remotes have sent start. */
static K_EVENT_DEFINE(emp_all_remotes_started);

/** @brief IPC communication data. One entry per connected core. */
static struct emp_ipc_data emp_ipc_data[CONFIG_EVENT_MANAGER_PROXY_CH_COUNT];


/**
 * @brief Find IPC structure by the given instance.
 *
 * @param instance The instance used for IPC service to transfer data between cores.
 *
 * @retval NULL Instance not found (not added?).
 * @retval other The pointer to the requested instance.
 */
static struct emp_ipc_data *find_ipc_by_instance(const struct device *instance)
{
	for (size_t i = 0; i < ARRAY_SIZE(emp_ipc_data); ++i) {
		if ((emp_ipc_data[i].used) &&
		    (emp_ipc_data[i].ept.instance == instance)) {
			return &emp_ipc_data[i];
		}
	}

	return NULL;
}

/**
 * @brief Find event type by name.
 *
 * @param name The name of the event.
 *
 * @retval NULL    Cannot find event.
 * @retval pointer Pointer to the event type structure.
 */
static struct event_type *find_event_by_name(const char *name)
{
	STRUCT_SECTION_FOREACH(event_type, et) {
		if (!strcmp(et->name, name)) {
			return et;
		}
	}

	return NULL;
}

/**
 * @brief Get event type position index on the event type array.
 *
 * @param et Event type pointer.
 *
 * @return Event type index.
 */
static size_t et2idx(const struct event_type *et)
{
	APP_EVENT_ASSERT_ID(et);

	return et - _event_type_list_start;
}

/**
 * @brief Get ipc data position index on the ipc data array.
 *
 * @param ipc Element of the @ref emp_ipc_data array.
 *
 * @return The index of the element on the array.
 */
static size_t ipc2idx(const struct emp_ipc_data *ipc)
{
	__ASSERT_NO_MSG(PART_OF_ARRAY(emp_ipc_data, ipc));

	return ipc - emp_ipc_data;
}

/**
 * @brief The IPC endpoint bound by remote.
 *
 * @param priv The pointer of the related element of the @ref emp_ipc_data array.
 */
static void handle_ipc_endpoint_bound(void *priv)
{
	struct emp_ipc_data *ipc = priv;

	k_event_set(&ipc->bound, 0x1);
}

static void handle_remote_event(struct emp_ipc_data *ipc, const void *data, size_t len)
{
	void *event = app_event_manager_alloc(len);

	memcpy(event, data, len);
	_event_submit(event);
}

static void handle_remote_command_subscribe(struct emp_ipc_data *ipc, const void *data, size_t len)
{
	if (ipc->started) {
		/* Reject if started. */
		__ASSERT_NO_MSG(false);
		return;
	}

	const struct emp_cmd_subscribe *cmd = data;

	/* At least 1 name character required. */
	if (len < (sizeof(*cmd) + 2)) {
		LOG_ERR("Unexpected command size: %zu", len);
		__ASSERT_NO_MSG(false);
		return;
	}

	struct event_type *et = find_event_by_name(cmd->name);

	if (!et) {
		LOG_ERR("Cannot register event: %s", cmd->name);
	} else {
		size_t ctx_idx = ipc2idx(ipc);
		size_t et_idx = et2idx(et);

		ipc->event_type_map[et_idx] = cmd->id;
		LOG_DBG("Remote event %s registered on ipc %zu", cmd->name, ctx_idx);
	}
}

static void handle_remote_command_start(struct emp_ipc_data *ipc, const void *data, size_t len)
{
	if (ipc->started) {
		/* Reject if started. */
		__ASSERT_NO_MSG(false);
		return;
	}

	ipc->started = true;

	LOG_DBG("Event transmission on ipc %d started", ipc2idx(ipc));

	/* Check if all remote cores started. */
	for (size_t i = 0; i < ARRAY_SIZE(emp_ipc_data); ++i) {
		ipc = &emp_ipc_data[i];

		if (ipc->used && !ipc->started) {
			return;
		}
	}

	k_event_set(&emp_all_remotes_started, 0x1);
}

static void handle_remote_command(struct emp_ipc_data *ipc, const void *data, size_t len)
{
	const struct emp_cmd *cmd = data;

	if (len < sizeof(*cmd)) {
		LOG_ERR("Unexpected command size: %zu", len);
		__ASSERT_NO_MSG(false);
		return;
	}

	switch (cmd->code) {
	case EMP_CMD_SUBSCRIBE:
		handle_remote_command_subscribe(ipc, data, len);
		break;

	case EMP_CMD_START:
		handle_remote_command_start(ipc, data, len);
		break;

	default:
		LOG_ERR("Unsupported command %u", cmd->code);
		__ASSERT_NO_MSG(false);
		break;
	}
}

/**
 * @brief The data received on endpoint callback.
 *
 * This callback is called when there is some data received from the remote core.
 *
 * @param data The pointer to the data received.
 * @param len  The length of the data received.
 * @param priv The pointer of the related element of the @ref emp_ipc_data array.
 */
static void handle_ipc_data_receive(const void *data, size_t len, void *priv)
{
	struct emp_ipc_data *ipc = priv;

	/* Execute only from threads! */
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (ipc->started && emp_started) {
		handle_remote_event(ipc, data, len);
	} else {
		handle_remote_command(ipc, data, len);
	}
}

/**
 * @brief The error on the endpoint callback.
 *
 * @param message The message from the backend.
 * @param priv    The pointer to the related element of the @ref emp_ipc_data array.
 */
static void handle_ipc_endpoint_error(const char *message, void *priv)
{
	LOG_ERR("Endpoint error: \"%s\"", message);
	__ASSERT_NO_MSG(false);
}

static int send_event_to_remote(struct emp_ipc_data *ipc, const struct app_event_header *eh)
{
	const struct event_type *remote_ev = ipc->event_type_map[et2idx(eh->type_id)];
	int ret;

	if (remote_ev == NULL) {
		return 0;
	}

	size_t size = app_event_manager_event_size(eh);
	uint32_t buffer[DIV_ROUND_UP(size, sizeof(uint32_t))];
	struct app_event_header *remote_eh = (struct app_event_header *)buffer;

	memcpy(buffer, eh, sizeof(buffer));
	remote_eh->type_id = remote_ev;

	for (size_t cnt = CONFIG_EVENT_MANAGER_PROXY_SEND_RETRIES + 1; cnt > 0; --cnt) {
		ret = ipc_service_send(&ipc->ept, buffer, sizeof(buffer));
		if (ret >= 0) {
			break;
		}
		k_usleep(1);
	}

	if (ret < 0) {
		LOG_ERR("Cannot send event to remote %p, err: %d", ipc, ret);
		__ASSERT_NO_MSG(false);
	}

	return ret;
}

static void event_manager_proxy_on_event_process(const struct app_event_header *eh)
{
	int ret = 0;

	if (!emp_started) {
		return;
	}

	for (size_t i = 0; (i < ARRAY_SIZE(emp_ipc_data)) && !ret; ++i) {
		struct emp_ipc_data *ipc = &emp_ipc_data[i];

		if (!ipc->used || !ipc->started) {
			continue;
		}

		ret = send_event_to_remote(ipc, eh);
	}
}
APP_EVENT_HOOK_POSTPROCESS_REGISTER(event_manager_proxy_on_event_process);

static int add_ipc_instace(struct emp_ipc_data *ipc, const struct device *instance)
{
	int ret = ipc_service_open_instance(instance);

	if (ret && ret != -EALREADY) {
		LOG_ERR("IPC service open instance failure: %d", ret);
		return ret;
	}

	ipc->started = false;
	ipc->ept_cfg = (struct ipc_ept_cfg) {
		.name = "event_manager_proxy",
		.cb = {
			.bound    = handle_ipc_endpoint_bound,
			.received = handle_ipc_data_receive,
			.error    = handle_ipc_endpoint_error
		},
		.priv = ipc
	};

	size_t event_type_count = _event_type_list_end - _event_type_list_start;

	ipc->event_type_map = (void *)&event_manager_proxy_array[ipc2idx(ipc) * event_type_count];
	__ASSERT_NO_MSG((char *)ipc->event_type_map < (char *)_event_manager_proxy_array_list_end);
	__ASSERT_NO_MSG((char *)(ipc->event_type_map + event_type_count) <=
			(char *)_event_manager_proxy_array_list_end);
	memset(ipc->event_type_map, 0, event_type_count * sizeof(ipc->event_type_map[0]));

	k_event_init(&ipc->bound);

	ret = ipc_service_register_endpoint(instance, &ipc->ept, &ipc->ept_cfg);
	if (ret) {
		LOG_ERR("Error registering endpoint in ipc service (%d)", ret);
		return ret;
	}

	ipc->used = true;

	return 0;
}

int event_manager_proxy_add_remote(const struct device *instance)
{
	__ASSERT_NO_MSG(find_ipc_by_instance(instance) == NULL);

	if (find_ipc_by_instance(instance) != NULL) {
		return -EALREADY;
	}

	for (size_t i = 0; i < ARRAY_SIZE(emp_ipc_data); ++i) {
		if (!emp_ipc_data[i].used) {
			return add_ipc_instace(&emp_ipc_data[i], instance);
		}
	}

	LOG_ERR("No free space for another remote");

	return -ENOMEM;
}

static int send_subscribe_command_to_remote(struct emp_ipc_data *ipc,
					   const struct event_type *local_event_id,
					   const char *remote_event_name)
{
	__ASSERT_NO_MSG(ipc);

	if (!k_event_wait(&ipc->bound, 0x1, false, EMP_BIND_TIMEOUT)) {
		LOG_ERR("IPC bind timeout");
		return -EPIPE;
	}

	/* Preparing and sending the command */
	struct emp_cmd_subscribe *cmd;
	size_t size = sizeof(*cmd) + strlen(remote_event_name) + 1;
	uint32_t buffer[DIV_ROUND_UP(size, sizeof(uint32_t))];

	cmd = (struct emp_cmd_subscribe *)buffer;
	cmd->code = EMP_CMD_SUBSCRIBE;
	cmd->id  = local_event_id;
	strcpy(cmd->name, remote_event_name);

	int ret = ipc_service_send(&ipc->ept, buffer, sizeof(buffer));

	if (ret < 0) {
		return ret;
	}

	return 0;
}

int event_manager_proxy_subscribe(const struct device *instance,
				  const struct event_type *local_event_id)
{
	__ASSERT_NO_MSG(!emp_started);

	struct emp_ipc_data *ipc = find_ipc_by_instance(instance);

	return send_subscribe_command_to_remote(ipc, local_event_id, local_event_id->name);
}

static int send_start_command_to_remote(struct emp_ipc_data *ipc)
{
	const struct emp_cmd cmd = {.code = EMP_CMD_START};

	__ASSERT_NO_MSG(ipc);

	if (!k_event_wait(&ipc->bound, 0x1, false, EMP_BIND_TIMEOUT)) {
		LOG_ERR("IPC bind timeout");
		return -EPIPE;
	}

	int ret = ipc_service_send(&ipc->ept, &cmd, sizeof(cmd));

	if (ret < 0) {
		return ret;
	}

	return 0;
}

int event_manager_proxy_start(void)
{
	int ret = 0;

	__ASSERT_NO_MSG(!emp_started);

	for (size_t i = 0; (i < ARRAY_SIZE(emp_ipc_data)) && !ret; ++i) {
		struct emp_ipc_data *ipc = &emp_ipc_data[i];

		if (!ipc->used) {
			continue;
		}

		ret = send_start_command_to_remote(ipc);
	}

	if (!ret) {
		emp_started = true;
	}

	return ret;
}

int event_manager_proxy_wait_for_remotes(k_timeout_t timeout)
{
	__ASSERT_NO_MSG(emp_started);

	if (!k_event_wait(&emp_all_remotes_started, 0x1, false, timeout)) {
		return -ETIME;
	}

	return 0;
}
