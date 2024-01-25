/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/nrf_provisioning.h>

#include "cloud_connection.h"
#include "sample_reboot.h"

LOG_MODULE_REGISTER(cloud_provisioning, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define NETWORK_UP			BIT(0)
#define NETWORK_DOWN			BIT(1)
#define PROVISIONING_IDLE		BIT(2)

static K_EVENT_DEFINE(prov_events);

static bool provisioning_started;

/* Called by the provisioning library to request the LTE modem be taken offline before credential
 * installation, and then again after credential installation is complete. This is necessary because
 * credentials cannot be installed to the modem while it is online.
 *
 * This limitation only applies to the nrf91 modem; Wi-Fi, for instance, is unaffected. But for
 * simplicity we use conn_mgr to take take all network interfaces offline and online.
 */
static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;

	ARG_UNUSED(user_data);

	/* The nrf_provisioning library requires us to return the previous functional mode. */
	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		return -EFAULT;
	}

	if (new_mode == LTE_LC_FUNC_MODE_NORMAL || new_mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		LOG_INF("Provisioning library requests normal mode");

		/* Connect all interfaces since we are done provisioning. */
		conn_mgr_all_if_connect(true);

		/* Wait for network readiness to be re-established before returning. */
		LOG_DBG("Waiting for network up");
		k_event_wait(&prov_events, NETWORK_UP, false, K_FOREVER);

		LOG_DBG("Network is up.");
	} else if (new_mode == LTE_LC_FUNC_MODE_OFFLINE ||
		   new_mode == LTE_LC_FUNC_MODE_DEACTIVATE_LTE) {
		LOG_INF("Provisioning library requests offline mode");
		/* Disconnect all interfaces to allow cert installation. */
		conn_mgr_all_if_disconnect(true);

		/* Wait for network disconnection before returning. */
		LOG_DBG("Waiting for network down.");
		k_event_wait(&prov_events, NETWORK_DOWN, false, K_FOREVER);

		LOG_DBG("Network is down.");
	}

	return fmode;
}

/* Delayable work item for marking provisioning as idle. */
static void mark_provisioning_idle_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Provisioning is idle.");
	k_event_post(&prov_events, PROVISIONING_IDLE);
}

static K_WORK_DELAYABLE_DEFINE(provisioning_idle_work, mark_provisioning_idle_work_fn);

/* Called by the provisioning library when each provisioning attempt starts, stops, or finishes
 * with reboot required.
 */
static void device_mode_cb(enum nrf_provisioning_event event, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event) {
	case NRF_PROVISIONING_EVENT_START:
		/* Called when the provisioning library begins checking for provisioning commands.
		 */
		LOG_DBG("NRF_PROVISIONING_EVENT_START");

		/* Mark provisioning as active. */
		LOG_INF("Provisioning is active.");
		k_event_clear(&prov_events, PROVISIONING_IDLE);
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		/* Called when the current provisioning command check has completed. */
		LOG_DBG("NRF_PROVISIONING_EVENT_STOP");

		/* Mark provisioning as idle after a small delay.
		 * This delay is to prevent false starts if the provisioning library performs an
		 * immediate retry.
		 */
		k_work_reschedule(&provisioning_idle_work, K_SECONDS(5));
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		/* Called when there are no further provisioning commands,
		 * and a reboot is needed.
		 */
		LOG_DBG("NRF_PROVISIONING_EVENT_DONE");

		LOG_INF("Provisioning completed.");
		sample_reboot_normal();
		break;
	default:
		LOG_ERR("Unknown event: %d", event);
		break;
	}
}

static struct nrf_provisioning_mm_change mmode = { .cb = modem_mode_cb };
static struct nrf_provisioning_dm_change dmode = { .cb = device_mode_cb };

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
static struct net_mgmt_event_callback l4_callback;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		/* Mark network as up. */
		k_event_clear(&prov_events, NETWORK_DOWN);
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
	} else if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Mark network as down. */
		k_event_clear(&prov_events, NETWORK_UP);
		k_event_post(&prov_events, NETWORK_DOWN);
	}
}

/* Set up any requirements for provisioning on boot */
static int prepare_provisioning(void)
{
	/* Start tracking network availability */
	net_mgmt_init_event_callback(
		&l4_callback, l4_event_handler, NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED
	);
	net_mgmt_add_event_callback(&l4_callback);
	return 0;
}

SYS_INIT(prepare_provisioning, APPLICATION, 0);

bool await_provisioning_idle(k_timeout_t timeout)
{
	return k_event_wait(&prov_events, PROVISIONING_IDLE, false, timeout) != 0;
}
