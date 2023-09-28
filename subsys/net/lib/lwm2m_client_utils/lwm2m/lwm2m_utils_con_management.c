/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <lwm2m_engine.h>
#include <lwm2m_util.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_ncs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lwm2m_sec_con_man, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

static bool dtls_stored;

static void lwm2m_utils_dtls_save(struct lwm2m_ctx *ctx)
{
	int ret;
	int store_dtls = 1;

	if (dtls_stored) {
		return;
	}

	ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_CONN_SAVE, &store_dtls,
			       sizeof(store_dtls));
	if (ret) {
		ret = -errno;
		LOG_ERR("Failed to store %d DTLS session: %d", ctx->sock_fd, ret);
	} else {
		LOG_DBG("DTLS session stored ok");
		dtls_stored = true;
	}
}

static void lwm2m_utils_dtls_load(struct lwm2m_ctx *ctx)
{
	int ret;
	int load_dtls = 1;

	if (!dtls_stored) {
		return;
	}

	ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_CONN_LOAD, &load_dtls,
			       sizeof(load_dtls));
	if (ret) {
		ret = -errno;
		LOG_ERR("Failed to load %d DTLS session: %d", ctx->sock_fd, ret);
	} else {
		LOG_DBG("DTLS session load ok");
		dtls_stored = false;
	}
}

void lwm2m_utils_connection_manage(struct lwm2m_ctx *client,
				      enum lwm2m_rd_client_event *client_event)
{
	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_LTE_CONNEVAL)) {
		lwm2m_utils_conneval(client, client_event);
	}

	switch (*client_event) {
	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_RAI)) {
			lwm2m_rai_last();
		}

		if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_DTLS_CON_MANAGEMENT)) {
			lwm2m_utils_dtls_save(client);
		}
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
		if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_DTLS_CON_MANAGEMENT)) {
			lwm2m_utils_dtls_load(client);
		}
		break;
	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_DTLS_CON_MANAGEMENT)) {
			/* Socket is cloded and dtls session also */
			dtls_stored = false;
		}
		break;

	default:
		break;
	}
}
