/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/suit_service.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include "suit_service_decode.h"
#include "suit_service_encode.h"
#include "suit_service_types.h"
#include "suit_service_utils.h"

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_srvc, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

/* Define the SSF service client as global variable, so it is possible to implement service using
 * multiple files.
 */
#define SSF_CLIENT_SERVICE_DEFINE_GLOBAL(_name, _srvc_name, _req_encode, _rsp_decode)              \
	const struct ssf_client_srvc _name = {                                                     \
		.id = (CONFIG_SSF_##_srvc_name##_SERVICE_ID),                                      \
		.version = (CONFIG_SSF_##_srvc_name##_SERVICE_VERSION),                            \
		.req_encode = (request_encoder)_req_encode,                                        \
		.rsp_decode = (response_decoder)_rsp_decode,                                       \
		.req_buf_size = (CONFIG_SSF_##_srvc_name##_SERVICE_BUFFER_SIZE)                    \
	}

SSF_CLIENT_SERVICE_DEFINE_GLOBAL(suit_srvc, SUIT, cbor_encode_suit_req, cbor_decode_suit_rsp);

#if defined(CONFIG_SUIT_STREAM_IPC_PROVIDER)

#include <suit_ipc_streamer.h>

SSF_CLIENT_NOTIF_LISTENER_DEFINE(suit_evt_listener, SUIT, cbor_decode_suit_nfy, notify_evt_handler);

/**
 * @brief	Function pointer and private data (context) registered by streamer provider.
 *		Utilized to inform about image chunk status change.
 *
 * @note	Streamer requestor notifies its peer that the status of some of image chunks
 *		has been changed. This happens when image chunk is processed by streamer requestor.
 *		Once such notification is received by streamer requestor proxy (this code)
 *		it can be populated (called back) to streamer provider, using registered function
 *		pointer. See suit_ipc_streamer_chunk_status_evt_subscribe.
 *		Only one streamer provider per local domain can be registered at time.
 */
static suit_ipc_streamer_chunk_status_notify_fn chunk_status_notify_fn;
static void *chunk_status_notify_context;

/**
 * @brief	Function pointer and private data (context) registered by streamer provider.
 *		Utilized to inform that the image is missing by the streamer requestor.
 *
 * @note	Streamer requestor notifies its peer about pending request for the image.
 *		Once such notification is received by streamer requestor proxy (this code)
 *		it can be populated (called back) to streamer provider, using registered function
 *		pointer. See suit_ipc_streamer_missing_image_evt_subscribe.
 *		Only one streamer provider per local domain can be registered at time.
 */
static suit_ipc_streamer_missing_image_notify_fn missing_image_notify_fn;
static void *missing_image_notify_context;

/**
 * @brief	Initializes notification channel between server (executed in Secure Domain)
 *		and client (this code, local domain)
 *
 * @note	This function shall be executed once, during the system initialization,
 *		in case if there is a need to support server-sent notifications.
 */
static int evt_listener_init(void)
{
	int err;
	void *context = NULL;

	err = ssf_client_notif_register(&suit_evt_listener, context);
	if (err != 0) {
		LOG_ERR("Failed to register for notifications: %d", err);
		return err;
	}

	struct suit_req req = { 0 };
	struct suit_rsp rsp = { 0 };
	struct suit_evt_sub_req *req_data = { 0 };
	struct suit_evt_sub_rsp *rsp_data = { 0 };

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(evt_sub);
	req_data = &req.SSF_SUIT_REQ(evt_sub);
	req_data->SSF_SUIT_REQ_ARG(evt_sub, subscribe) = true;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send request: %d", err);

		int dreg_err = ssf_client_notif_deregister(&suit_evt_listener);

		if (dreg_err) {
			LOG_ERR("Failed to deregister from notifications: %d", dreg_err);
		}

		return err;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(evt_sub);
	err = rsp_data->SSF_SUIT_RSP_ARG(evt_sub, ret);
	if (err != 0) {
		LOG_ERR("Failed to subscribe: %d", err);

		int dreg_err = ssf_client_notif_deregister(&suit_evt_listener);

		if (dreg_err) {
			LOG_ERR("Failed to deregister from notifications: %d", dreg_err);
		}

		return err;
	}

	LOG_INF("%s success", __func__);
	return 0;
}

/**
 * @brief	Passes notification about missing image from event handler to
 *		registered client (streamer provider).
 */
static int handle_missing_image_notify_fn(const struct suit_missing_image_evt_nfy *nfy_data)
{
	int err = 0;
	suit_ipc_streamer_missing_image_notify_fn notify_fn = missing_image_notify_fn;

	if (notify_fn != NULL) {
		void *context = missing_image_notify_context;
		const uint8_t *resource_id = nfy_data->suit_missing_image_evt_nfy_resource_id.value;
		size_t resource_id_length = nfy_data->suit_missing_image_evt_nfy_resource_id.len;
		uint32_t stream_session_id = nfy_data->suit_missing_image_evt_nfy_stream_session_id;

		err = notify_fn(resource_id, resource_id_length, stream_session_id, context);
	}

	return err;
}

/**
 * @brief	Passes notification about chunk status update from event handler to
 *		registered client (streamer provider).
 */
static int handle_chunk_status_notify_fn(const struct suit_chunk_status_evt_nfy *nfy_data)
{
	int err = 0;
	suit_ipc_streamer_chunk_status_notify_fn notify_fn = chunk_status_notify_fn;

	if (notify_fn != NULL) {
		void *context = chunk_status_notify_context;
		uint32_t stream_session_id = nfy_data->suit_chunk_status_evt_nfy_stream_session_id;

		err = notify_fn(stream_session_id, context);
	}

	return err;
}

/**
 * @brief	Decodes notification message and calls respective
 *		notification handler
 */
static int notify_evt_handler(struct ssf_notification *notif, void *handler_user_data)
{
	ARG_UNUSED(handler_user_data);

	struct suit_nfy notification;

	int err = ssf_client_notif_decode(notif, &notification);

	if (err != 0) {
		LOG_ERR("Decoding notification failed: %d", err);
		return err;
	}

	switch (notification.suit_nfy_msg_choice) {
	case suit_nfy_msg_suit_missing_image_evt_nfy_m_c:
		LOG_INF("Missing image notification");
		struct suit_missing_image_evt_nfy *missing_image_evt_nfy_data =
			&notification.suit_nfy_msg_suit_missing_image_evt_nfy_m;

		err = handle_missing_image_notify_fn(missing_image_evt_nfy_data);
		break;

	case suit_nfy_msg_suit_chunk_status_evt_nfy_m_c:
		LOG_INF("Chunk status update notification");
		struct suit_chunk_status_evt_nfy *chunk_status_evt_nfy_data =
			&notification.suit_nfy_msg_suit_chunk_status_evt_nfy_m;

		err = handle_chunk_status_notify_fn(chunk_status_evt_nfy_data);
		break;

	default:
		break;
	}
	ssf_client_notif_decode_done(notif);
	return err;
}

suit_plat_err_t suit_ipc_streamer_chunk_enqueue(uint32_t stream_session_id, uint32_t chunk_id,
						uint32_t offset, uint8_t *address, uint32_t size,
						bool last_chunk)
{
	int err;
	struct suit_req req = { 0 };
	struct suit_rsp rsp = { 0 };
	struct suit_chunk_enqueue_req *req_data = { 0 };
	struct suit_chunk_enqueue_rsp *rsp_data = { 0 };

	LOG_INF("Chunk enqueue, stream_session_id: %d, chunk_id: %d, offset: %d,"
		" address: 0x%08X, size: %d, last_chunk: %d",
		stream_session_id, chunk_id, offset, (uint32_t)address, size, last_chunk);

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(chunk_enqueue);
	req_data = &req.SSF_SUIT_REQ(chunk_enqueue);

	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, stream_session_id) = stream_session_id;
	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, chunk_id) = chunk_id;
	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, offset) = offset;
	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, last_chunk) = last_chunk;
	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, addr) = (uint32_t)address;
	req_data->SSF_SUIT_REQ_ARG(chunk_enqueue, size) = size;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("Chunk enqueue, ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(chunk_enqueue);
	err = rsp_data->SSF_SUIT_RSP_ARG(chunk_enqueue, ret);

	LOG_INF("Chunk enqueue, ssf_client_send_request return code: %d", err);
	return err;
}

suit_plat_err_t suit_ipc_streamer_chunk_status_req(uint32_t stream_session_id,
						   suit_ipc_streamer_chunk_info_t *chunk_info,
						   size_t *chunk_info_count)
{
	int err;
	struct suit_req req = { 0 };
	struct suit_rsp rsp = { 0 };
	struct suit_chunk_status_req *req_data = { 0 };
	struct suit_chunk_status_rsp *rsp_data = { 0 };

	LOG_INF("Chunk status request, stream_session_id: %d", stream_session_id);

	if (chunk_info == NULL || chunk_info_count == NULL) {
		LOG_INF("Chunk status request, invalid arguments");
		return SUIT_PLAT_ERR_NOMEM;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(chunk_status);
	req_data = &req.SSF_SUIT_REQ(chunk_status);
	req_data->SSF_SUIT_REQ_ARG(chunk_status, stream_session_id) = stream_session_id;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("Chunk status request, ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(chunk_status);
	err = rsp_data->SSF_SUIT_RSP_ARG(chunk_status, ret);
	LOG_INF("Chunk status request, ssf_client_send_request return code: %d", err);

	if (err == SUIT_PLAT_SUCCESS) {
		size_t count = rsp_data->SSF_SUIT_RSP_ARG(chunk_status,
							  chunk_info_suit_chunk_info_entry_m_count);

		if (count > *chunk_info_count) {
			*chunk_info_count = 0;
			LOG_ERR("Chunk status request, not enough space to store response");
			return SUIT_PLAT_ERR_NOMEM;
		}

		for (int i = 0; i < count; ++i) {
			struct suit_chunk_info_entry *in_entry = &rsp_data->SSF_SUIT_RSP_ARG(
				chunk_status, chunk_info_suit_chunk_info_entry_m)[i];

			suit_ipc_streamer_chunk_info_t *tgt_entry = &chunk_info[i];

			tgt_entry->chunk_id = in_entry->suit_chunk_info_entry_chunk_id;
			tgt_entry->status = in_entry->suit_chunk_info_entry_status;
		}

		*chunk_info_count = count;
	}

	return err;
}

suit_plat_err_t
suit_ipc_streamer_chunk_status_evt_subscribe(suit_ipc_streamer_chunk_status_notify_fn notify_fn,
					     void *context)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	/* Just one client (ipc streamer provider) can be subscribed for notification at time */
	if (chunk_status_notify_fn == NULL) {
		chunk_status_notify_fn = notify_fn;
		chunk_status_notify_context = context;
		LOG_INF("Subscribed for chunk status update");
	} else {
		err = SUIT_PLAT_ERR_NO_RESOURCES;
		LOG_ERR("Failed to subscribe for chunk status update");
	}

	return err;
}

void suit_ipc_streamer_chunk_status_evt_unsubscribe(void)
{
	chunk_status_notify_fn = NULL;
	chunk_status_notify_context = NULL;
	LOG_INF("Unsubscribed from chunk status update");
}

suit_plat_err_t
suit_ipc_streamer_missing_image_evt_subscribe(suit_ipc_streamer_missing_image_notify_fn notify_fn,
					      void *context)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	/* Just one client (ipc streamer provider) can be subscribed for notification at time */
	if (missing_image_notify_fn == NULL) {
		missing_image_notify_fn = notify_fn;
		missing_image_notify_context = context;
		LOG_INF("Subscribed for image request");
	} else {
		err = SUIT_PLAT_ERR_NO_RESOURCES;
		LOG_ERR("Failed to subscribe for image request");
	}

	return err;
}

void suit_ipc_streamer_missing_image_evt_unsubscribe(void)
{
	missing_image_notify_fn = NULL;
	missing_image_notify_context = NULL;
	LOG_INF("Unsubscribed from image request");
}

suit_plat_err_t suit_ipc_streamer_requestor_init(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	err = evt_listener_init();
	if (err != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return SUIT_PLAT_SUCCESS;
}
#endif /*CONFIG_SUIT_STREAM_IPC_PROVIDER*/
