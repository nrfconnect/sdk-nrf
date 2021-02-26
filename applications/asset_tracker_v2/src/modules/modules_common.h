/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULES_COMMON_H_
#define _MODULES_COMMON_H_

#define IS_EVENT(_ptr, _mod, _evt) \
		is_ ## _mod ## _module_event(&_ptr->module._mod.header) &&     \
		_ptr->module._mod.type == _evt

#define SEND_EVENT(_mod, _type)						       \
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event(); \
	event->type = _type;						       \
	EVENT_SUBMIT(event)

#define SEND_ERROR(_mod, _type, _error_code)				      \
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event(); \
	event->type = _type;						      \
	event->data.err = _error_code;					      \
	EVENT_SUBMIT(event)

struct module_data {
	k_tid_t thread_id;
	char *name;
	struct k_msgq *msg_q;
};

void module_register(struct module_data *module);

void module_set_thread(struct module_data *module, const k_tid_t thread_id);

void module_set_name(struct module_data *module, char *name);

void module_set_queue(struct module_data *module,  struct k_msgq *msg_q);

void module_purge_queue(struct module_data *module);

int module_get_next_msg(struct module_data *module, void *msg);

/** @brief Enqueue message to a module's queue.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_enqueue_msg(struct module_data *module, void *msg);

void module_start(struct module_data *module);

uint32_t module_active_count_get(void);

#endif /* _MODULES_COMMON_H_ */
