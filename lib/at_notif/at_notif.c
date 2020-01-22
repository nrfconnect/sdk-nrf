/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <init.h>
#include <at_cmd.h>
#include <at_notif.h>
#include <sys/slist.h>

LOG_MODULE_REGISTER(at_notif, CONFIG_AT_NOTIF_LOG_LEVEL);

static K_MUTEX_DEFINE(list_mtx);

/**@brief Link list element for notification handler. */
struct notif_handler {
	sys_snode_t        node;
	void               *ctx;
	at_notif_handler_t handler;
};

static sys_slist_t handler_list;


/**
 * @brief Find the handler from the notification list.
 *
 * @return The node or NULL if not found and its previous node in @p prev_out.
 */
static struct notif_handler *find_node(struct notif_handler **prev_out,
	void *ctx, at_notif_handler_t handler)
{
	struct notif_handler *prev = NULL, *curr, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&handler_list, curr, tmp, node) {
		if (curr->ctx == ctx && curr->handler == handler) {
			*prev_out = prev;
			return curr;
		}
		prev = curr;
	}
	return NULL;
}

/**@brief Add the handler in the notification list if not already present. */
static int append_notif_handler(void *ctx, at_notif_handler_t handler)
{
	struct notif_handler *to_ins;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Check if handler is already registered. */
	if (find_node(&to_ins, ctx, handler) != NULL) {
		LOG_DBG("Handler already registered. Nothing to do");
		k_mutex_unlock(&list_mtx);
		return 0;
	}

	/* Allocate memory and fill. */
	to_ins = (struct notif_handler *)k_malloc(sizeof(struct notif_handler));
	if (to_ins == NULL) {
		k_mutex_unlock(&list_mtx);
		return -ENOBUFS;
	}
	memset(to_ins, 0, sizeof(struct notif_handler));
	to_ins->ctx     = ctx;
	to_ins->handler = handler;

	/* Insert handler in the list. */
	sys_slist_append(&handler_list, &to_ins->node);
	k_mutex_unlock(&list_mtx);
	return 0;
}

/**@brief Remove the handler from the notification list if registered. */
static int remove_notif_handler(void *ctx, at_notif_handler_t handler)
{
	struct notif_handler *curr, *prev = NULL;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Check if the handler is registered before removing it. */
	curr = find_node(&prev, ctx, handler);
	if (curr == NULL) {
		LOG_WRN("Handler not registered. Nothing to do");
		k_mutex_unlock(&list_mtx);
		return 0;
	}

	/* Remove the handler from the list. */
	sys_slist_remove(&handler_list, &prev->node, &curr->node);
	k_free(curr);

	k_mutex_unlock(&list_mtx);
	return 0;
}

/**@brief AT command notifications handler. */
static void notif_dispatch(const char *response)
{
	struct notif_handler *curr, *tmp;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Dispatch notifications to all registered handlers */
	LOG_DBG("Dispatching events:");
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&handler_list, curr, tmp, node) {
		LOG_DBG(" - ctx=0x%08X, handler=0x%08X", (u32_t)curr->ctx,
			(u32_t)curr->handler);
		curr->handler(curr->ctx, response);
	}
	LOG_DBG("Done");

	k_mutex_unlock(&list_mtx);
}

static int module_init(struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Initialization");
	sys_slist_init(&handler_list);
	at_cmd_set_notification_handler(notif_dispatch);
	return 0;
}

int at_notif_init(void)
{
	return module_init(NULL);
}

int at_notif_register_handler(void *context, at_notif_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("Invalid handler (context=0x%08X, handler=0x%08X)",
			(u32_t)context, (u32_t)handler);
		return -EINVAL;
	}
	return append_notif_handler(context, handler);
}

int at_notif_deregister_handler(void *context, at_notif_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("Invalid handler (context=0x%08X, handler=0x%08X)",
			(u32_t)context, (u32_t)handler);
		return -EINVAL;
	}
	return remove_notif_handler(context, handler);
}

#ifdef CONFIG_AT_NOTIF_SYS_INIT
SYS_INIT(module_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
