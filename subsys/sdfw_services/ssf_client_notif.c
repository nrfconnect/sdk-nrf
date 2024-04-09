/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/ssf_client_notif.h>

#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"
#include "ssf_client_transport.h"

#include <zcbor_decode.h>

SSF_CLIENT_LOG_DECLARE(ssf_client, CONFIG_SSF_CLIENT_LOG_LEVEL);

static struct ssf_client_notif_listener *listeners[CONFIG_SSF_CLIENT_REGISTERED_LISTENERS_MAX];
static struct ssf_client_sem listeners_sem;

static int decode_header(const uint8_t *buf, size_t buf_size, uint16_t *service_id,
			 uint16_t *service_version, struct ssf_notification *notif)
{
	uint32_t id;
	uint32_t version;

	ZCBOR_STATE_D(state, CONFIG_SSF_CLIENT_ZCBOR_MAX_BACKUPS, buf, buf_size, 1, 0);

	bool success = zcbor_list_start_decode(state) &&
		       ((zcbor_uint32_decode(state, &id) && zcbor_uint32_decode(state, &version)) ||
			(zcbor_list_map_end_force_decode(state), false)) &&
		       zcbor_list_end_decode(state);

	if (!success) {
		return -SSF_EPROTO;
	}

	notif->data = state->payload;
	notif->data_len = buf_size - (state->payload - buf);
	notif->pkt = buf;

	*service_id = (uint16_t)id;
	*service_version = (uint16_t)version;

	return 0;
}

int ssf_client_notif_init(void)
{
	return ssf_client_sem_init(&listeners_sem);
}

void ssf_client_notif_handler(const uint8_t *pkt, size_t pkt_len)
{
	int err;
	uint16_t service_id;
	uint16_t server_service_version;
	struct ssf_notification notif;
	struct ssf_client_notif_listener *listener = NULL;

	err = decode_header(pkt, pkt_len, &service_id, &server_service_version, &notif);
	if (err != 0) {
		ssf_client_transport_decoding_done(pkt);
		return;
	}

	ssf_client_sem_take(&listeners_sem, SSF_CLIENT_SEM_WAIT_FOREVER);

	for (int i = 0; i < CONFIG_SSF_CLIENT_REGISTERED_LISTENERS_MAX; i++) {
		if (listeners[i] != NULL && listeners[i]->id == service_id) {
			listener = listeners[i];
			break;
		}
	}

	ssf_client_sem_give(&listeners_sem);

	if (listener == NULL) {
		ssf_client_transport_decoding_done(pkt);
		SSF_CLIENT_LOG_ERR("No listener for service 0x%x is registered", service_id);
		return;
	}

	if (listener->handler == NULL) {
		ssf_client_transport_decoding_done(pkt);
		SSF_CLIENT_LOG_ERR("Handler missing for listener, service 0x%x", listener->id);
		return;
	}

	if (listener->version != server_service_version) {
		ssf_client_transport_decoding_done(pkt);
		SSF_CLIENT_LOG_ERR("Notification version number mismatch: service 0x%x, "
				   "client ver %d, server ver %d",
				   listener->id, listener->version, server_service_version);
		return;
	}

	notif.notif_decode = listener->notif_decode;

	err = listener->handler(&notif, listener->context);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Notification handler failed, service 0x%x, err %d",
				   listener->id, err);
	}
}

int ssf_client_notif_decode(const struct ssf_notification *notif, void *decoded_notif)
{
	int err;

	if (notif == NULL || notif->data == NULL || notif->notif_decode == NULL ||
	    decoded_notif == NULL) {
		return -SSF_EINVAL;
	}

	err = notif->notif_decode(notif->data, notif->data_len, decoded_notif, NULL);
	if (err != 0) {
		return -SSF_EPROTO;
	}

	return 0;
}

void ssf_client_notif_decode_done(struct ssf_notification *notif)
{
	if (notif == NULL || notif->pkt == NULL) {
		SSF_CLIENT_LOG_ERR("Failed to free notif buffer");
		return;
	}

	ssf_client_transport_decoding_done((uint8_t *)notif->pkt);
}

int ssf_client_notif_register(struct ssf_client_notif_listener *listener, void *context)
{
	int err;

	if (listener == NULL || listener->handler == NULL || listener->notif_decode == NULL) {
		return -SSF_EINVAL;
	}

	ssf_client_sem_take(&listeners_sem, SSF_CLIENT_SEM_WAIT_FOREVER);

	if (listener->is_registered) {
		err = -SSF_EALREADY;
		goto clean_exit;
	}

	for (int i = 0; i < CONFIG_SSF_CLIENT_REGISTERED_LISTENERS_MAX; i++) {
		if (listeners[i] == NULL) {
			listener->is_registered = true;
			listener->context = context;
			listeners[i] = listener;

			err = 0;
			goto clean_exit;
		}
	}

	err = -SSF_ENOBUFS;

clean_exit:
	ssf_client_sem_give(&listeners_sem);
	return err;
}

int ssf_client_notif_deregister(struct ssf_client_notif_listener *listener)
{
	int err;

	if (listener == NULL) {
		return -SSF_EINVAL;
	}

	ssf_client_sem_take(&listeners_sem, SSF_CLIENT_SEM_WAIT_FOREVER);

	for (int i = 0; i < CONFIG_SSF_CLIENT_REGISTERED_LISTENERS_MAX; i++) {
		if (listeners[i] == listener) {
			listeners[i] = NULL;
			listener->context = NULL;
			listener->is_registered = false;

			err = 0;
			goto clean_exit;
		}
	}

	err = -SSF_ENXIO;

clean_exit:
	ssf_client_sem_give(&listeners_sem);
	return err;
}
