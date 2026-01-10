/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>
#include <modem/ntn.h>
#include <date_time.h>

#include "lte_ip_addr_helper.h"
#include "lte_cell_prfl_helper.h"

LOG_MODULE_REGISTER(nrf_modem_lib_netif_ntn, CONFIG_NRF_MODEM_LIB_NET_IF_NTN_LOG_LEVEL);

/* Forward declarations */
static void connection_timeout_work_fn(struct k_work *work);
static void connection_timeout_schedule(void);
static void modem_fault_work_fn(struct k_work *work);
static void deregister_handlers_work_fn(struct k_work *work);

/* Delayable work used to handle LTE connection timeouts. */
static K_WORK_DELAYABLE_DEFINE(connection_timeout_work, connection_timeout_work_fn);

/* Work item for handling modem faults. */
static K_WORK_DEFINE(modem_fault_handler_work, modem_fault_work_fn);

/* Work item for deregistering handlers. */
static K_WORK_DEFINE(deregister_handlers_work, deregister_handlers_work_fn);

static int lte_net_if_disconnect_ntn(struct conn_mgr_conn_binding *const if_conn);

/* Local reference to the network interface the L2 connection layer is bound to. */
static struct net_if *iface_bound;

/* Container and protection mutex for internal state of the connectivity binding. */
static struct {
	/* Tracks whether a PDN bearer is currently active. */
	bool has_pdn;

	/* Tracks whether a serving cell is currently or was recently available. */
	bool has_cell;
} internal_state;

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static struct k_work gnss_location_work;

static void gnss_event_handler(int event);

static K_MUTEX_DEFINE(internal_state_lock);
static K_SEM_DEFINE(gnss_fix_sem, 0, 1);

/* Local functions */

static void apply_gnss_time(const struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	struct tm gnss_time = {
		.tm_year = pvt_data->datetime.year - 1900,
		.tm_mon = pvt_data->datetime.month - 1,
		.tm_mday = pvt_data->datetime.day,
		.tm_hour = pvt_data->datetime.hour,
		.tm_min = pvt_data->datetime.minute,
		.tm_sec = pvt_data->datetime.seconds,
	};

	date_time_set(&gnss_time);
}

/* Function called whenever an irrecoverable error occurs and LTE is activated.
 * Notifies a fatal connectivity error and deactivates LTE.
 */
static void fatal_error_notify_and_disconnect(void)
{
	net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
	net_if_dormant_on(iface_bound);
	(void)lte_net_if_disconnect_ntn(NULL);
}

/* Handler called on modem faults. */
void lte_net_if_ntn_modem_fault_handler(void)
{
	/* The net_mgmt event notification cannot be handled in interrupt context, so we
	 * defer it to the system workqueue.
	 */
	k_work_submit(&modem_fault_handler_work);
}

static void modem_fault_work_fn(struct k_work *work)
{
	net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, iface_bound);
	net_if_dormant_on(iface_bound);
}

static uint16_t pdn_mtu(void)
{
	struct lte_lc_pdn_dynamic_info info;
	int rc;

	rc = lte_lc_pdn_dynamic_info_get(0, &info);
	if ((rc != 0) || (info.ipv4_mtu == 0)) {
		LOG_WRN("MTU query failed, error: %d, MTU: %d", rc, info.ipv4_mtu);
		/* Fallback to the minimum value that IPv4 is required to support */
		info.ipv4_mtu = NET_IPV4_MTU;
	}

	LOG_DBG("Network MTU: %d", info.ipv4_mtu);

	return info.ipv4_mtu;
}

/* Called when we detect LTE connectivity has been gained.
 *
 * Queries the network MTU, marks the iface as active, and cancels any pending timeout.
 */
static void become_active(void)
{
	LOG_DBG("Becoming active");
	net_if_set_mtu(iface_bound, pdn_mtu());
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
		ret = lte_net_if_disconnect_ntn(NULL);
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

	int ret = lte_net_if_disconnect_ntn(NULL);

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
#if CONFIG_NET_IPV4
	int ret;

	/* If IPv4 is enabled, check whether modem has been assigned an IPv4. */
	ret = lte_ipv4_addr_add_cid(iface_bound, 10);

	if (ret == -ENODATA) {
		LOG_WRN("No IPv4 address given by the network");
	} else if (ret) {
		LOG_ERR("lte_ipv4_addr_add_cid, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}

#endif /* CONFIG_NET_IPV4 */

	update_has_pdn(true);
}

static void on_pdn_deactivated(void)
{
	int ret;

#if CONFIG_NET_IPV4
	ret = lte_ipv4_addr_remove_cid(iface_bound, 10);
	if (ret) {
		LOG_ERR("ipv4_addr_remove, error: %d", ret);
		fatal_error_notify_and_disconnect();
		return;
	}
#endif /* CONFIG_NET_IPV4 */

	update_has_pdn(false);
}

static void gnss_location_work_handler(struct k_work *work)
{
	int err;
	struct nrf_modem_gnss_pvt_data_frame pvt_data;

	/* Read PVT data in thread context */
	err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
	if (err != 0) {
		LOG_ERR("Failed to read GNSS data nrf_modem_gnss_read(), err: %d", err);
		return;
	}

	if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		LOG_DBG("Got valid GNSS location: lat: %f, lon: %f, alt: %f",
		(double)pvt_data.latitude,
		(double)pvt_data.longitude,
		(double)pvt_data.altitude);
		memcpy(&last_pvt, &pvt_data, sizeof(last_pvt));
		apply_gnss_time(&last_pvt);
		k_sem_give(&gnss_fix_sem);
	}

	/* Log SV (Satellite Vehicle) data */
	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt_data.sv[i].sv == 0) {
			/* SV not valid, skip */
			continue;
		}

		LOG_INF("SV: %3d C/N0: %4.1f el: %2d az: %3d signal: %d in fix: %d unhealthy: %d",
		pvt_data.sv[i].sv,
		pvt_data.sv[i].cn0 * 0.1,
		pvt_data.sv[i].elevation,
		pvt_data.sv[i].azimuth,
		pvt_data.sv[i].signal,
		pvt_data.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
		pvt_data.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
	}
}

/* Event handlers */
static void gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		k_work_submit(&gnss_location_work);
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("NRF_MODEM_GNSS_EVT_FIX");
		break;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_WRN("NRF_MODEM_GNSS_EVT_BLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_ERR("NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		k_sem_give(&gnss_fix_sem);
		break;
	default:
		LOG_DBG("Unknown GNSS event: %d", event);
		break;
	}
}

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
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
		case LTE_LC_NW_REG_UICC_FAIL:
			LOG_WRN("The modem reports a UICC failure. Is SIM installed?");
			__fallthrough;
		default:
			LOG_DBG("Not registered to serving cell");
			/* Mark the serving cell as lost. */
			update_has_cell(false);
			break;
		}
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		if (evt->modem_evt.type == LTE_LC_MODEM_EVT_RESET_LOOP) {
			LOG_WRN("The modem has detected a reset loop. LTE network attach is now "
				"restricted for the next 30 minutes.");

			LOG_DBG("For more information, see the AT command documentation "
				"for the %%MDMEV notification");
		}

		break;
	case LTE_LC_EVT_RRC_UPDATE:
		if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
			LOG_INF("LTE_LC_RRC_MODE_CONNECTED");

		} else if (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			LOG_INF("LTE_LC_RRC_MODE_IDLE");
		}

		break;
	case LTE_LC_EVT_PDN:
		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ACTIVATED:
			LOG_DBG("PDN connection activated, cid: %d", evt->pdn.cid);
			on_pdn_activated();
			break;
		case LTE_LC_EVT_PDN_NETWORK_DETACH:
			LOG_DBG("PDN network detached, cid: %d", evt->pdn.cid);
			on_pdn_deactivated();
			break;
		case LTE_LC_EVT_PDN_DEACTIVATED:
			LOG_DBG("PDN connection deactivated, cid: %d", evt->pdn.cid);
			on_pdn_deactivated();
			break;
		case LTE_LC_EVT_PDN_SUSPENDED:
			LOG_DBG("PDN connection suspended, cid: %d", evt->pdn.cid);
			on_pdn_deactivated();
			break;
		case LTE_LC_EVT_PDN_RESUMED:
			LOG_DBG("PDN connection resumed, cid: %d", evt->pdn.cid);
			on_pdn_activated();
			break;
#if CONFIG_NET_IPV6
		case LTE_LC_EVT_PDN_IPV6_UP:
			LOG_DBG("PDN IPv6 up");
			on_pdn_ipv6_up();
			break;
		case LTE_LC_EVT_PDN_IPV6_DOWN:
			LOG_DBG("PDN IPv6 down");
			on_pdn_ipv6_down();
			break;
#endif /* CONFIG_NET_IPV6 */
		case LTE_LC_EVT_PDN_CTX_DESTROYED:
			LOG_DBG("PDN context destroyed");
			break;

#if CONFIG_LTE_LC_PDN_ESM_STRERROR
		case LTE_LC_EVT_PDN_ESM_ERROR:
			LOG_DBG("Event: PDP context %d, %s", evt->pdn.cid,
				lte_lc_pdn_esm_strerror(evt->pdn.esm_err));
			break;
#endif /* CONFIG_LTE_LC_PDN_ESM_STRERROR */
		default:
			break;
		}

		break;
	default:
		break;
	}
}

static int set_gnss_active_mode(void)
{
	int err;
	enum lte_lc_func_mode mode;

	err = lte_lc_func_mode_get(&mode);
	if (err) {
		LOG_ERR("Failed to get LTE function mode, error: %d", err);

		return err;
	}

	/* If needed, go offline to be able to set system mode */
	switch (mode) {
	case LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG:
		__fallthrough;
	case LTE_LC_FUNC_MODE_OFFLINE:
		__fallthrough;
	case LTE_LC_FUNC_MODE_POWER_OFF:
		break;
	default:
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		if (err) {
			LOG_ERR("lte_lc_func_mode_set, error: %d", err);

			return err;
		}

		break;
	}
	/* Configure GNSS system mode */
	err = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS,
				     LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	if (err) {
		LOG_ERR("Failed to set GNSS system mode, error: %d", err);

		return err;
	}

	/* Activate GNSS functional mode */
	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
	if (err) {
		LOG_ERR("Failed to activate GNSS fun mode, error: %d", err);

		return err;
	}

	/* Set GNSS to single fix mode */
	err = nrf_modem_gnss_fix_interval_set(0);
	if (err) {
		LOG_ERR("Failed to set GNSS fix interval, error: %d", err);
	}

	/* Set GNSS fix timeout to 180 seconds */
	err = nrf_modem_gnss_fix_retry_set(180);
	if (err) {
		LOG_ERR("Failed to set GNSS fix retry, error: %d", err);
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS, error: %d", err);
	}

	return 0;
}

static int set_gnss_inactive_mode(void)
{
	int err;

	err = nrf_modem_gnss_stop();
	if (err) {
		LOG_ERR("Failed to stop GNSS, error: %d", err);
	}

	/* Set modem to CFUN=30 mode when exiting GNSS state */
	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS);
	if (err) {
		LOG_ERR("lte_lc_func_mode_set, error: %d", err);
	}

	return 0;
}

static void set_ntn_active_mode(void)
{
	int err;
	enum lte_lc_func_mode mode;

	err = lte_lc_func_mode_get(&mode);
	if (err) {
		LOG_ERR("Failed to get LTE function mode, error: %d", err);

		return;
	}

	/* If needed, go offline to be able to set system mode */
	switch (mode) {
	case LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG:
		__fallthrough;
	case LTE_LC_FUNC_MODE_OFFLINE:
		__fallthrough;
	case LTE_LC_FUNC_MODE_POWER_OFF:
		break;
	default:
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		if (err) {
			LOG_ERR("lte_lc_func_mode_set, error: %d", err);

			return;
		}

		break;
	}

	/* Configure NTN system mode */
	err = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_NTN_NBIOT,
				     LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	if (err) {
		LOG_ERR("Failed to set NTN system mode, error: %d", err);

		return;
	}

	err = ntn_location_set((double)last_pvt.latitude,
				(double)last_pvt.longitude,
				(float)last_pvt.altitude,
				0);
	if (err) {
		LOG_ERR("Failed to set location, error: %d", err);

		return;
	}

	#if defined(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_BANDLOCK_ENABLE)
	err = nrf_modem_at_printf("AT%%XBANDLOCK=2,,\"%i\"",
					CONFIG_NRF_MODEM_LIB_NET_IF_NTN_BANDLOCK);
	if (err) {
		LOG_ERR("Failed to set NTN band lock, error: %d", err);

		return;
	}
	#endif
	#if defined(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_CHANNEL_SELECT_ENABLE)
	err = nrf_modem_at_printf("AT%%CHSELECT=1,14,%i",
			CONFIG_NRF_MODEM_LIB_NET_IF_NTN_CHANNEL_SELECT);
	if (err) {
		LOG_ERR("Failed to set NTN channel lock, error: %d", err);

		return;
	}
	#endif
}

static int set_ntn_offline_mode(void)
{
	int err;

	LOG_DBG("Going offline while keeping NTN registration context");

	/* Set modem to offline mode without losing registration */
	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG);
	if (err) {
		LOG_ERR("lte_lc_func_mode_set, error: %d", err);

		return err;
	}

	return 0;
}

static void lte_net_if_init_ntn(struct conn_mgr_conn_binding *if_conn)
{
	int err;
	int timeout = CONFIG_NRF_MODEM_LIB_NET_IF_NTN_CONNECT_TIMEOUT_SECONDS;

	net_if_dormant_on(if_conn->iface);

	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_AUTO_CONNECT)) {
		conn_mgr_binding_set_flag(if_conn, CONN_MGR_IF_NO_AUTO_CONNECT, true);
	}

	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_AUTO_DOWN)) {
		conn_mgr_binding_set_flag(if_conn, CONN_MGR_IF_NO_AUTO_DOWN, true);
	}

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_CONNECTION_PERSISTENCE)) {
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

	err = lte_lc_pdn_default_ctx_events_enable();
	if (err) {
		LOG_ERR("lte_lc_pdn_default_ctx_events_enable, error: %d", err);
		net_mgmt_event_notify(NET_EVENT_CONN_IF_FATAL_ERROR, if_conn->iface);

		return;
	}

	/* Keep local reference to the network interface that the connectivity layer is bound to. */
	iface_bound = if_conn->iface;

	k_work_init(&gnss_location_work, gnss_location_work_handler);

	LOG_DBG("NTN Modem network interface ready");
}

int lte_net_if_enable_ntn(void)
{
	int err;

	if (!nrf_modem_is_initialized()) {
		err = nrf_modem_lib_init();
		if (err) {
			LOG_ERR("Failed to initialize nrf_modem_lib, error: %d", err);

			return err;
		}
	}

	nrf_modem_gnss_event_handler_set(gnss_event_handler);

	cell_prfl_set_profiles();

	return 0;
}

int lte_net_if_disable_ntn(void)
{
	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_DOWN_DEFAULT_LTE_DISCONNECT)) {
		return lte_net_if_disconnect_ntn(NULL);
	} else {
		return nrf_modem_lib_shutdown();
	}

	return 0;

}

static int lte_net_if_connect_ntn(struct conn_mgr_conn_binding *const if_conn)
{
	int err;

	ARG_UNUSED(if_conn);

	LOG_DBG("Looking for GNSS fix");

	set_gnss_active_mode();

	/* Wait for valid GNSS fix with 3-minute timeout */
	if (k_sem_take(&gnss_fix_sem, K_FOREVER)) {
		LOG_ERR("GNSS timeout");
		set_gnss_inactive_mode();
		lte_net_if_disconnect_ntn(NULL);
		return 0;
	}

	set_gnss_inactive_mode();

	LOG_DBG("Connecting to NTN");

	/* Register handler for registration status notifications */
	lte_lc_register_handler(lte_evt_handler);

	set_ntn_active_mode();

	err = lte_lc_modem_events_enable();
	if (err) {
		LOG_WRN("lte_lc_modem_events_enable, error: %d", err);
	}

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	if (err) {
		LOG_ERR("lte_lc_func_mode_set, error: %d", err);
		return err;
	}

	connection_timeout_schedule();

	return 0;
}

static void deregister_handlers_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	lte_lc_deregister_handler(lte_evt_handler);
}

static int lte_net_if_disconnect_ntn(struct conn_mgr_conn_binding *const if_conn)
{
	ARG_UNUSED(if_conn);

	LOG_DBG("Disconnecting from NTN");

	k_work_cancel_delayable(&connection_timeout_work);

	set_ntn_offline_mode();

	/* Sleep shortly to allow events to propagate to handlers*/
	k_sleep(K_SECONDS(2));

	/* Submit work to deregister handlers */
	k_work_submit(&deregister_handlers_work);

	return 0;
}

/* Bind connectivity APIs for NTN.
 * extern in nrf9x_sockets.c
 */
struct conn_mgr_conn_api lte_net_if_conn_mgr_api_ntn = {
	.init = lte_net_if_init_ntn,
	.connect = lte_net_if_connect_ntn,
	.disconnect = lte_net_if_disconnect_ntn,
};
