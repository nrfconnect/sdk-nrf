/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <event_manager.h>
#include "modules_common.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(modules_common, CONFIG_MODULES_COMMON_LOG_LEVEL);

struct event_prototype {
	struct event_header header;
	uint8_t event_id;
};

/* List containing metadata on active modules in the application. */
static sys_slist_t module_list = SYS_SLIST_STATIC_INIT(&module_list);
static K_MUTEX_DEFINE(module_list_lock);

/* Structure containing general information about the modules in the application. */
static struct modules_info {
	/* Modules that support shutdown. */
	atomic_t shutdown_supported_count;
	/* Number of active modules in the application. */
	atomic_t active_modules_count;
} modules_info;

/* Public interface */
void module_purge_queue(struct module_data *module)
{
	k_msgq_purge(module->msg_q);
}

int module_get_next_msg(struct module_data *module, void *msg)
{
	int err = k_msgq_get(module->msg_q, msg, K_FOREVER);

	if (err == 0 && IS_ENABLED(CONFIG_MODULES_COMMON_LOG_LEVEL_DBG)) {
		struct event_prototype *evt_proto =
			(struct event_prototype *)msg;
		struct event_type *event =
			(struct event_type *)evt_proto->header.type_id;
		char buf[50];

		event->log_event(&evt_proto->header, buf, sizeof(buf));

		LOG_DBG("%s module: Dequeued %s",
			module->name,
			log_strdup(buf));
	}

	return err;
}

int module_enqueue_msg(struct module_data *module, void *msg)
{
	int err;

	err = k_msgq_put(module->msg_q, msg, K_NO_WAIT);
	if (err) {
		LOG_WRN("%s: Message could not be enqueued, error code: %d",
			module->name, err);
			/* Purge message queue before reporting an error. This
			 * makes sure that the calling module can
			 * enqueue and process new events and is not being
			 * blocked by a full message queue.
			 *
			 * This error is concidered irrecoverable and should be
			 * rebooted on.
			 */
			module_purge_queue(module);
		return err;
	}

	if (IS_ENABLED(CONFIG_MODULES_COMMON_LOG_LEVEL_DBG)) {
		struct event_prototype *evt_proto =
			(struct event_prototype *)msg;
		struct event_type *event =
			(struct event_type *)evt_proto->header.type_id;
		char buf[50];

		event->log_event(&evt_proto->header, buf, sizeof(buf));

		LOG_DBG("%s module: Enqueued: %s", log_strdup(module->name),
			log_strdup(buf));
	}

	return 0;
}

bool modules_shutdown_register(uint32_t id_reg)
{
	bool retval = false;
	struct module_data *module, *next_module = NULL;

	if (id_reg == 0) {
		LOG_WRN("Passed in module ID cannot be 0");
		return false;
	}

	k_mutex_lock(&module_list_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&module_list, module, next_module, header) {
		if (module->id == id_reg) {
			if (module->supports_shutdown) {
				/* A module shutdown has been registered. Decrease the number of
				 * active modules in the application and delete the list entry for
				 * the corresponding module.
				 */
				sys_slist_find_and_remove(&module_list, &module->header);
				atomic_dec(&modules_info.active_modules_count);
				atomic_dec(&modules_info.shutdown_supported_count);

				LOG_WRN("Module \"%s\" shutdown registered", module->name);
			} else {
				goto exit;
			}
			break;
		}
	};

	if (modules_info.shutdown_supported_count == 0) {
		/* All modules in the application have reported a shutdown. */
		retval = true;
	}

exit:
	k_mutex_unlock(&module_list_lock);
	return retval;
}

int module_start(struct module_data *module)
{
	if (module == NULL) {
		LOG_ERR("Module metadata is NULL");
		return -EINVAL;
	}

	if (module->name == NULL) {
		LOG_ERR("Module name is NULL");
		return -EINVAL;
	}

	module->id = k_cycle_get_32();
	atomic_inc(&modules_info.active_modules_count);

	if (module->supports_shutdown) {
		atomic_inc(&modules_info.shutdown_supported_count);
	}

	/* Append passed in module metadata to linked list. */
	k_mutex_lock(&module_list_lock, K_FOREVER);
	sys_slist_append(&module_list, &module->header);
	k_mutex_unlock(&module_list_lock);

	if (module->thread_id) {
		LOG_DBG("Module \"%s\" with thread ID %p started", module->name, module->thread_id);
	} else {
		LOG_DBG("Module \"%s\" started", module->name);
	}

	return 0;
}

uint32_t module_active_count_get(void)
{
	return atomic_get(&modules_info.active_modules_count);
}
