/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULES_COMMON_H_
#define _MODULES_COMMON_H_

#include <zephyr.h>

#define IS_EVENT(_ptr, _mod, _evt) \
		is_ ## _mod ## _module_event(&_ptr->module._mod.header) &&		\
		_ptr->module._mod.type == _evt

#define SEND_EVENT(_mod, _type)								\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	event->type = _type;								\
	EVENT_SUBMIT(event)

#define SEND_ERROR(_mod, _type, _error_code)						\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	event->type = _type;								\
	event->data.err = _error_code;							\
	EVENT_SUBMIT(event)

#define SEND_SHUTDOWN_ACK(_mod, _type, _id)						\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	event->type = _type;								\
	event->data.id = _id;								\
	EVENT_SUBMIT(event)

struct module_data {
	/* Variable used to construct a linked list of module metadata. */
	sys_snode_t header;
	/* ID specific to each module. Internally assigned when calling module_start(). */
	uint32_t id;
	k_tid_t thread_id;
	char *name;
	struct k_msgq *msg_q;
	/* Flag signifying if the module supports shutdown. */
	bool supports_shutdown;
};

void module_purge_queue(struct module_data *module);

int module_get_next_msg(struct module_data *module, void *msg);

/** @brief Enqueue message to a module's queue.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_enqueue_msg(struct module_data *module, void *msg);

/** @brief Register that a module has performed a graceful shutdown.
 *
 *  @param id_reg Identifier of module.
 *
 *  @return true If this API has been called for all modules supporting graceful shutdown in the
 *	    application.
 */
bool modules_shutdown_register(uint32_t id_reg);

/** @brief Register and start a module.
 *
 *  @param module Pointer to a structure containing module metadata.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_start(struct module_data *module);

uint32_t module_active_count_get(void);

#endif /* _MODULES_COMMON_H_ */
