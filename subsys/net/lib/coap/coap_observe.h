/*
 * Copyright (c) 2015 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_observe.h
 *
 * @defgroup iot_sdk_coap_observe CoAP Observe
 * @ingroup iot_sdk_coap
 * @{
 * @brief Internal API of Nordic's CoAP Observe implementation.
 */
#ifndef COAP_OBSERVE_H__
#define COAP_OBSERVE_H__

#include <stdint.h>

#include <net/coap_api.h>
#include <net/coap_observe_api.h>

#include "coap_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@cond NO_DOXYGEN */

/**@brief Register a new observer.
 *
 * @param[out] handle   Handle to the observer instance registered. Returned by
 *                      reference. Should not be NULL.
 * @param[in]  observer Pointer to the observer structure to register. The data
 *                      will be copied. Should not be NULL.
 *
 * @retval 0      If the observer was registered successfully.
 * @retval ENOMEM If the observer could not be added to the list.
 * @retval EINVAL If one of the parameters is a NULL pointer.
 */
u32_t internal_coap_observe_server_register(u32_t *handle,
					    coap_observer_t *observer);

/**@brief Unregister an observer.
 *
 * @details Unregister the observer and clear the memory used by this instance.
 *
 * @param[in] handle Handle to the observer instance registered.
 *
 * @retval 0      If the observer was successfully unregistered.
 * @retval ENOENT If the given handle was not found in the observer list.
 */
u32_t internal_coap_observe_server_unregister(u32_t handle);

/**@brief Search the observer list for an observer matching remote address and
 *        subject given.
 *
 * @param[out] handle        Handle to the observer instance registered.
 *                           Returned by reference. Should not be NULL.
 * @param[in]  observer_addr Pointer to an address structure giving remote
 *                           address of the observer and port number.
 *                           Should not be NULL.
 * @param[in]  resource      Pointer to the resource the observer is registered
 *                           to. Should not be NULL.
 *
 * @retval 0      If observer was found in the observer list.
 * @retval EINVAL If one of the pointers are NULL.
 * @retval ENOENT If observer was not found.
 */
u32_t internal_coap_observe_server_search(u32_t *handle,
					  struct sockaddr *observer_addr,
					  coap_resource_t *resource);

/**@brief Iterate through observers subscribing to a specific resource.
 *
 * @param[out] observer Pointer to be filled by the search function upon finding
 *                      the next observer starting from the observer pointer
 *                      provided. Should not be NULL.
 * @param[in]  observer Pointer to the observer where to start the search.
 * @param[in]  resource Pointer to the resource of interest. Should not be NULL.
 *
 * @retval 0      If observer was found.
 * @retval EINVAL If observer or resource pointer is NULL.
 * @retval ENOENT If next observer was not found.
 */
u32_t internal_coap_observe_server_next_get(coap_observer_t **observer,
					    coap_observer_t *start,
					    coap_resource_t *resource);

/**@brief Retrieve the observer based on handle.
 *
 * @param[in]  handle   Handle to the coap_observer_t instance.
 * @param[out] observer Pointer to an observer return by reference.
 *                      Should not be NULL.
 *
 * @retval 0      If observer was found in the observer list.
 * @retval EINVAL If observer pointer is NULL.
 * @retval ENOENT If observer associated with the handle was not found.
 */
u32_t internal_coap_observe_server_get(u32_t handle,
				       coap_observer_t **observer);

/**@brief Register a new observable resource.
 *
 * @param[out] handle     Handle to the observable resource instance registered.
 *                        Returned by reference. Should not be NULL.
 * @param[in]  observable Pointer to a observable resource structure to
 *                        register. The structure will be copied.
 *                        Should not be NULL.
 *
 * @retval 0      If the observable resource was registered successfully.
 * @retval ENOMEM If the observable resource could not be added to the list.
 * @retval EINVAL If one of the parameters is a NULL pointer.
 */
u32_t internal_coap_observe_client_register(u32_t *handle,
					    coap_observable_t *observable);

/**@brief Unregister an observable resource.
 *
 * @details Unregister the observable resource and clear the memory used by
 *          this instance.
 *
 * @param[in] handle Handle to the observable resource instance registered.
 *
 * @retval 0      If the observable resource was successfully unregistered.
 * @retval ENOENT If the given handle was not found in the observable resource
 *                list.
 */
u32_t internal_coap_observe_client_unregister(u32_t handle);

/**@brief Search for a observable resource instance by token.
 *
 * @param[out] handle    Handle to the observable resource instance registered.
 *                       Returned by reference. Should not be NULL.
 * @param[in]  token     Pointer to the byte array holding the token id.
 *                       Should not be NULL.
 * @param[in]  token_len Length of the token.
 *
 * @retval 0      If observable resource was found in the observable resource
 *                list.
 * @retval EINVAL If one of the pointers are NULL.
 * @retval ENOENT If observable resource was not found in the observable
 *                resource list.
 */
u32_t internal_coap_observe_client_search(u32_t *handle, u8_t *token,
					  u16_t token_len);

/**@brief Retrieve the observable resource based on handle.
 *
 * @param[in]  handle     Handle to the coap_observable_t instance.
 * @param[out] observable Pointer to an observable resource return by reference.
 *                        Should not be NULL.
 *
 * @retval 0      If observable resource was found in the observable resource
 *                list.
 * @retval EINVAL If observable pointer is NULL.
 * @retval ENOENT If observable resource associated with the handle was not
 *                found.
 */
u32_t internal_coap_observe_client_get(u32_t handle,
				       coap_observable_t **observable);

/**@brief Iterate through observable resources.
 *
 * @param[out] observable Pointer to be filled by the search function upon
 *                        finding the next observable resource starting from
 *                        the pointer provided. Should not be NULL.
 * @param[out] handle     Handler to the observable resource found returned by
 *                        reference. Should not be NULL.
 * @param[in]  observable Pointer to the observable resource where to start the
 *                        search.
 *
 * @retval 0      If observer was found.
 * @retval EINVAL If observer or observer pointer is NULL.
 * @retval ENOENT If next observer was not found.
 */
u32_t internal_coap_observe_client_next_get(coap_observable_t **observable,
					    u32_t *handle,
					    coap_observable_t *start);

#if (COAP_ENABLE_OBSERVE_SERVER == 1) || (COAP_ENABLE_OBSERVE_CLIENT == 1)

/**@brief Internal function to initialize observer (client) and
 *        observable (server) lists.
 *
 * @retval 0 If initialization was successful.
 */
void internal_coap_observe_init(void);

#else /* COAP_ENABLE_OBSERVE_SERVER || COAP_ENABLE_OBSERVE_CLIENT */

#define internal_coap_observe_init(...)

#endif /* COAP_ENABLE_OBSERVE_SERVER || COAP_ENABLE_OBSERVE_CLIENT */

#if (COAP_ENABLE_OBSERVE_CLIENT == 1)

/**@brief Observe client function to be run when sending requests.
 *
 * @details The function will peek into the outgoing messages to see if any
 *          actions regarding subscription to observable resources has to be
 *          done.
 *
 * @param[in] request Pointer to the outgoing request.
 */
void coap_observe_client_send_handle(coap_message_t *request);

/**@brief Observe client function to be run when response message has been
 *        received.
 *
 * @details The function will register and unregister observable resources based
 *          on the received response messages. Upon a notification max-age
 *          values will be updated, and the correct response callback will be
 *          called. If a notification is terminated by the peer, the function
 *          will automatically terminate the subscription from the client by
 *          unregistering the observable resource.
 *
 * @param[in] response Pointer to the response message received.
 * @param[in] item     Pointer to the queued element of the outgoing request.
 */
void coap_observe_client_response_handle(coap_message_t *response,
					 coap_queue_item_t *item);

#else /* COAP_ENABLE_OBSERVE_CLIENT */

#define coap_observe_client_send_handle(...)
#define coap_observe_client_response_handle(...)

#endif /* COAP_ENABLE_OBSERVE_CLIENT */

/**@endcond */

#ifdef __cplusplus
}
#endif

#endif /* COAP_OBSERVE_H__ */

/** @} */
