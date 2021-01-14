/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_CLOUD_H_
#define ZEPHYR_INCLUDE_CLOUD_H_

/**
 * @brief Cloud API
 * @defgroup cloud_api Cloud API
 * @{
 */

#include <zephyr.h>

/**@brief Cloud events that can be notified asynchronously by the backend. */
enum cloud_event_type {
	/** The cloud backend is connecting to the configured cloud vendor. */
	CLOUD_EVT_CONNECTING,
	/** The cloud backend is connected to the cloud. */
	CLOUD_EVT_CONNECTED,
	/** The cloud backend got disconnected from the cloud. */
	CLOUD_EVT_DISCONNECTED,
	/** The cloud backend has established the appropriate resources for the
	 *  connection.
	 */
	CLOUD_EVT_READY,
	/** An error has occurred in the cloud backend. */
	CLOUD_EVT_ERROR,
	/** Data has been sent to the cloud. */
	CLOUD_EVT_DATA_SENT,
	/** Data has been received from the cloud. */
	CLOUD_EVT_DATA_RECEIVED,
	/** The device is not yet associated with the user. Pairing requested.
	 */
	CLOUD_EVT_PAIR_REQUEST,
	/** Pairing with the user has been carried out. */
	CLOUD_EVT_PAIR_DONE,
	/** FOTA has started. */
	CLOUD_EVT_FOTA_START,
	/** FOTA has finished. */
	CLOUD_EVT_FOTA_DONE,
	/** The underlying FOTA process has a pending image erase. */
	CLOUD_EVT_FOTA_ERASE_PENDING,
	/** The FOTA image erase has finished. */
	CLOUD_EVT_FOTA_ERASE_DONE,
	/** FOTA download progress event. */
	CLOUD_EVT_FOTA_DL_PROGRESS,

	CLOUD_EVT_COUNT
};

enum cloud_disconnect_reason {
	CLOUD_DISCONNECT_USER_REQUEST,
	/** The connection was closed by the cloud */
	CLOUD_DISCONNECT_CLOSED_BY_REMOTE,
	/** The connection is no longer valid */
	CLOUD_DISCONNECT_INVALID_REQUEST,
	/** Miscellaneous error */
	CLOUD_DISCONNECT_MISC,

	CLOUD_DISCONNECT_COUNT
};

/**@brief Quality of Service for message sent by a cloud backend. */
enum cloud_qos {
	CLOUD_QOS_AT_MOST_ONCE,
	CLOUD_QOS_AT_LEAST_ONCE,
	CLOUD_QOS_EXACTLY_ONCE,

	CLOUD_QOS_COUNT
};

/**@brief Cloud endpoint type. */
enum cloud_endpoint_type {
	/** Endpoint used to send messages to cloud. */
	CLOUD_EP_MSG,
	/** Endpoint used to update the cloud-side device state. */
	CLOUD_EP_STATE,
	/** Endpoint used to delete the cloud-side device state. */
	CLOUD_EP_STATE_DELETE,
	/** Enpoint used to request the cloud-side device state. */
	CLOUD_EP_STATE_GET,
	/** Endpoint used to pair the device with a user. */
	CLOUD_EP_PAIR,

	CLOUD_EP_COMMON_COUNT,
	CLOUD_EP_PRIV_START = CLOUD_EP_COMMON_COUNT,
	CLOUD_EP_PRIV_END = INT16_MAX
};

/**@brief Cloud connect results. */
enum cloud_connect_result {
	CLOUD_CONNECT_RES_SUCCESS = 0,
	/** Cloud backend is not initialized. */
	CLOUD_CONNECT_RES_ERR_NOT_INITD = -1,
	/** Invalid parameters in cloud connection request. */
	CLOUD_CONNECT_RES_ERR_INVALID_PARAM = -2,
	/** Host cannot be found with the available network interfaces. */
	CLOUD_CONNECT_RES_ERR_NETWORK = -3,
	/** A backend-specific error. */
	CLOUD_CONNECT_RES_ERR_BACKEND = -4,
	/** Error cause cannot be determined.*/
	CLOUD_CONNECT_RES_ERR_MISC = -5,
	/** MQTT RX/TX buffers were not initialized. */
	CLOUD_CONNECT_RES_ERR_NO_MEM = -6,
	/** Invalid private key */
	CLOUD_CONNECT_RES_ERR_PRV_KEY = -7,
	/** Invalid CA or client certificate */
	CLOUD_CONNECT_RES_ERR_CERT = -8,
	/** Miscellaneous certificate error */
	CLOUD_CONNECT_RES_ERR_CERT_MISC = -9,
	/** Timeout; typically occurs when the inserted SIM card has no data */
	CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA = -10,
	/** Connection process has already been started. */
	CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED = -11,
};

/** @brief Forward declaration of cloud backend type. */
struct cloud_backend;

/** @brief Cloud endpoint data. */
struct cloud_endpoint {
	enum cloud_endpoint_type type;
	char *str;
	size_t len;
};

/**@brief Cloud message type. */
struct cloud_msg {
	char *buf;
	size_t len;
	enum cloud_qos qos;
	struct cloud_endpoint endpoint;
};

/**@brief Cloud event type. */
struct cloud_event {
	enum cloud_event_type type;
	union {
		struct cloud_msg msg;
		int err;
		/** FOTA progress in percentage. */
		int fota_progress;
		bool persistent_session;
	} data;
};

/**
 * @brief Cloud event handler function type.
 *
 * @param backend   Pointer to cloud backend.
 * @param evt       Pointer to cloud event.
 * @param user_data Pointer to user defined data that will be passed on as
 * 		       argument to cloud event handler.
 */
typedef void (*cloud_evt_handler_t)(const struct cloud_backend *const backend,
				    const struct cloud_event *const evt,
				    void *user_data);

/**
 * @brief Cloud backend API.
 *
 * ping() and user_data_set() can be omitted, the other functions are mandatory.
 */
struct cloud_api {
	int (*init)(const struct cloud_backend *const backend,
		    cloud_evt_handler_t handler);
	int (*uninit)(const struct cloud_backend *const backend);
	int (*connect)(const struct cloud_backend *const backend);
	int (*disconnect)(const struct cloud_backend *const backend);
	int (*send)(const struct cloud_backend *const backend,
		    const struct cloud_msg *const msg);
	int (*ping)(const struct cloud_backend *const backend);
	int (*keepalive_time_left)(const struct cloud_backend *const backend);
	int (*input)(const struct cloud_backend *const backend);
	int (*ep_subscriptions_add)(const struct cloud_backend *const backend,
				    const struct cloud_endpoint *const list,
				    size_t list_count);
	int (*ep_subscriptions_remove)(
				const struct cloud_backend *const backend,
				const struct cloud_endpoint *const list,
				size_t list_count);
	int (*user_data_set)(const struct cloud_backend *const backend,
			     void *user_data);
};

/**@brief Structure for cloud backend configuration. */
struct cloud_backend_config {
	char *name;
	cloud_evt_handler_t handler;
	int socket;
	void *user_data;
	char *id;
	size_t id_len;
};

/**@brief Structure for cloud backend. */
struct cloud_backend {
	const struct cloud_api *const api;
	struct cloud_backend_config *const config;
};

/**@brief Initialize a cloud backend. Performs all necessary
 *	  configuration of the backend required to connect its online
 *	  counterpart.
 *
 * @param backend Pointer to cloud backend structure.
 * @param handler Handler to receive events from the backend.
 */
static inline int cloud_init(struct cloud_backend *const backend,
							 cloud_evt_handler_t handler)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->init == NULL) {
		return -ENOTSUP;
	}

	if (handler == NULL) {
		return -EINVAL;
	}

	return backend->api->init(backend, handler);
}

/**@brief Uninitialize a cloud backend. Gracefully disconnects
 *        remote endpoint and releases memory.
 *
 * @param backend Pointer to cloud backend structure.
 */
static inline int cloud_uninit(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->uninit == NULL) {
		return -ENOTSUP;
	}

	return backend->api->uninit(backend);
}

/**@brief Request connection to a cloud backend.
 *
 * @details The backend is required to expose the socket in use when this
 *	    function returns. The socket should then be available through
 *	    backend->config->socket and the application may start listening
 *	    for events on it.
 *
 * @param backend Pointer to a cloud backend structure.
 *
 * @return connect result defined by enum cloud_connect_result.
 */
static inline int cloud_connect(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->connect == NULL) {
		return CLOUD_CONNECT_RES_ERR_INVALID_PARAM;
	}

	return backend->api->connect(backend);
}

/**@brief Disconnect from a cloud backend.
 *
 * @param backend Pointer to a cloud backend structure.
 *
 * @return 0 or a negative error code indicating reason of failure.
 */
static inline int cloud_disconnect(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->disconnect == NULL) {
		return -ENOTSUP;
	}

	return backend->api->disconnect(backend);
}

/**@brief Send data to a cloud.
 *
 * @param backend Pointer to a cloud backend structure.
 * @param msg     Pointer to cloud message structure.
 *
 * @return 0 or a negative error code indicating reason of failure.
 */
static inline int cloud_send(const struct cloud_backend *const backend,
			     struct cloud_msg *msg)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->send == NULL) {
		return -ENOTSUP;
	}

	return backend->api->send(backend, msg);
}

/**
 * @brief Optional API to ping the cloud's remote endpoint periodically.
 *
 * @param backend Pointer to cloud backend.
 *
 * @return 0 or a negative error code indicating reason of failure.
 */
static inline int cloud_ping(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL) {
		return -ENOTSUP;
	}

	/* Ping will only be sent if the backend has implemented it. */
	if (backend->api->ping != NULL) {
		return backend->api->ping(backend);
	}

	return 0;
}

/**
 * @brief Helper function to determine when next keep alive message should be
 *        sent. Can be used for instance as a source for `poll` timeout.
 *
 * @param backend Pointer to cloud backend.
 *
 * @return Time in milliseconds until next keep alive message is expected to
 *         be sent.
 * @return -1 if keep alive messages are not enabled.
 */
static inline int cloud_keepalive_time_left(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->keepalive_time_left == NULL) {
		__ASSERT(0, "Missing cloud backend functionality");
		return SYS_FOREVER_MS;
	}

	return backend->api->keepalive_time_left(backend);
}

/**
 * @brief Process incoming data to backend.
 *
 * @note This is a non-blocking call.
 *
 * @param backend Pointer to cloud backend.
 *
 * @return 0 or a negative error code indicating reason of failure.
 */
static inline int cloud_input(const struct cloud_backend *const backend)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->input == NULL) {
		return -ENOTSUP;
	}

	return backend->api->input(backend);
}

/**@brief Add Cloud endpoint subscriptions.
 *
 * @param backend Pointer to cloud backend structure.
 * @param list Pointer to a list of endpoint subscriptions.
 * @param list_count Number of entries in the endpoint subscription list.
 */
static inline int cloud_ep_subscriptions_add(
			const struct cloud_backend *const backend,
			const struct cloud_endpoint *list, size_t list_count)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->ep_subscriptions_add == NULL) {
		return -ENOTSUP;
	}

	return backend->api->ep_subscriptions_add(backend, list, list_count);
}

/**@brief Remove Cloud endpoint subscriptions.
 *
 * @param backend Pointer to cloud backend structure.
 * @param list Pointer to a list of endpoint subscriptions.
 * @param list_count Number of entries in the endpoint subscriptions list.
 */
static inline int cloud_ep_subscriptions_remove(
			const struct cloud_backend *const backend,
			const struct cloud_endpoint *list, size_t list_count)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->ep_subscriptions_remove == NULL) {
		return -ENOTSUP;
	}

	return backend->api->ep_subscriptions_remove(backend, list, list_count);
}

/**@brief Set the user-defined data that is passed as an argument to cloud event
 * 	  handler.
 *
 * @param backend   Pointer to cloud backend structure.
 * @param user_data Pointer to user defined data that will be passed on as
 * 		       argument to cloud event handler.
 */
static inline int cloud_user_data_set(struct cloud_backend *const backend,
				      void *user_data)
{
	if (backend == NULL || backend->api == NULL ||
	    backend->api->user_data_set == NULL) {
		return -ENOTSUP;
	}

	if (user_data == NULL) {
		return -EINVAL;
	}

	return backend->api->user_data_set(backend, user_data);
}

/**
 * @brief Calls the user-provided event handler with event data.
 *
 * @param backend	Pointer to cloud backend.
 * @param evt		Pointer to event data.
 * @param user_data	Pointer to user-defined data.
 */
static inline void cloud_notify_event(struct cloud_backend *backend,
				      struct cloud_event *evt,
				      void *user_data)
{
	if (backend->config->handler) {
		backend->config->handler(backend, evt, user_data);
	}
}

/**
 * @brief Get binding (pointer) to cloud backend if a registered backend
 *	      matches the provided name.
 *
 * @param name String with the name of the cloud backend.
 *
 * @return Pointer to the cloud backend structure.
 **/
struct cloud_backend *cloud_get_binding(const char *name);

#define CLOUD_BACKEND_DEFINE(_name, _api)			               \
									       \
	static struct cloud_backend_config UTIL_CAT(_name, _config) =	       \
	{								       \
		.name = STRINGIFY(_name)				       \
	};								       \
									       \
	static const struct cloud_backend _name				       \
	__attribute__ ((section(".cloud_backends"))) __attribute__((used)) =   \
	{								       \
		.api = &_api,						       \
		.config = &UTIL_CAT(_name, _config)			       \
	};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CLOUD_H_ */
