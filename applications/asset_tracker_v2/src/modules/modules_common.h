/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULES_COMMON_H_
#define _MODULES_COMMON_H_

/**@file
 *@brief Modules common library header.
 */

#include <zephyr/kernel.h>

/**
 * @defgroup modules_common Modules common library
 * @{
 * @brief A Library that exposes generic functionality shared between modules.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Macro that checks if an event is of a certain type.
 *
 * @param _ptr Name of module message struct variable.
 * @param _mod Name of module that the event corresponds to.
 * @param _evt Name of the event.
 *
 * @return true if the event matches the event checked for, otherwise false.
 */
#define IS_EVENT(_ptr, _mod, _evt) \
		is_ ## _mod ## _module_event(&_ptr->module._mod.header) &&		\
		_ptr->module._mod.type == _evt

/** @brief Macro used to submit an event.
 *
 * @param _mod Name of module that the event corresponds to.
 * @param _type Name of the type of event.
 */
#define SEND_EVENT(_mod, _type)								\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	__ASSERT(event, "Not enough heap left to allocate event");			\
	event->type = _type;								\
	APP_EVENT_SUBMIT(event)

/** @brief Macro used to submit an error event.
 *
 * @param _mod Name of module that the event corresponds to.
 * @param _type Name of the type of error event.
 * @param _error_code Error code.
 */
#define SEND_ERROR(_mod, _type, _error_code)						\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	__ASSERT(event, "Not enough heap left to allocate event");			\
	event->type = _type;								\
	event->data.err = _error_code;							\
	APP_EVENT_SUBMIT(event)

/** @brief Macro used to submit a shutdown event.
 *
 * @param _mod Name of module that the event corresponds to.
 * @param _type Name of the type of shutdown event.
 * @param _id ID of the module that acknowledges the shutdown.
 */
#define SEND_SHUTDOWN_ACK(_mod, _type, _id)						\
	struct _mod ## _module_event *event = new_ ## _mod ## _module_event();		\
	__ASSERT(event, "Not enough heap left to allocate event");			\
	event->type = _type;								\
	event->data.id = _id;								\
	APP_EVENT_SUBMIT(event)

/** @brief Structure that contains module metadata. */
struct module_data {
	/* Variable used to construct a linked list of module metadata. */
	sys_snode_t header;
	/* ID specific to each module. Internally assigned when calling module_start(). */
	uint32_t id;
	/* The ID of the module thread. */
	k_tid_t thread_id;
	/* Name of the module. */
	char *name;
	/* Pointer to the internal message queue in the module. */
	struct k_msgq *msg_q;
	/* Flag signifying if the module supports shutdown. */
	bool supports_shutdown;
};

/** @brief Purge a module's queue.
 *
 *  @param[in] module Pointer to a structure containing module metadata.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
void module_purge_queue(struct module_data *module);

/** @brief Get the next message in a module's queue.
 *
 *  @param[in] module Pointer to a structure containing module metadata.
 *  @param[out] msg Pointer to a message buffer that the output will be written to.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_get_next_msg(struct module_data *module, void *msg);

/** @brief Enqueue message to a module's queue.
 *
 *  @param[in] module Pointer to a structure containing module metadata.
 *  @param[in] msg Pointer to a message that will be enqueued.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_enqueue_msg(struct module_data *module, void *msg);

/** @brief Register that a module has performed a graceful shutdown.
 *
 *  @param[in] id_reg Identifier of module.
 *
 *  @return true If this API has been called for all modules supporting graceful shutdown in the
 *	    application.
 */
bool modules_shutdown_register(uint32_t id_reg);

/** @brief Register and start a module.
 *
 *  @param[in] module Pointer to a structure containing module metadata.
 *
 *  @return 0 if successful, otherwise a negative error code.
 */
int module_start(struct module_data *module);

/** @brief Get the number of active modules in the application.
 *
 *  @return Number of active modules in the application.
 */
uint32_t module_active_count_get(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* _MODULES_COMMON_H_ */
