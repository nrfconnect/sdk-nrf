/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_l2_dect_conn_mgr, CONFIG_NET_L2_DECT_CONN_MGR_LOG_LEVEL);

static struct net_mgmt_event_callback dect_mgmt_cb;
static int64_t connection_timeout;

static int dect_nrp_connect(struct net_if *iface)
{
	struct dect_settings current_settings;
	int ret;

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, iface, &current_settings,
		       sizeof(current_settings));
	if (ret) {
		LOG_ERR("%s: cannot read current settings: %d", (__func__), ret);
		return ret;
	}

	if (net_if_is_carrier_ok(iface) == false &&
	    !conn_mgr_if_get_flag(
		iface, CONN_MGR_IF_NO_AUTO_CONNECT)) {
		/* No carrier yet (i.e. driver have not activated DECT stack on modem),
		 * but auto-connect is enabled, so wait for the carrier/activate.
		 * So, after activation, the connect will continue. Do not consider this an error
		 * at this point.
		 */
		LOG_DBG("%s: No carrier yet, waiting for carrier/activate", (__func__));
		return 0;
	}

	if (current_settings.device_type & DECT_DEVICE_TYPE_PT) {
		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, iface, NULL, 0);
		if (ret) {
			LOG_ERR("%s: cannot initiate request for joining a network: %d",
			       (__func__),
			       ret);
		}
	} else {
		__ASSERT_NO_MSG(current_settings.device_type & DECT_DEVICE_TYPE_FT);

		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, iface, NULL, 0);
		if (ret) {
			LOG_ERR("%s: cannot initiate request for creating a network: %d",
			       (__func__),
			       ret);
		}
	}

	return ret;
}

int dect_nrp_net_if_connect(struct conn_mgr_conn_binding *const binding)
{
	if (binding->timeout) {
		connection_timeout = k_uptime_get() + binding->timeout * MSEC_PER_SEC;
	} else {
		connection_timeout = 0;
	}

	return dect_nrp_connect(binding->iface);
}

int dect_nrp_net_if_disconnect(struct conn_mgr_conn_binding *const binding)
{
	struct dect_settings current_settings;
	int ret;

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, binding->iface, &current_settings,
		       sizeof(current_settings));
	if (ret) {
		LOG_ERR("%s: cannot read current settings: %d", (__func__), ret);
		goto exit;
	}
	if (current_settings.device_type & DECT_DEVICE_TYPE_PT) {
		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_UNJOIN, binding->iface, NULL, 0);
		if (ret) {
			LOG_ERR("%s: cannot initiate request for unjoining a network: %d",
			       (__func__),
			       ret);
		}
	} else {
		__ASSERT_NO_MSG(current_settings.device_type & DECT_DEVICE_TYPE_FT);
		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, binding->iface, NULL, 0);
		if (ret) {
			LOG_ERR("%s: cannot initiate request for removing a network: %d",
			       (__func__),
			       ret);
		}
	}
exit:
	return ret;
}

static void dect_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_DECT_NETWORK_STATUS: {
		struct dect_network_status_evt *evt = (struct dect_network_status_evt *)cb->info;

		if (conn_mgr_if_get_flag(iface, CONN_MGR_IF_PERSISTENT) &&
		    evt->network_status == DECT_NETWORK_STATUS_FAILURE &&
		    evt->dect_err_cause == DECT_STATUS_RD_NOT_FOUND) {
			if (!connection_timeout || k_uptime_get() < connection_timeout) {
				dect_nrp_connect(iface);
				LOG_INF("NET_EVENT_DECT_NETWORK_STATUS: Reconnecting...");
			}
		}
		break;
	}
	case NET_EVENT_DECT_ASSOCIATION_CHANGED: {
		struct dect_association_changed_evt *evt =
			(struct dect_association_changed_evt *)cb->info;

		if (conn_mgr_if_get_flag(iface, CONN_MGR_IF_PERSISTENT) &&
		    evt->neighbor_role == DECT_NEIGHBOR_ROLE_PARENT &&
		    evt->association_change_type == DECT_ASSOCIATION_RELEASED &&
		    evt->association_released.release_cause !=
					DECT_RELEASE_CAUSE_CONNECTION_TERMINATION) {
			/* Lost connection to parent but not because on purpose so reconnect. */
			dect_nrp_net_if_connect(conn_mgr_if_get_binding(iface));
			LOG_INF("NET_EVENT_DECT_ASSOCIATION_CHANGED: Reconnecting...");
		}
		break;
	}
	case NET_EVENT_DECT_ACTIVATE_DONE: {
		struct dect_settings current_settings;
		int ret;
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		if (evt->status == DECT_STATUS_OK &&
		    !conn_mgr_if_get_flag(iface, CONN_MGR_IF_NO_AUTO_CONNECT) &&
		    net_if_is_admin_up(iface)) {
			ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ,
				       iface, &current_settings, sizeof(current_settings));
			if (ret) {
				LOG_ERR("NET_EVENT_DECT_ACTIVATE_DONE: "
				       "cannot read current settings: %d", ret);
				break;
			}
			/* Trigger connecting right away, no need to wait if sink is up for FT.
			 * IPv6 addressing changes are passed on the run later when global
			 * connectivity.
			 */
			LOG_INF("NET_EVENT_DECT_ACTIVATE_DONE: Auto-connecting...");
			dect_nrp_net_if_connect(conn_mgr_if_get_binding(iface));
		}
		break;
	}
	default:
		break;
	}
}

void dect_nrp_net_if_init(struct conn_mgr_conn_binding *const binding)
{
	/* Configure the interface according to kconfig values. */
	if (!IS_ENABLED(CONFIG_NET_L2_DECT_CONN_MGR_AUTO_CONNECT)) {
		conn_mgr_binding_set_flag(binding, CONN_MGR_IF_NO_AUTO_CONNECT, true);
	}

	if (!IS_ENABLED(CONFIG_NET_L2_DECT_CONN_MGR_AUTO_DOWN)) {
		conn_mgr_binding_set_flag(binding, CONN_MGR_IF_NO_AUTO_DOWN, true);
	}

	if (IS_ENABLED(CONFIG_NET_L2_DECT_CONN_MGR_CONNECTION_PERSISTENCE)) {
		conn_mgr_binding_set_flag(binding, CONN_MGR_IF_PERSISTENT, true);
	}

	if (CONFIG_NET_L2_DECT_CONN_MGR_CONNECT_TIMEOUT_SECONDS > 0) {
		conn_mgr_if_set_timeout(binding->iface,
					CONFIG_NET_L2_DECT_CONN_MGR_CONNECT_TIMEOUT_SECONDS);
	}

	net_mgmt_init_event_callback(&dect_mgmt_cb, dect_event_handler,
		NET_EVENT_DECT_NETWORK_STATUS | NET_EVENT_DECT_ASSOCIATION_CHANGED |
		NET_EVENT_DECT_ACTIVATE_DONE | NET_EVENT_DECT_SINK_STATUS);
	net_mgmt_add_event_callback(&dect_mgmt_cb);
}

static struct conn_mgr_conn_api l2_dect_conn_api = {
	.connect = dect_nrp_net_if_connect,
	.disconnect = dect_nrp_net_if_disconnect,
	.init = dect_nrp_net_if_init,
};

CONN_MGR_CONN_DEFINE(CONNECTIVITY_DECT_MGMT, &l2_dect_conn_api);
