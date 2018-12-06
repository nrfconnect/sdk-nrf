/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_observe);

#include <string.h>
#include <errno.h>

#include <net/coap_observe_api.h>

#include "coap.h"
#include "coap_observe.h"

#if (COAP_ENABLE_OBSERVE_SERVER == 1)

typedef struct {
	coap_observer_t observer;
	struct sockaddr_in6 remote; /* Provision for maximum size. */
} internal_coap_observer_t;

static internal_coap_observer_t observers[COAP_OBSERVE_MAX_NUM_OBSERVERS];

static void observe_server_init(void)
{
	COAP_ENTRY();
	memset(observers, 0, sizeof(internal_coap_observer_t) *
	       COAP_OBSERVE_MAX_NUM_OBSERVERS);
	COAP_EXIT();
}

u32_t internal_coap_observe_server_register(u32_t *handle,
					    coap_observer_t *observer)
{
	NULL_PARAM_CHECK(handle);
	NULL_PARAM_CHECK(observer);
	NULL_PARAM_MEMBER_CHECK(observer->remote);
	NULL_PARAM_MEMBER_CHECK(observer->resource_of_interest);

	if ((observer->remote->sa_family != AF_INET) &&
	    (observer->remote->sa_family != AF_INET6)) {
		return EINVAL;
	}

	COAP_ENTRY();

	/* Check if there is already a registered observer in the list to be
	 * reused.
	 */
	u32_t i;
	u32_t err_code = coap_observe_server_search(
			&i, observer->remote, observer->resource_of_interest);


	if (err_code == 0) {
		memcpy(&observers[i].observer, observer,
		       sizeof(coap_observer_t));
		observers[i].observer.remote =
					(struct sockaddr *)&observers[i].remote;
		*handle = i;

		COAP_EXIT();
		return 0;
	}

	/* Check if there is an available spot in the observer list. */
	for (i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVERS; i++) {
		if (observers[i].observer.resource_of_interest == NULL) {
			memcpy(&(observers[i].observer), observer,
			       sizeof(coap_observer_t));

			if (observer->remote->sa_family == AF_INET6) {
				memcpy(&(observers[i].remote), observer->remote,
				       sizeof(struct sockaddr_in6));
			} else {
				memcpy(&(observers[i].remote), observer->remote,
				       sizeof(struct sockaddr_in));
			}
			observers[i].observer.remote =
					(struct sockaddr *)&observers[i].remote;
			*handle = i;

			COAP_EXIT();
			return 0;
		}
	}

	COAP_EXIT();
	return ENOMEM;
}


u32_t internal_coap_observe_server_unregister(u32_t handle)
{
	u32_t ret = 0;

	COAP_ENTRY();

	if (handle >= COAP_OBSERVE_MAX_NUM_OBSERVERS) {
		/* Out of bounds. */
		ret = ENOENT;
	} else if (observers[handle].observer.resource_of_interest == NULL) {
		/* Not registered. */
		ret = ENOENT;
	} else {
		/* Unregister successfully. */
		observers[handle].observer.resource_of_interest = NULL;
	}

	COAP_EXIT();
	return ret;
}


u32_t internal_coap_observe_server_search(u32_t *handle,
					  struct sockaddr *observer_addr,
					  coap_resource_t *resource)
{
	NULL_PARAM_CHECK(handle);
	NULL_PARAM_CHECK(observer_addr);
	NULL_PARAM_CHECK(resource);

	for (u32_t i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVERS; i++) {
		if (observers[i].observer.resource_of_interest == resource) {
			const struct sockaddr *remote =
				(struct sockaddr *)&observers[i].remote;
			const struct sockaddr_in6 *remote6 =
				(struct sockaddr_in6 *)&observers[i].remote;
			const struct sockaddr_in *remote4 =
				(struct sockaddr_in *)&observers[i].remote;

			const struct sockaddr_in6 *observer_addr6 =
					(struct sockaddr_in6 *)observer_addr;
			const struct sockaddr_in *observer_addr4 =
					(struct sockaddr_in *)observer_addr;

			if ((remote->sa_family         == AF_INET6) &&
			    (observer_addr->sa_family  == AF_INET6) &&
			    (observer_addr6->sin6_port == remote6->sin6_port)) {
				if (memcmp(observer_addr6->sin6_addr.s6_addr,
					   remote6->sin6_addr.s6_addr,
					   sizeof(struct in6_addr)) == 0) {
					*handle = i;
					return 0;
				}
			}

			if ((remote->sa_family        == AF_INET) &&
			    (observer_addr->sa_family == AF_INET) &&
			    (observer_addr4->sin_port == remote4->sin_port)) {
				if (memcmp(&observer_addr4->sin_addr,
					   &remote4->sin_addr,
					   sizeof(struct in_addr)) == 0) {
					*handle = i;
					return 0;
				}
			}
		}
	}

	return ENOENT;
}


u32_t internal_coap_observe_server_next_get(coap_observer_t **observer,
					    coap_observer_t *start,
					    coap_resource_t *resource)
{
	NULL_PARAM_CHECK(resource);
	NULL_PARAM_CHECK(observer);

	if (start == NULL) {
		for (u32_t i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVERS; i++) {
			if (observers[i].observer.resource_of_interest ==
								resource) {
				(*observer) = &observers[i].observer;
				return 0;
			}
		}
	} else {
		u32_t index_to_previous =
				(u8_t)(((u32_t)start - (u32_t)observers) /
				(u32_t)sizeof(internal_coap_observer_t));

		for (u32_t i = index_to_previous + 1;
		     i < COAP_OBSERVE_MAX_NUM_OBSERVERS; i++) {
			if (observers[i].observer.resource_of_interest ==
								resource) {
				(*observer) = &observers[i].observer;
				return 0;
			}
		}
	}

	(*observer) = NULL;

	return ENOENT;
}

u32_t internal_coap_observe_server_get(u32_t handle, coap_observer_t **observer)
{
	NULL_PARAM_CHECK(observer);

	if (handle >= COAP_OBSERVE_MAX_NUM_OBSERVERS) {
		return ENOENT;
	}

	if (observers[handle].observer.resource_of_interest == NULL) {
		return ENOENT;
	}

	*observer = &observers[handle].observer;
	return 0;
}
#else
#define observe_server_init(...)
#endif

#if (COAP_ENABLE_OBSERVE_CLIENT == 1)

typedef struct {
	coap_observable_t observable;
	struct sockaddr_in6 remote; /* Provision for the largest size. */
} internal_coap_observables_t;

static internal_coap_observables_t
				observables[COAP_OBSERVE_MAX_NUM_OBSERVABLES];

static void observe_client_init(void)
{
	COAP_ENTRY();
	memset(observables, 0, sizeof(internal_coap_observables_t) *
	       COAP_OBSERVE_MAX_NUM_OBSERVABLES);
	COAP_EXIT();
}


u32_t internal_coap_observe_client_register(u32_t *handle,
					    coap_observable_t *observable)
{
	NULL_PARAM_CHECK(handle);
	NULL_PARAM_CHECK(observable);
	NULL_PARAM_MEMBER_CHECK(observable->remote);
	NULL_PARAM_MEMBER_CHECK(observable->response_callback);

	if ((observable->remote->sa_family != AF_INET)  &&
	    (observable->remote->sa_family != AF_INET6)) {
		return EINVAL;
	}

	COAP_ENTRY();

	/* Check if there is an available spot in the observer list. */
	for (u32_t i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVABLES; i++) {
		if (observables[i].observable.response_callback == NULL) {
			memcpy(&(observables[i].observable), observable,
			       sizeof(coap_observable_t));

			if (observable->remote->sa_family == AF_INET6) {
				memcpy(&(observables[i].remote),
				       observable->remote,
				       sizeof(struct sockaddr_in6));
			} else {
				memcpy(&(observables[i].remote),
				       observable->remote,
				       sizeof(struct sockaddr_in));
			}
			observables[i].observable.remote =
				(struct sockaddr *)&observables[i].remote;
			*handle = i;

			COAP_EXIT();
			return 0;
		}
	}

	COAP_EXIT();
	return ENOMEM;
}


u32_t internal_coap_observe_client_unregister(u32_t handle)
{
	u32_t ret = 0;

	COAP_ENTRY();

	if (handle >= COAP_OBSERVE_MAX_NUM_OBSERVABLES) {
		/* Out of bounds. */
		ret = ENOENT;
	} else if (observables[handle].observable.response_callback == NULL) {
		/* Not registered. */
		ret = ENOENT;
	} else {
		/* Unregister successfully. */
		observables[handle].observable.response_callback = NULL;
	}

	COAP_EXIT();
	return ret;
}


u32_t internal_coap_observe_client_search(u32_t *handle, u8_t *token,
					  u16_t token_len)
{
	NULL_PARAM_CHECK(handle);
	NULL_PARAM_CHECK(token);

	for (u32_t i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVABLES; i++) {
		if ((observables[i].observable.response_callback != NULL) &&
		    (observables[i].observable.token_len != 0) &&
		    (memcmp(observables[i].observable.token, token,
			    observables[i].observable.token_len) == 0)) {
			*handle = i;
			return 0;
		}
	}

	return ENOENT;
}


u32_t internal_coap_observe_client_get(u32_t handle,
				       coap_observable_t **observable)
{
	NULL_PARAM_CHECK(observable);

	if (handle >= COAP_OBSERVE_MAX_NUM_OBSERVABLES) {
		return ENOENT;
	}

	if (observables[handle].observable.response_callback == NULL) {
		return ENOENT;
	}

	*observable = &observables[handle].observable;

	return 0;
}

u32_t internal_coap_observe_client_next_get(coap_observable_t **observable,
					    u32_t *handle,
					    coap_observable_t *start)
{
	NULL_PARAM_CHECK(observable);

	if (start == NULL) {
		for (u32_t i = 0; i < COAP_OBSERVE_MAX_NUM_OBSERVABLES; i++) {
			if (observables[i].observable.response_callback !=
									NULL) {
				(*observable) = &observables[i].observable;
				(*handle) = i;
				return 0;
			}
		}
	} else {
		u32_t index_to_previous =
			(u8_t)(((u32_t)start - (u32_t)observables) /
			(u32_t)sizeof(internal_coap_observables_t));

		for (u32_t i = index_to_previous + 1;
		     i < COAP_OBSERVE_MAX_NUM_OBSERVABLES; i++) {
			if (observables[i].observable.response_callback !=
									NULL) {
				(*observable) = &observables[i].observable;
				(*handle) = i;
				return 0;
			}
		}
	}

	(*observable) = NULL;

	COAP_MUTEX_UNLOCK();

	return ENOENT;
}

static u32_t observe_opt_present(coap_message_t *message)
{
	u8_t index;

	for (index = 0; index < message->options_count; index++) {
		if (message->options[index].number == COAP_OPT_OBSERVE) {
			return 0;
		}
	}
	return ENOENT;
}

static void set_max_age(coap_observable_t *observable, coap_message_t *response)
{
	u8_t index;

	for (index = 0; index < response->options_count; index++) {
		if (response->options[index].number == COAP_OPT_MAX_AGE) {
			u32_t max_age;

			observable->max_age = coap_opt_u_decode(
				&max_age, response->options[index].length,
				response->options[index].data);
			observable->max_age = max_age;
			return;
		}
	}

	/* Max-Age option is not present, set default value to 60. */
	observable->max_age = 60;
}

void coap_observe_client_send_handle(coap_message_t *request)
{
	COAP_ENTRY();

	if (request->header.code == COAP_CODE_GET) {
		u32_t observe_option = 0;

		if (observe_opt_present(request) == 0) {
			/* Locate option and check value. */
			u8_t index;

			for (index = 0; index < request->options_count;
			     index++) {
				if (request->options[index].number ==
							COAP_OPT_OBSERVE) {
					u32_t err_code = coap_opt_u_decode(
						&observe_option,
						request->options[index].length,
						request->options[index].data);
					if (err_code != 0) {
						return;
					}
					break;
				}
			}
		}

		if (observe_option == 1) {
			/* Un-register observable instance. */
			u32_t handle;
			u32_t err_code = internal_coap_observe_client_search(
						&handle, request->token,
						request->header.token_len);

			if (err_code == 0) {
				(void)internal_coap_observe_client_unregister(
									handle);
				COAP_TRC("OBSERVE=1 in request, "
					 "client_unregister handle: %i",
					 handle);
			}
		}
	}

	COAP_EXIT();
}

void coap_observe_client_response_handle(coap_message_t *response,
					 coap_queue_item_t *item)
{
	COAP_ENTRY();

	if (observe_opt_present(response) == 0) {
		if (item == NULL) {
			/* Search for the token in the observable list. */
			u32_t handle;
			u32_t err_code = internal_coap_observe_client_search(
					&handle, response->token,
					response->header.token_len);

			if (err_code == 0) {
				/* Fetch the observable. */
				coap_observable_t *observable;

				err_code = internal_coap_observe_client_get(
							handle, &observable);
				if (err_code == 0) {
					/* Update max-age to the newly received
					 * message.
					 */
					set_max_age(observable, response);

					COAP_MUTEX_UNLOCK();

					/* Callback to the application. */
					observable->response_callback(
							0, NULL, response);

					COAP_MUTEX_LOCK();

					COAP_TRC("Notification received on "
						 "handle: %i", handle);

#ifdef COAP_AUTOMODE
					if (response->header.type ==
								COAP_TYPE_CON) {
						/* Reply an ACK upon CON
						 * message.
						 */
					} else if (response->header.type ==
								COAP_TYPE_RST) {
						/* Remove observable from list.
						 */
					}
#endif
				} else {
#ifdef COAP_AUTOMODE
					if (response->header.type ==
								COAP_TYPE_CON) {
						/* Reply reset upon CON message
						 * when observer is not located.
						 */
					}
#endif
				}
			} else {
				/* Send RST message back to server to indicate
				 * there is no one listening.
				 */
			}
		} else { /* item set. */
			/* If there is no observable instance created yet for
			 * this token, add it.
			 */
			u32_t handle;
			u32_t err_code = internal_coap_observe_client_search(
					&handle, response->token,
					response->header.token_len);

			if (err_code == ENOENT) {
				/* If the response is a valid response, add the
				 * observable resource.
				 */
				if (response->header.code ==
							COAP_CODE_205_CONTENT) {
					coap_observable_t observable;

					/* Token Length. */
					observable.token_len =
						response->header.token_len;
					/* Remote. */
					observable.remote = response->remote;

					/* Token. */
					memcpy(observable.token,
					       response->token,
					       observable.token_len);
					/* Callback to be called upon
					 * notification.
					 */
					observable.response_callback =
								item->callback;

					/* Update max-age to the newly received
					 * message.
					 */
					set_max_age(&observable, response);

					/* Register the observable. */
					u32_t observable_resource_handle;

					internal_coap_observe_client_register(
						&observable_resource_handle,
						&observable);
					/* TODO: error check */

					COAP_TRC("Subscription response "
						 "received, client_register "
						 "handle: %i",
						 observable_resource_handle);
				}
			}
		}
	} else {  /* COAP_OPT_OBSERVE not present */
		u32_t handle;
		u32_t err_code = internal_coap_observe_client_search(
						&handle, response->token,
						response->header.token_len);

		if (err_code == 0) {
			(void)internal_coap_observe_client_unregister(handle);
			COAP_TRC("OBSERVE not present in notification, "
				 "client_unregister handle: %i", handle);
		}
	}

	COAP_EXIT();
}
#else
#define observe_client_init(...)
#endif

#if (COAP_ENABLE_OBSERVE_SERVER == 1) || (COAP_ENABLE_OBSERVE_CLIENT == 1)
void internal_coap_observe_init(void)
{
	observe_server_init();
	observe_client_init();
}
#endif

#if (COAP_ENABLE_OBSERVE_SERVER == 1)

u32_t coap_observe_server_register(u32_t *handle, coap_observer_t *observer)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_server_register(handle,
							       observer);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_server_unregister(u32_t handle)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_server_unregister(handle);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_server_search(u32_t *handle, struct sockaddr *observer_addr,
				 coap_resource_t *resource)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_server_search(
					handle, observer_addr, resource);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_server_next_get(coap_observer_t **observer,
				   coap_observer_t *start,
				   coap_resource_t *resource)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_server_next_get(observer,
							       start, resource);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_server_get(u32_t handle, coap_observer_t **observer)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_server_get(handle, observer);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

#endif /* COAP_ENABLE_OBSERVE_SERVER = 1 */

#if (COAP_ENABLE_OBSERVE_CLIENT == 1)

u32_t coap_observe_client_register(u32_t *handle, coap_observable_t *observable)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_client_register(handle,
							       observable);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_client_unregister(u32_t handle)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_client_unregister(handle);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_client_search(u32_t *handle, u8_t *token, u16_t token_len)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_client_search(handle, token,
							     token_len);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_client_get(u32_t handle, coap_observable_t **observable)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_client_get(handle, observable);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_observe_client_next_get(coap_observable_t **observable,
				   u32_t *handle,
				   coap_observable_t *start)
{
	COAP_MUTEX_UNLOCK();

	u32_t err_code = internal_coap_observe_client_next_get(observable,
							       handle, start);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

#endif /* COAP_ENABLE_OBSERVE_CLIENT == 1 */
