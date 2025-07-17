/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include <modem/lte_lc.h>
#include <net/nrf_provisioning.h>
#include "nrf_provisioning_at.h"

LOG_MODULE_REGISTER(nrf_provisioning_sample, CONFIG_NRF_PROVISIONING_SAMPLE_LOG_LEVEL);

#define NETWORK_UP			    BIT(0)
#define PROVISIONING_IDLE		BIT(1)
#define CONN_TIMEOUT_S			5
#define NORMAL_REBOOT_S		    10
#define PROVISIONING_IDLE_DELAY_S	3

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

static struct net_mgmt_event_callback l4_callback;
static struct net_mgmt_event_callback conn_callback;

static K_EVENT_DEFINE(prov_events);

static bool provisioning_started;

static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;
	uint32_t events = 0;
	ARG_UNUSED(user_data);

	/* The nrf_provisioning library requires us to return the previous functional mode. */
	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		return -EFAULT;
	}

	if (new_mode == LTE_LC_FUNC_MODE_NORMAL || new_mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		LOG_INF("Provisioning library requests normal mode");

		/* Reactivate LTE */
		conn_mgr_all_if_connect(true);

		/* Wait for network readiness to be re-established before returning. */
		LOG_DBG("Waiting for network up");
		events = k_event_wait(&prov_events, NETWORK_UP, false, K_SECONDS(CONN_TIMEOUT_S));
		if ((events & NETWORK_UP) == 0) {
			LOG_WRN("Timeout waiting for network up event");
		} else {
			LOG_DBG("Network is up.");
		}
	}

	if (new_mode == LTE_LC_FUNC_MODE_OFFLINE || new_mode == LTE_LC_FUNC_MODE_DEACTIVATE_LTE) {
		LOG_INF("Provisioning library requests offline mode");

		/* The provisioning library wants to install or generate credentials.
		 * Deactivate LTE to allow this.
		 */
		conn_mgr_all_if_disconnect(true);

		LOG_DBG("Network is down.");
	}

	return fmode;
}

static void reboot_device(void)
{
	/* Disconnect from network gracefully */
	conn_mgr_all_if_disconnect(true);

	/* We must do this before we reboot, otherwise we might trip LTE boot loop
	 * prevention, and the modem will be temporarily disable itself.
	 */
	(void)lte_lc_power_off();

	LOG_PANIC();
	k_sleep(K_SECONDS(NORMAL_REBOOT_S));
	sys_reboot(SYS_REBOOT_COLD);
}

/* Delayable work item for marking provisioning as idle. */
static void mark_provisioning_idle_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Provisioning is idle.");
	k_event_set(&prov_events, PROVISIONING_IDLE);
}

static K_WORK_DELAYABLE_DEFINE(provisioning_idle_work, mark_provisioning_idle_work_fn);

static void device_mode_cb(enum nrf_provisioning_event event, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event) {
	case NRF_PROVISIONING_EVENT_START:
		LOG_INF("Provisioning started");
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		/* Mark provisioning as idle after a small delay.
		 * This delay is to prevent false starts if the provisioning library performs an
		 * immediate retry.
		 */
		k_work_reschedule(&provisioning_idle_work, K_SECONDS(PROVISIONING_IDLE_DELAY_S));
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		LOG_INF("Provisioning done, rebooting...");
		reboot_device();
		break;
	default:
		LOG_ERR("Unknown event");
		break;
	}
}

static struct nrf_provisioning_mm_change mmode = { .cb = modem_mode_cb, .user_data = NULL };
static struct nrf_provisioning_dm_change dmode = { .cb = device_mode_cb, .user_data = NULL };

/* Work item to initialize the provisioning library and start checking for provisioning commands.
 * Called automatically the first time network connectivity is established.
 * Needs to be a work item since nrf_provisioning_init may attempt to install certs in a blocking
 * fashion.
 */
static void start_provisioning_work_fn(struct k_work *work)
{
	LOG_INF("Initializing the nRF Provisioning library...");

	int ret = nrf_provisioning_init(&mmode, &dmode);

	if (ret) {
		LOG_ERR("Failed to initialize provisioning client, error: %d", ret);
	}
}

static K_WORK_DEFINE(start_provisioning_work, start_provisioning_work_fn);

/* Callback to track network connectivity */
static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event, struct net_if *iface)
{
	if ((event & L4_EVENT_MASK) != event) {
		return;
	}

	if (event == NET_EVENT_L4_CONNECTED) {
		/* Mark network as up. */
		LOG_INF("Network connectivity gained!");
		k_event_post(&prov_events, NETWORK_UP);

		/* Start the provisioning library after network readiness is first established.
		 * We offload this to a workqueue item to avoid a deadlock.
		 * (nrf_provisioning_init might attempt to install certs, and in the process,
		 * trigger a blocking wait for L4_DOWN, which cannot fire until this handler exits.)
		 */
		if (!provisioning_started) {
			k_work_submit(&start_provisioning_work);
			provisioning_started = true;
		}
	}

	if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Mark network as down. */
		LOG_INF("Network connectivity lost!");
		k_event_clear(&prov_events, NETWORK_UP);
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
									uint32_t event,
									struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

int main(void)
{
	int ret;
	LOG_INF("nRF Device Provisioning Sample");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_callback, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_callback);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_callback, connectivity_event_handler,
								CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_callback);

	LOG_INF("Bringing network interface up and connecting to the network");

	ret = conn_mgr_all_if_up(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", ret);
		return ret;
	}

	ret = conn_mgr_all_if_connect(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", ret);
		return ret;
	}

	return 0;
}
