/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Event manager proxy header.
 */
#ifndef _EVENT_MANAGER_PROXY_H_
#define _EVENT_MANAGER_PROXY_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <app_event_manager.h>

/**
 * @defgroup event_manager_proxy Event Manager Proxy
 * @brief Event Manager Proxy
 *
 * @{
 */

/**
 * @brief Subscribe for the remote event.
 *
 * Register the listener for the event from the remote core.
 * @param instance The instance used for IPC service to transfer data between cores.
 * @param ename Name of the event. The event name has to be the same on remote and local cores.
 * @return See @ref event_manager_proxy_subscribe.
 */
#define EVENT_MANAGER_PROXY_SUBSCRIBE(instance, ename) \
	event_manager_proxy_subscribe((instance), APP_EVENT_ID(ename))

/**
 * @brief Add remote core communication channel.
 *
 * This function registers endpoint used to communication with another core.
 *
 * @param instance The instance used for IPC service to transfer data between cores.
 * @retval -EALREADY Given remote instance was added already.
 * @retval -ENOMEM No place for the new endpoint. See @kconfig{CONFIG_EVENT_MANAGER_PROXY_CH_COUNT}.
 * @retval -EIO    Comes from IPC service,
 *                 see ipc_service_open_instance or ipc_service_register_endpoint.
 * @retval -EINVAL Comes from IPC service,
 *                 see ipc_service_open_instance or ipc_service_register_endpoint.
 * @retval -EBUSY  Comes from IPC service, see ipc_service_register_endpoint.
 *
 * @retval 0 On success.
 * @retval other errno codes depending on the IPC service backend implementation.
 */
int event_manager_proxy_add_remote(const struct device *instance);

/**
 * @brief Subscribe for the remote event.
 *
 * This function registers the local event proxy to remote event proxy to listen
 * selected event.
 *
 * @note
 * This function may wait for the IPC endpoint to bond.
 * To be sure that it will be available to bond write the code this way that all the remotes
 * are added first by @ref event_manager_proxy_add_remote and then start to add listeners.
 * In other case if the other core adds more than one remote, we run into risk that we will wait
 * for the endpoint to bond while the remote core waits for other endpoint to bond and never
 * configures the requested endpoint.
 *
 * @param instance Remote IPC instance.
 * @param local_event_id The id of the event we wish to receive for the event on the remote core.
 *
 * @retval 0 On success.
 * @retval -EACCES Function called after @ref event_manager_proxy_start.
 * @retval -EPIPE  The remote core did not bond in during timeout period. No communication.
 * @retval -ETIME  Timeout while waiting for the response from the other core.
 * @retval other errno code.
 */
int event_manager_proxy_subscribe(
	const struct device *instance,
	const struct event_type *local_event_id);

/**
 * @brief Start events transfer.
 *
 * This function sends start command to all added remote cores.
 * This command finalizes initialization process, therefore the functions
 * @ref event_manager_proxy_add_remote and @ref event_manager_proxy_subscribe
 * cannot be used anymore.
 *
 * @retval 0 On success.
 * @retval -EPIPE  The remote core did not bond in during timeout period. No communication.
 * @retval other errno code.
 */
int event_manager_proxy_start(void);

/**
 * @brief Wait for all the remote cores to report their readiness.
 *
 * This function stops the current thread if there is any remote that did not finish its
 * initialization.
 *
 * @note
 * Call this function only after @ref event_manager_proxy_start.
 * @note
 * This function may be called multiple times.
 *
 * @param timeout Waiting period for all the remote cores to finish theirs initialization.
 *
 * @retval 0 On success.
 * @retval -ETIME Timeout reached.
 */
int event_manager_proxy_wait_for_remotes(k_timeout_t timeout);

/** @} */
#endif /* _EVENT_MANAGER_PROXY_H_ */
