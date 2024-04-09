/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_CLIENT_NOTIF_H__
#define SSF_CLIENT_NOTIF_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/ssf_errno.h>

/**
 * @brief       SSF notification decode function prototype.
 *              Functions of this type are typically generated from cddl with zcbor.
 */
typedef int (*ssf_notif_decoder)(const uint8_t *payload, size_t payload_len, void *result,
				 size_t *payload_len_out);

/**
 * @brief       Structure to hold an incoming notification.
 *              Decode with @ref ssf_client_notif_decode.
 */
struct ssf_notification {
	/* Notification data start */
	const uint8_t *data;
	/* Length of notification data */
	size_t data_len;
	/*
	 * Start of received packet. For releasing underlying buffer
	 * with ssf_client_transport_decoding_done.
	 */
	const uint8_t *pkt;
	/* Notification packet decoder */
	ssf_notif_decoder notif_decode;
};

/**
 * @brief       SSF notification handler prototype.
 *
 * @param[in]   notif  A pointer to the incoming notification.
 * @param[in]   context  Optional user context.
 *
 * @return      0 on success, otherwise a negative errno.
 */
typedef int (*ssf_notif_handler)(struct ssf_notification *notif, void *context);

/**
 * @brief       SSF notification listener.
 */
struct ssf_client_notif_listener {
	/* Service ID */
	uint16_t id;
	/* Client's version number for the service */
	uint16_t version;
	/* Packet decoder for notification */
	ssf_notif_decoder notif_decode;
	/* Handler for notification */
	ssf_notif_handler handler;
	/* Optional user context */
	void *context;
	/* Is listener registered */
	bool is_registered;
};

/**
 * @brief       Define a notification listener, that is used for invoking the correct handler
 *              function when receiving notifications from the server.
 *              The listener must be registered with @ref ssf_client_notif_register
 *              before notifications can be received.
 *
 * @param       _name  Name of the notification listener.
 * @param[in]   _srvc_name  Short uppercase service name. Must match the service_name variable used
 *                          when specifying the service with the Kconfig.template.service template.
 * @param[in]   _notif_decode  Function of type @ref ssf_notif_decoder. Used when decoding
 *                             notifications.
 * @param[in]   _handler  Function of type @ref ssf_notif_handler. Invoked when receiving a
 *                        notification with the associated service id.
 */
#define SSF_CLIENT_NOTIF_LISTENER_DEFINE(_name, _srvc_name, _notif_decode, _handler)               \
	static int _handler(struct ssf_notification *notif, void *context);                        \
	static struct ssf_client_notif_listener _name = {                                          \
		.id = (CONFIG_SSF_##_srvc_name##_SERVICE_ID),                                      \
		.version = (CONFIG_SSF_##_srvc_name##_SERVICE_VERSION),                            \
		.notif_decode = (ssf_notif_decoder)_notif_decode,                                  \
		.handler = _handler,                                                               \
		.is_registered = false                                                             \
	}

/**
 * @brief       Decode a received notification.
 *
 * @param[in]   notif  A pointer to the incoming notification.
 * @param[out]  decoded_notif  A pointer to a (zcbor) structure, allocated by the caller,
 *                             to be populated by the associated listener's decode function.
 *
 * @return      0 on success,
 *              -SSF_EINVAL if parameters are invalid.
 *              -SSF_EPROTO if decode function fails or is missing.
 */
int ssf_client_notif_decode(const struct ssf_notification *notif, void *decoded_notif);

/**
 * @brief       Must be invoked from @ref ssf_notif_handler to free transport layer buffer when
 *              the request have been decoded and can be freed.
 *
 * @param[in]   notif  A pointer to the incoming notification.
 */
void ssf_client_notif_decode_done(struct ssf_notification *notif);

/**
 * @brief       Register a listener. Enables the listener's handler to be called
 *              when receiving notifications from server with associated service id.
 *
 * @param[in]   listener  The listener to register.
 * @param[in]   context  Optional user context passed to handler function when a notification
 *                        is received.
 *
 * @return      0 on success,
 *              -SSF_EINVAL if parameters are invalid.
 *              -SSF_EALREADY if the listener is already registered.
 *              -SSF_ENOBUFS if max number of listeners have already been registered.
 */
int ssf_client_notif_register(struct ssf_client_notif_listener *listener, void *context);

/**
 * @brief       De-register a listener. Stops the listener's handler from being called
 *              when receiving notifications from server with associated service id.
 *
 * @param[in]   listener  The listener to de-register.
 *
 * @return      0 on success,
 *              -SSF_EINVAL if parameters are invalid.
 *              -SSF_ENXIO if the listener is not already registered.
 */
int ssf_client_notif_deregister(struct ssf_client_notif_listener *listener);

#endif /* SSF_CLIENT_NOTIF_H__ */
