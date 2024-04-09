/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/reset_evt_service.h>

#include "reset_evt_service_decode.h"
#include "reset_evt_service_encode.h"
#include "reset_evt_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"

SSF_CLIENT_LOG_REGISTER(reset_evt_service, CONFIG_SSF_RESET_EVT_SERVICE_LOG_LEVEL);

SSF_CLIENT_SERVICE_DEFINE(reset_evt_srvc, RESET_EVT, cbor_encode_reset_evt_sub_req,
			  cbor_decode_reset_evt_sub_rsp);

SSF_CLIENT_NOTIF_LISTENER_DEFINE(reset_evt_listener, RESET_EVT, cbor_decode_reset_evt_notif,
				 reset_evt_handler);

static ssf_reset_evt_callback user_callback;

static int reset_evt_handler(struct ssf_notification *notif, void *handler_user_data)
{
	int err;
	struct reset_evt_notif notification;

	if (user_callback == NULL) {
		SSF_CLIENT_LOG_WRN("No user callback set");
		return -SSF_EFAULT;
	}

	err = ssf_client_notif_decode(notif, &notification);
	ssf_client_notif_decode_done(notif);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Decoding notification failed: %d", err);
		return err;
	}

	err = user_callback(notification.reset_evt_notif_domains,
			    notification.reset_evt_notif_delay_ms, handler_user_data);

	return err;
}

int ssf_reset_evt_subscribe(ssf_reset_evt_callback callback, void *context)
{
	int err;
	bool req_subscribe;
	int32_t rsp_status;

	if (user_callback != NULL || callback == NULL) {
		return -SSF_EINVAL;
	}

	err = ssf_client_notif_register(&reset_evt_listener, context);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to register for notifications: %d", err);
		return err;
	}

	req_subscribe = true;
	err = ssf_client_send_request(&reset_evt_srvc, &req_subscribe, &rsp_status, NULL);
	if (err != 0) {
		return err;
	}

	if (rsp_status != 0) {
		SSF_CLIENT_LOG_ERR("Failed to subscribe to server notifications: %d", err);
		err = ssf_client_notif_deregister(&reset_evt_listener);
		if (err != 0) {
			SSF_CLIENT_LOG_ERR("Failed to deregister from notifications: %d", err);
		}

		return -SSF_EFAULT;
	}

	user_callback = callback;

	return err;
}

int ssf_reset_evt_unsubscribe(void)
{
	int err;
	bool req_subscribe;
	int32_t rsp_status;

	req_subscribe = false;

	err = ssf_client_send_request(&reset_evt_srvc, &req_subscribe, &rsp_status, NULL);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to issue unsubscribe message to server: %d", err);
		return err;
	}

	if (rsp_status != 0) {
		SSF_CLIENT_LOG_ERR("Server returned error when unsubscribing: %d", err);
		return -SSF_EFAULT;
	}

	err = ssf_client_notif_deregister(&reset_evt_listener);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to de-register from notifications: %d", err);
		return err;
	}

	user_callback = NULL;

	return err;
}
