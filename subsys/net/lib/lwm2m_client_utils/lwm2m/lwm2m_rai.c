/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/lwm2m.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>
#include <net/lwm2m_client_utils.h>

LOG_MODULE_REGISTER(lwm2m_rai, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);
static void lwm2m_set_socket_state(int sock_fd, enum lwm2m_socket_states state);

static bool activate_rai;

static int set_rel14feat(void)
{
	int ret;

	ret = nrf_modem_at_printf("AT%%REL14FEAT=0,1,0,0,0");
	if (ret < 0) {
		LOG_ERR("AT command failed, error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int lwm2m_rai_req(enum lwm2m_rai_mode mode)
{
	int ret;

	ret = nrf_modem_at_printf("AT%%RAI=%d", mode);
	if (ret) {
		LOG_ERR("AT command failed, error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}

NRF_MODEM_LIB_ON_INIT(lwm2m_init_rai, lwm2m_init_rai, NULL);
static void lwm2m_init_rai(int ret, void *ctx)
{
	ARG_UNUSED(ret);
	ARG_UNUSED(ctx);

	int r;

	r = set_rel14feat();
	if (r) {
		LOG_ERR("set_rel14feat failed, error code: %d", r);
	}

	r = lwm2m_rai_req(LWM2M_RAI_MODE_ENABLED);
	if (r) {
		LOG_ERR("RAI request failed, error code: %d", r);
	}
}

void lwm2m_utils_rai_event_cb(struct lwm2m_ctx *client,
				      enum lwm2m_rd_client_event *client_event)
{
	switch (*client_event) {
	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
	case LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED:
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
	case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
		/* RAI can be enabled as we are now registered and server have refreshed all the
		 * data
		 */
		if (!activate_rai) {
			LOG_INF("RAI enabled");
			client->set_socket_state = lwm2m_set_socket_state;
			activate_rai = true;
		}
		break;


	default:
		/* Stop RAI hints until we have properly registered and server have
		 * refreshed all the data
		 */
		if (activate_rai) {
			LOG_INF("RAI disabled");
			activate_rai = false;
			client->set_socket_state = NULL;
			lwm2m_set_socket_state(client->sock_fd, LWM2M_SOCKET_STATE_ONGOING);
		}
		break;
	}
}

static void lwm2m_set_socket_state(int sock_fd, enum lwm2m_socket_states state)
{
	int opt = -1;
	int ret;
	static const char * const opt_names[] = {
		"ONGOING",
		"ONE_RESP",
		"LAST",
		"NO_DATA",
	};

	if (!activate_rai || sock_fd < 0) {
		return;
	}
	switch (state) {
	case LWM2M_SOCKET_STATE_ONGOING:
		opt = SO_RAI_ONGOING;
		break;
	case LWM2M_SOCKET_STATE_ONE_RESPONSE:
		opt = SO_RAI_ONE_RESP;
		break;
	case LWM2M_SOCKET_STATE_LAST:
		opt = SO_RAI_LAST;
		break;
	case LWM2M_SOCKET_STATE_NO_DATA:
		opt = SO_RAI_NO_DATA;
		break;
	}

	LOG_DBG("Set socket option SO_RAI_%s\n", opt_names[state]);
	ret = setsockopt(sock_fd, SOL_SOCKET, opt, NULL, 0);

	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Failed to set RAI socket option, error code: %d", ret);
	}
}
