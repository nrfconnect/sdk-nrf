/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include "lte_ip_addr_helper.h"
#include "lte_connectivity.h"

LOG_MODULE_REGISTER(lte_connectivity, CONFIG_LTE_CONNECTIVITY_LOG_LEVEL);

/* This library requires that Zephyr's IPv4 and IPv6 stacks are enabled.
 *
 * The modem supports both IPv6 and IPv4. At any time, either IP families can be given by
 * the network, independently of each other. Because of this we require that the corresponding
 * IP stacks are enabled in Zephyr NET in order to add and remove both IP family address types
 * to and from the network interface.
 */
BUILD_ASSERT(IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4), "IPv6 and IPv4 required");

BUILD_ASSERT((CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE >= 1024),
	     "Stack size of the connection manager internal thread is too low. "
	     "Increase CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE to a minimum of 1024");

/* Forward declarations */
static void connection_timeout_work_fn(struct k_work *work);
int lte_connectivity_disconnect(struct conn_mgr_conn_binding *const if_conn);

/* Delayable work used to handle LTE connection timeouts. */
static K_WORK_DELAYABLE_DEFINE(connection_timeout_work, connection_timeout_work_fn);

/* Local reference to the network interface the l2 connection layer is bound to. */
static struct net_if *iface_bound;

/* Variable used to specify behavior when the network interface is brought down. */
static uint8_t net_if_down_options = LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN;

/* Local functions */

/* Function called whenever an irrecoverable error occurs and LTE is activated.
 * Notifies a fatal connectivity error and deactivates LTE.
 */
static void fatal_error_notify_and_disconnect(void)
{
	net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
	net_if_dormant_on(iface_bound);
	(void)lte_connectivity_disconnect(NULL);
}

/* Handler called on modem faults. */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem error: 0x%x, PC: 0x%x", fault_info->reason, fault_info->program_counter);
	fatal_error_notify_and_disconnect();
}

/* Work handler that deactivates LTE. */
static void connection_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_DBG("LTE connection timeout");

	int ret = lte_connectivity_disconnect(NULL);

	if (ret) {
		LOG_ERR("lte_connectivity_disconnect, error: %d", ret);
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

static int modem_init(void)
{
	LOG_DBG("Initializing nRF Modem Library");

	int ret = nrf_modem_lib_init();

	/* Handle return values related to modem DFU. */
	switch (ret) {
	case 0:
		/* Initialization successful, no action required. */
		LOG_DBG("nRF Modem Library initialized successfully");
		return 0;
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_DBG("Modem DFU successful. The modem will run the updated "
			"firmware after reinitialization.");
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("Modem DFU error: %d. The modem will automatically run the previous "
			"(non-updated) firmware after reinitialization.",
			ret);
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_ERR("Modem DFU not executed due to low voltage, error: %d. "
			"The modem will retry the update on reinitialization.",
			ret);
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		/* Fall through */
	default:
		LOG_ERR("The modem encountered a fatal error during DFU: %d", ret);
		return ret;
	}

	LOG_DBG("Reinitializing nRF Modem Library");

	ret = nrf_modem_lib_init();
	if (ret) {
		LOG_ERR("Reinitialization of nRF Modem library failed, retval: %d", ret);
		return ret;
	}

	LOG_DBG("nRF Modem Library reinitialized");

	return 0;
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

	net_if_dormant_off(iface_bound);

	k_work_cancel_delayable(&connection_timeout_work);
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
		 * deactivate LTE if the modem is not able to reconnect before the set timeout.
		 */
		connection_timeout_schedule();
	} else {
		/* If persistence is disabled, LTE is deactivated upon a lost connection.
		 * Re-establishment is reliant on the application calling conn_mgr_if_connect().
		 */
		ret = lte_connectivity_disconnect(NULL);
		if (ret) {
			LOG_ERR("lte_lc_func_mode_set, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
			return;
		}
	}
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

/* Public APIs */
void lte_connectivity_init(struct conn_mgr_conn_binding *if_conn)
{
	int ret;
	int timeout = CONFIG_LTE_CONNECTIVITY_CONNECT_TIMEOUT_SECONDS;
	int persistent = IS_ENABLED(CONFIG_LTE_CONNECTIVITY_CONNECTION_PERSISTENCE);

	net_if_dormant_on(if_conn->iface);

	/* Default auto features off. */
	if (!IS_ENABLED(CONFIG_LTE_CONNECTIVITY_AUTO_CONNECT)) {
		ret = conn_mgr_if_set_flag(if_conn->iface, CONN_MGR_IF_NO_AUTO_CONNECT, true);
		if (ret) {
			LOG_ERR("conn_mgr_if_set_flag, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, if_conn->iface);
			return;
		}
	}

	if (!IS_ENABLED(CONFIG_LTE_CONNECTIVITY_AUTO_DOWN)) {
		ret = conn_mgr_if_set_flag(if_conn->iface, CONN_MGR_IF_NO_AUTO_DOWN, true);
		if (ret) {
			LOG_ERR("conn_mgr_if_set_flag, error: %d", ret);
			net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, if_conn->iface);
			return;
		}
	}

	/* Set default values for connection persistence and timeout */
	if_conn->timeout = (timeout > CONN_MGR_IF_NO_TIMEOUT) ? timeout : CONN_MGR_IF_NO_TIMEOUT;
	if_conn->flags |= (persistent == 1) ? BIT(CONN_MGR_IF_PERSISTENT) : 0;

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

	/* Keep local reference to the network interface that the connectivity layer is bound to. */
	iface_bound = if_conn->iface;

	LOG_DBG("LTE Connectivity library initialized");
}

int lte_connectivity_enable(void)
{
	int ret;

	if (!nrf_modem_is_initialized()) {
		ret = modem_init();
		if (ret) {
			LOG_ERR("modem_init, error: %d", ret);
			return ret;
		}
	}

	/* Calling this function if the link controller has already been initialized is safe,
	 * as it will return 0 in that case.
	 */
	ret = lte_lc_init();
	if (ret) {
		LOG_ERR("lte_lc_init, error: %d", ret);
		return ret;
	}

	return 0;
}

int lte_connectivity_disable(void)
{
	int ret;

	if (net_if_down_options == LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN) {
		ret = nrf_modem_lib_shutdown();
		if (ret) {
			LOG_ERR("nrf_modem_lib_shutdown, retval: %d", ret);
			return ret;
		}
	} else if (net_if_down_options == LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT) {
		ret = lte_connectivity_disconnect(NULL);
		if (ret) {
			LOG_ERR("lte_connectivity_disconnect, retval: %d", ret);
			return ret;
		}
	} else {
		LOG_ERR("Unsupported option");
		return -ENOTSUP;
	}

	return 0;
}

int lte_connectivity_connect(struct conn_mgr_conn_binding *const if_conn)
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

int lte_connectivity_disconnect(struct conn_mgr_conn_binding *const if_conn)
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

int lte_connectivity_options_set(struct conn_mgr_conn_binding *const if_conn, int name,
				 const void *value, size_t length)
{
	ARG_UNUSED(name);

	uint8_t *temp;

	if (name != LTE_CONNECTIVITY_IF_DOWN) {
		return -ENOPROTOOPT;
	}

	if (length > sizeof(uint8_t)) {
		return -ENOBUFS;
	}

	temp = (uint8_t *)value;
	net_if_down_options = *temp;

	switch (net_if_down_options) {
	case LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN:
		LOG_DBG("Shutdown modem when the network interface is brought down");
		break;
	case LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT:
		LOG_DBG("Disconnected from LTE when the network interface is brought down");
		break;
	default:
		LOG_ERR("Unsupported option");
		return -EBADF;
	}

	return 0;
}

int lte_connectivity_options_get(struct conn_mgr_conn_binding *const if_conn, int name, void *value,
				 size_t *length)
{
	ARG_UNUSED(name);

	uint8_t *temp = (uint8_t *)value;

	if (name != LTE_CONNECTIVITY_IF_DOWN) {
		return -ENOPROTOOPT;
	}

	*temp = net_if_down_options;
	*length = sizeof(net_if_down_options);

	return 0;
}
