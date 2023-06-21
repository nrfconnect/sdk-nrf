/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(l2_wifi_mgr_conn);
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <net/wifi_mgmt_ext.h>
#include <net/l2_wifi_connect.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/src/supp_events.h>
#include "conn_mgr_private.h"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
				NET_EVENT_WIFI_DISCONNECT_RESULT)

#define WPA_SUPP_EVENTS (NET_EVENT_WPA_SUPP_READY)

static struct net_mgmt_event_callback net_l2_mgmt_cb;
static struct net_mgmt_event_callback net_wpa_supp_cb;
int connection_status;

int net_l2_wifi_connect(struct conn_mgr_conn_binding *const binding);
int net_l2_wifi_disconnect(struct conn_mgr_conn_binding *const binding);

K_SEM_DEFINE(wpa_supp_ready_sem, 0, 1);

/* Delayable work used to handle Wi-Fi connection timeouts. */
static void wifi_conn_work_handler(struct k_work *work);
K_THREAD_STACK_DEFINE(l2_wifi_wq_stack_area, CONFIG_L2_WIFI_CONN_WQ_STACK_SIZE);
struct k_work_q wifi_conn_wq;
static K_WORK_DELAYABLE_DEFINE(wifi_conn_work, wifi_conn_work_handler);

/* Store a local reference to the connection binding */
static struct net_if *wifi_iface;

static void wifi_conn_work_handler(struct k_work *work)
{
	if (connection_status != WIFI_STATE_COMPLETED) {
		net_l2_wifi_disconnect(NULL);
	}
}

static int wifi_conn_timeout_schedule(void)
{
	int rc = 0;

	if (conn_mgr_if_get_timeout(wifi_iface) > CONN_MGR_IF_NO_TIMEOUT) {
		rc = k_work_schedule_for_queue(&wifi_conn_wq, &wifi_conn_work,
					       K_SECONDS(conn_mgr_if_get_timeout(wifi_iface)));
	}

	return rc;
}

static void net_l2_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		connection_status = WIFI_STATE_DISCONNECTED;
	} else {
		connection_status = WIFI_STATE_COMPLETED;
	}
	k_work_cancel_delayable(&wifi_conn_work);
}

static void net_l2_wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	if (iface != wifi_iface) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		net_l2_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		if (conn_mgr_if_get_flag(wifi_iface, CONN_MGR_IF_PERSISTENT)) {
			k_work_reschedule_for_queue(&wifi_conn_wq, &wifi_conn_work,
							K_SECONDS(conn_mgr_if_get_timeout(iface)));
		} else {
			/* For disconnection that may have been prompted by AP */
			net_l2_wifi_disconnect(NULL);
		}
		break;
	default:
		break;
	}
}

static void wpa_supp_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WPA_SUPP_READY:
		k_sem_give(&wpa_supp_ready_sem);
		break;
	default:
		break;
	}
}

int net_l2_wifi_connect(struct conn_mgr_conn_binding *const binding)
{
	int rc;

	ARG_UNUSED(binding);

	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, wifi_iface, NULL, 0);

	if (rc) {
		LOG_ERR("net management connect_stored request failed: %d\n", rc);
		return -ENOEXEC;
	}

	return wifi_conn_timeout_schedule();
}

int net_l2_wifi_disconnect(struct conn_mgr_conn_binding *const binding)
{
	ARG_UNUSED(binding);

	return net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wifi_iface, NULL, 0);
}

void net_l2_wifi_init(struct conn_mgr_conn_binding *const binding)
{
	int ret;
	int persistent;

	net_mgmt_init_event_callback(&net_l2_mgmt_cb,
				     net_l2_wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);
	net_mgmt_add_event_callback(&net_l2_mgmt_cb);

	net_mgmt_init_event_callback(&net_wpa_supp_cb,
				     wpa_supp_event_handler,
				     WPA_SUPP_EVENTS);
	net_mgmt_add_event_callback(&net_wpa_supp_cb);

	k_sem_take(&wpa_supp_ready_sem, K_FOREVER);
	wifi_iface = binding->iface;

	if (!IS_ENABLED(CONFIG_L2_WIFI_CONNECTIVITY_AUTO_CONNECT)) {
		ret = conn_mgr_if_set_flag(wifi_iface, CONN_MGR_IF_NO_AUTO_CONNECT, true);
		if (ret) {
			LOG_ERR("conn_mgr_if_set_flag, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, wifi_iface);
			return;
		}
	}

	if (!IS_ENABLED(CONFIG_L2_WIFI_CONNECTIVITY_AUTO_DOWN)) {
		ret = conn_mgr_if_set_flag(wifi_iface, CONN_MGR_IF_NO_AUTO_DOWN, true);
		if (ret) {
			LOG_ERR("conn_mgr_if_set_flag, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, wifi_iface);
			return;
		}
	}

	persistent = IS_ENABLED(CONFIG_L2_WIFI_CONNECTIVITY_CONNECTION_PERSISTENCE);
	binding->flags |= (persistent == 1) ? BIT(CONN_MGR_IF_PERSISTENT) : 0;
	k_work_queue_init(&wifi_conn_wq);

	k_work_queue_start(&wifi_conn_wq,
			   l2_wifi_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(l2_wifi_wq_stack_area),
			   0,
			   NULL);
}

static struct conn_mgr_conn_api l2_wifi_conn_api = {
	.connect = net_l2_wifi_connect,
	.disconnect = net_l2_wifi_disconnect,
	.init = net_l2_wifi_init,
};

CONN_MGR_CONN_DEFINE(L2_CONN_WLAN0, &l2_wifi_conn_api);
