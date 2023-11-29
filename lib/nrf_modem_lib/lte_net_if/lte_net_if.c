/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include "lte_ip_addr_helper.h"

LOG_MODULE_REGISTER(nrf_modem_lib_netif, CONFIG_NRF_MODEM_LIB_NET_IF_LOG_LEVEL);

/* This library requires that Zephyr's IPv4 and IPv6 stacks are enabled.
 *
 * The modem supports both IPv6 and IPv4. At any time, either IP families can be given by
 * the network, independently of each other. Because of this we require that the corresponding
 * IP stacks are enabled in Zephyr NET in order to add and remove both IP family address types
 * to and from the network interface.
 */
BUILD_ASSERT(IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4), "IPv6 and IPv4 required");

BUILD_ASSERT((CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE >= 1024),
	     "Stack size of the connection manager internal thread is too low. "
	     "Increase CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE to a minimum of 1024");

/* Forward declarations */
static void connection_timeout_work_fn(struct k_work *work);
static void connection_timeout_schedule(void);

/* Delayable work used to handle LTE connection timeouts. */
static K_WORK_DELAYABLE_DEFINE(connection_timeout_work, connection_timeout_work_fn);

static int lte_net_if_disconnect(struct conn_mgr_conn_binding *const if_conn);

/* Local reference to the network interface the l2 connection layer is bound to. */
static struct net_if *iface_bound;

/* Container and protection mutex for internal state of the connectivity binding. */
static struct {
	/* Tracks whether a PDN bearer is currently active. */
	bool has_pdn;

	/* Tracks whether a serving cell is currently or was recently available. */
	bool has_cell;
} internal_state;

static K_MUTEX_DEFINE(internal_state_lock);

/* Local functions */

/* Function called whenever an irrecoverable error occurs and LTE is activated.
 * Notifies a fatal connectivity error and deactivates LTE.
 */
static void fatal_error_notify_and_disconnect(void)
{
	net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
	net_if_dormant_on(iface_bound);
	(void)lte_net_if_disconnect(NULL);
}

/* Handler called on modem faults. */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem error: 0x%x, PC: 0x%x", fault_info->reason, fault_info->program_counter);
	fatal_error_notify_and_disconnect();
}

/* Called when we detect LTE connectivity has been gained.
 *
 * Marks the iface as active and cancels any pending timeout.
 */
static void become_active(void)
{
	LOG_DBG("Becoming active");
	net_if_dormant_off(iface_bound);
	k_work_cancel_delayable(&connection_timeout_work);
}

/* Called when we detect LTE connectivity has been lost.
 *
 * Marks the iface as dormant, and depending on persistence either schedules a timeout, or cancels
 * any attempt to regain connectivity.
 */
static void become_dormant(void)
{
	int ret;

	LOG_DBG("Becoming dormant");
	net_if_dormant_on(iface_bound);

	/* Return immediately after removing IP addresses in the case where PDN has been
	 * deactivated due to the modem being shutdown.
	 */
	if (!nrf_modem_is_initialized()) {
		return;
	}

	if (conn_mgr_if_get_flag(iface_bound, CONN_MGR_IF_PERSISTENT)) {
		/* If persistence is enabled, don't deactivate LTE. Let the modem try to
		 * re-establish the connection. Schedule a connection timeout work that
		 * deactivates LTE if the modem is not able to reconnect before the set timeout.
		 */
		connection_timeout_schedule();
	} else {
		/* If persistence is disabled, LTE is deactivated upon a lost connection.
		 * Re-establishment is reliant on the application calling conn_mgr_if_connect().
		 */
		ret = lte_net_if_disconnect(NULL);
		if (ret) {
			LOG_ERR("lte_net_if_disconnect, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
		}
	}
}

/* Updates the internal state as instructed and checks if connectivity was gained or lost.
 * Not thread safe.
 */
static void update_connectivity(bool has_pdn, bool has_cell)
{
	bool had_connectivity = internal_state.has_pdn && internal_state.has_cell;
	bool has_connectivity = has_pdn && has_cell;

	internal_state.has_pdn = has_pdn;
	internal_state.has_cell = has_cell;

	if (had_connectivity != has_connectivity) {
		if (has_connectivity) {
			become_active();
		} else {
			become_dormant();
		}
	}
}

static void update_has_pdn(bool has_pdn)
{
	(void)k_mutex_lock(&internal_state_lock, K_FOREVER);

	if (has_pdn != internal_state.has_pdn) {
		if (has_pdn) {
			LOG_DBG("Gained PDN bearer");
		} else {
			LOG_DBG("Lost PDN bearer");
		}

		update_connectivity(has_pdn, internal_state.has_cell);
	}

	(void)k_mutex_unlock(&internal_state_lock);
}

static void update_has_cell(bool has_cell)
{
	(void)k_mutex_lock(&internal_state_lock, K_FOREVER);

	if (has_cell != internal_state.has_cell) {
		if (has_cell) {
			LOG_DBG("Gained serving cell");
		} else {
			LOG_DBG("Lost serving cell");
		}

		update_connectivity(internal_state.has_pdn, has_cell);
	}

	(void)k_mutex_unlock(&internal_state_lock);
}

/* Work handler that deactivates LTE. */
static void connection_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_DBG("LTE connection timeout");

	int ret = lte_net_if_disconnect(NULL);

	if (ret) {
		LOG_ERR("lte_net_if_disconnect, error: %d", ret);
		net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
	}

	net_mgmt_event_notify(NET_EVENT_CONN_IF_TIMEOUT, iface_bound);
}

static void connection_timeout_schedule(void)
{
	int timeout_seconds = conn_mgr_if_get_timeout(iface_bound);

	if (timeout_seconds > CONN_MGR_IF_NO_TIMEOUT) {
		k_work_reschedule(&connection_timeout_work, K_SECONDS(timeout_seconds));

		LOG_DBG("Trigger connection timeout handler in %d seconds", timeout_seconds);
	}
}

static void on_pdn_activated(void)
{
	int ret;

	ret = lte_ipv4_addr_add(iface_bound);
	if (ret == -ENODATA) {
		LOG_WRN("No IPv4 address given by the network");
		return;
	} else if (ret) {
		LOG_ERR("ipv4_addr_add, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}

	update_has_pdn(true);
}

static void on_pdn_deactivated(void)
{
	int ret;

	ret = lte_ipv4_addr_remove(iface_bound);
	if (ret) {
		LOG_ERR("ipv4_addr_remove, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}

	ret = lte_ipv6_addr_remove(iface_bound);
	if (ret) {
		LOG_ERR("ipv6_addr_remove, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}

	update_has_pdn(false);
}

static void on_pdn_ipv6_up(void)
{
	int ret;

	ret = lte_ipv6_addr_add(iface_bound);
	if (ret) {
		LOG_ERR("ipv6_addr_add, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}
}

static void on_pdn_ipv6_down(void)
{
	int ret;

	ret = lte_ipv6_addr_remove(iface_bound);
	if (ret) {
		LOG_ERR("ipv6_addr_remove, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}
}

/* Event handlers */
static void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
#if CONFIG_PDN_ESM_STRERROR
	case PDN_EVENT_CNEC_ESM:
		LOG_DBG("Event: PDP context %d, %s", cid, pdn_esm_strerror(reason));
		break;
#endif
	case PDN_EVENT_ACTIVATED:
		LOG_DBG("PDN connection activated");
		on_pdn_activated();
		break;
	case PDN_EVENT_NETWORK_DETACH:
		LOG_DBG("PDN network detached");
		on_pdn_deactivated();
		break;
	case PDN_EVENT_DEACTIVATED:
		LOG_DBG("PDN connection deactivated");
		on_pdn_deactivated();
		break;
	case PDN_EVENT_IPV6_UP:
		LOG_DBG("PDN IPv6 up");
		on_pdn_ipv6_up();
		break;
	case PDN_EVENT_IPV6_DOWN:
		LOG_DBG("PDN IPv6 down");
		on_pdn_ipv6_down();
		break;
	default:
		LOG_ERR("Unexpected PDN event: %d", event);
		break;
	}
}

static void lte_reg_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type != LTE_LC_EVT_NW_REG_STATUS) {
		return;
	}

	switch (evt->nw_reg_status) {
	case LTE_LC_NW_REG_REGISTERED_HOME:
		__fallthrough;
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		/* Mark serving cell as available. */
		LOG_DBG("Registered to serving cell");
		update_has_cell(true);
		break;
	case LTE_LC_NW_REG_SEARCHING:
		/* Searching for a new cell, do not consider this cell loss unless it
		 * fails (which will generate a new LTE_LC_EVT_NW_REG_STATUS event with
		 * an unregistered status).
		 */
		break;
	default:
		LOG_DBG("Not registered to serving cell");
		/* Mark the serving cell as lost. */
		update_has_cell(false);
		break;
	}
}

static void lte_net_if_init(struct conn_mgr_conn_binding *if_conn)
{
	int ret;
	int timeout = CONFIG_NRF_MODEM_LIB_NET_IF_CONNECT_TIMEOUT_SECONDS;

	net_if_dormant_on(if_conn->iface);

	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT)) {
		conn_mgr_binding_set_flag(if_conn, CONN_MGR_IF_NO_AUTO_CONNECT, true);
	}

	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN)) {
		conn_mgr_binding_set_flag(if_conn, CONN_MGR_IF_NO_AUTO_DOWN, true);
	}

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_CONNECTION_PERSISTENCE)) {
		conn_mgr_binding_set_flag(if_conn, CONN_MGR_IF_PERSISTENT, true);
	}

	/* Set default values for timeouts */
	if_conn->timeout = (timeout > CONN_MGR_IF_NO_TIMEOUT) ? timeout : CONN_MGR_IF_NO_TIMEOUT;

	if (if_conn->timeout > CONN_MGR_IF_NO_TIMEOUT) {
		LOG_DBG("Connection timeout is enabled and set to %d seconds", if_conn->timeout);
	} else {
		LOG_DBG("Connection timeout is disabled");
	}

	LOG_DBG("Connection persistence is %s",
		(if_conn->flags & BIT(CONN_MGR_IF_PERSISTENT)) ? "enabled" : "disabled");

	/* Register handler for default PDP context 0. */
	ret = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (ret) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", ret);
		net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, if_conn->iface);
		return;
	}

	/* Register handler for registration status notifications */
	lte_lc_register_handler(lte_reg_handler);

	/* Keep local reference to the network interface that the connectivity layer is bound to. */
	iface_bound = if_conn->iface;

	LOG_DBG("Modem network interface ready");
}

int lte_net_if_enable(void)
{
	if (!nrf_modem_is_initialized()) {
		return nrf_modem_lib_init();
	}

	return 0;
}

int lte_net_if_disable(void)
{
	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_DOWN_DEFAULT_LTE_DISCONNECT)) {
		return lte_net_if_disconnect(NULL);
	} else {
		return nrf_modem_lib_shutdown();
	}
}

static int lte_net_if_connect(struct conn_mgr_conn_binding *const if_conn)
{
	ARG_UNUSED(if_conn);

	LOG_DBG("Connecting to LTE");

	int ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);

	if (ret) {
		LOG_ERR("lte_lc_func_mode_set, error: %d", ret);
		return ret;
	}

	connection_timeout_schedule();

	return 0;
}

static int lte_net_if_disconnect(struct conn_mgr_conn_binding *const if_conn)
{
	ARG_UNUSED(if_conn);

	int ret;

	LOG_DBG("Disconnecting from LTE");

	k_work_cancel_delayable(&connection_timeout_work);

	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);
	if (ret) {
		LOG_ERR("lte_lc_func_mode_set, error: %d", ret);
		return ret;
	}

	return 0;
}

/* Bind connectity APIs.
 * extern in nrf91_sockets.c
 */
struct conn_mgr_conn_api lte_net_if_conn_mgr_api = {
	.init = lte_net_if_init,
	.connect = lte_net_if_connect,
	.disconnect = lte_net_if_disconnect,
};
