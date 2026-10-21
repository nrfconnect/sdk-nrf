/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT NR+ RPC sample: client or server role (see prj.conf / server.conf).
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if defined(CONFIG_DECT_NR_RPC_SERVER)

#include <zephyr/net/net_core.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_rpc.h>

static void rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("RPC error: %d", report->code);
}

static const char *modem_fault_reason_str(uint32_t reason)
{
	switch (reason) {
	case NRF_MODEM_FAULT_UNDEFINED:
		return "Undefined fault";
	case NRF_MODEM_FAULT_HW_WD_RESET:
		return "HW WD reset";
	case NRF_MODEM_FAULT_HARDFAULT:
		return "Hard fault";
	case NRF_MODEM_FAULT_MEM_MANAGE:
		return "Memory management fault";
	case NRF_MODEM_FAULT_BUS:
		return "Bus fault";
	case NRF_MODEM_FAULT_USAGE:
		return "Usage fault";
	case NRF_MODEM_FAULT_SECURE_RESET:
		return "Secure control reset";
	case NRF_MODEM_FAULT_PANIC_DOUBLE:
		return "Error handler crash";
	case NRF_MODEM_FAULT_PANIC_RESET_LOOP:
		return "Reset loop";
	case NRF_MODEM_FAULT_ASSERT:
		return "Assert";
	case NRF_MODEM_FAULT_PANIC:
		return "Unconditional SW reset";
	case NRF_MODEM_FAULT_FLASH_ERASE:
		return "Flash erase fault";
	case NRF_MODEM_FAULT_FLASH_WRITE:
		return "Flash write fault";
	case NRF_MODEM_FAULT_POFWARN:
		return "Undervoltage fault";
	case NRF_MODEM_FAULT_THWARN:
		return "Overtemperature fault";
	default:
		return "Unknown reason";
	}
}

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem fault: reason=0x%x (%s), PC=0x%x",
		fault_info->reason,
		modem_fault_reason_str(fault_info->reason),
		fault_info->program_counter);

	__ASSERT(false, "Modem crash detected, halting application");
}

int main(void)
{
	int err;

	LOG_INF("DECT RPC server running. DECT interface receives/sends via RPC.");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("nrf_modem_lib_init failed: %d", err);
		return err;
	}
	LOG_INF("nRF Modem library initialized");

	LOG_INF("Initializing RPC...");
	err = nrf_rpc_init(rpc_err_handler);
	if (err != 0) {
		LOG_ERR("nrf_rpc_init failed: %d", err);
		return err;
	}
	LOG_INF("RPC initialized");

	return 0;
}

#elif defined(CONFIG_DECT_NR_RPC_CLIENT)

#include <nrf_rpc.h>

#include <net/dect/dect_rpc.h>

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#endif

#if !defined(CONFIG_NRF_RPC_INIT)
static void rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("RPC error: %d", report->code);
}

static K_SEM_DEFINE(rpc_bound_sem, 0, 1);

static void bound_handler(const struct nrf_rpc_group *group)
{
	if (group == dect_rpc_client_get_group()) {
		k_sem_give(&rpc_bound_sem);
	}
}
#endif

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
static struct net_if *rpc_client_iface;
static struct net_mgmt_event_callback rpc_if_evt_cb;
static struct net_mgmt_event_callback rpc_l4_evt_cb;

static void rpc_if_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				 struct net_if *iface)
{
	if (iface != rpc_client_iface) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		LOG_INF("NET_EVENT_IF_UP");
		break;
	case NET_EVENT_IF_DOWN:
		LOG_INF("NET_EVENT_IF_DOWN");
		break;
	case NET_EVENT_IF_ADMIN_UP:
		LOG_INF("NET_EVENT_IF_ADMIN_UP");
		break;
	case NET_EVENT_IF_ADMIN_DOWN:
		LOG_INF("NET_EVENT_IF_ADMIN_DOWN");
		break;
	default:
		break;
	}
}

static void rpc_l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				 struct net_if *iface)
{
	if (iface != rpc_client_iface) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("NET_EVENT_L4_CONNECTED");
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("NET_EVENT_L4_DISCONNECTED");
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		LOG_INF("NET_EVENT_L4_IPV6_CONNECTED");
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		LOG_INF("NET_EVENT_L4_IPV6_DISCONNECTED");
		break;
	default:
		break;
	}
}

static void rpc_client_net_events_register(void)
{
	int idx = net_if_get_by_name(CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME);

	if (idx < 0) {
		return;
	}

	rpc_client_iface = net_if_get_by_index(idx);

	net_mgmt_init_event_callback(&rpc_if_evt_cb, rpc_if_event_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN | NET_EVENT_IF_ADMIN_UP |
					     NET_EVENT_IF_ADMIN_DOWN);
	net_mgmt_add_event_callback(&rpc_if_evt_cb);

	net_mgmt_init_event_callback(&rpc_l4_evt_cb, rpc_l4_event_handler,
				     NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED |
					     NET_EVENT_L4_IPV6_CONNECTED |
					     NET_EVENT_L4_IPV6_DISCONNECTED);
	net_mgmt_add_event_callback(&rpc_l4_evt_cb);
}
#endif /* CONFIG_DECT_NR_RPC_CONN_MGR */

int main(void)
{
#if !defined(CONFIG_NRF_RPC_INIT)
	nrf_rpc_set_bound_handler(bound_handler);
	LOG_INF("Initializing RPC...");
	int err;

	err = nrf_rpc_init(rpc_err_handler);
	if (err != 0) {
		LOG_ERR("nrf_rpc_init failed: %d", err);
		return err;
	}
	LOG_INF("Waiting for server... (start server now)");
	if (k_sem_take(&rpc_bound_sem, K_MSEC(15000)) != 0) {
		LOG_ERR("Timeout waiting for RPC group (server not connected?)");
		return -ETIMEDOUT;
	}
	LOG_INF("RPC initialized (server ready)");
	dect_rpc_client_notify_rpc_init_done();
#endif

	if (net_if_get_by_name(CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME) < 0) {
		LOG_ERR("No DECT RPC interface named \"%s\" (CONFIG_NET_INTERFACE_NAME?)",
			CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME);
		return -ENOENT;
	}

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
	rpc_client_net_events_register();
#endif

#if defined(CONFIG_DECT_NR_RPC_AUTO_SYNC)
	LOG_INF("DECT RPC client running. %s starts admin-down; after RPC init, auto-sync "
		"pulls addresses from the server and brings the iface up (~%d s, or run "
		"dect sync)",
		CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME, CONFIG_DECT_NR_RPC_AUTO_SYNC_DELAY_SEC);
#else
	LOG_INF("DECT RPC client running. %s uses NO_AUTO_START; run dect sync then "
		"net if up %s",
		CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME, CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME);
#endif
#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
	LOG_INF("Use dect connect / dect disconnect for DECT association on the server");
#endif
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
	LOG_INF("mDNS name: %s.local (resolve from server or other hosts on link)",
		CONFIG_NET_HOSTNAME);
#endif

	return 0;
}

#else
#error "Build with default prj.conf (client) or EXTRA_CONF_FILE=server.conf"
#endif
