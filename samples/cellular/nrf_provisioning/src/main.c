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

#define NETWORK_UP			BIT(0)
#define NETWORK_DOWN			BIT(1)
#define PROVISIONING_IDLE		BIT(2)
#define NETWORK_IPV4_CONNECTED		BIT(3)
#define NETWORK_IPV6_CONNECTED		BIT(4)
#define NORMAL_REBOOT_S		    10
#define PROVISIONING_IDLE_DELAY_S	3
#define EVENT_MASK                                                                                 \
	(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | NET_EVENT_L4_IPV6_CONNECTED |        \
	 NET_EVENT_L4_IPV4_CONNECTED)

static K_EVENT_DEFINE(prov_events);

static bool provisioning_started;

static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;

	ARG_UNUSED(user_data);

	/* The nrf_provisioning library requires us to return the previous functional mode. */
	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		return -EFAULT;
	}

	if (new_mode == fmode) {
		LOG_DBG("Functional mode unchanged: %d", fmode);
		return fmode;
	}

	if (new_mode == LTE_LC_FUNC_MODE_NORMAL || new_mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		LOG_INF("Provisioning library requests normal mode");

		/* Reactivate LTE */
		conn_mgr_all_if_connect(true);

		/* Wait for network readiness to be re-established before returning. */
		LOG_DBG("Waiting for network up");
		k_event_wait(&prov_events, NETWORK_IPV4_CONNECTED, false, K_FOREVER);
		/* Wait extra 2 seconds for IPv6 negotiation */
		k_event_wait(&prov_events, NETWORK_IPV6_CONNECTED, false, K_SECONDS(2));

		LOG_DBG("Network is up.");
	}

	if (new_mode == LTE_LC_FUNC_MODE_OFFLINE || new_mode == LTE_LC_FUNC_MODE_DEACTIVATE_LTE) {
		LOG_INF("Provisioning library requests offline mode");

		/* The provisioning library wants to install or generate credentials.
		 * Deactivate LTE to allow this.
		 */
		conn_mgr_all_if_disconnect(true);

		/* Note:
		 * You could shut down both LTE by using
		 * lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		 * But conn_mgr will interpret this as an unintended connectivity loss.
		 * Using conn_mgr_all_if_disconnect lets conn_mgr know the LTE loss is intentional.
		 */

		/* Wait for network disconnection before returning. */
		LOG_DBG("Waiting for network down.");
		k_event_wait(&prov_events, NETWORK_DOWN, false, K_FOREVER);

		LOG_DBG("Network is down.");

		/* conn_mgr disables only LTE connectivity. GNSS may remain active.
		 * Ensure GNSS is also disabled before storing credentials to the modem.
		 */
		enum lte_lc_func_mode current_mode;

		if (lte_lc_func_mode_get(&current_mode)) {
			LOG_ERR("Failed to read modem functional mode");
			return -EFAULT;
		}
		if (current_mode == LTE_LC_FUNC_MODE_ACTIVATE_GNSS) {
			LOG_WRN("GNSS is still active. Deactivating GNSS.");
			if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS)) {
				LOG_ERR("Failed to deactivate GNSS");
				return -EFAULT;
			}
		}
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
	k_event_post(&prov_events, PROVISIONING_IDLE);
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

/* Callback to track network connectivity */
static struct net_mgmt_event_callback l4_callback;
static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event, struct net_if *iface)
{
	if ((event & EVENT_MASK) != event) {
		return;
	}

	if (event == NET_EVENT_L4_IPV4_CONNECTED) {
		/* Mark network as up. */
		LOG_INF("IPv4 connectivity gained!");
		k_event_clear(&prov_events, NETWORK_DOWN);
		k_event_post(&prov_events, NETWORK_IPV4_CONNECTED);
	} else if (event == NET_EVENT_L4_IPV6_CONNECTED) {
		/* Mark network as up. */
		LOG_INF("IPv6 connectivity gained!");
		k_event_clear(&prov_events, NETWORK_DOWN);
		k_event_post(&prov_events, NETWORK_IPV6_CONNECTED);
	} else if (event == NET_EVENT_L4_CONNECTED) {
		/* Mark network as up. */
		LOG_INF("Network connectivity gained!");
		k_event_clear(&prov_events, NETWORK_DOWN);
		k_event_post(&prov_events, NETWORK_UP);

		/* Start the provisioning library after network readiness is first established. */
		if (!provisioning_started) {
			LOG_INF("Initializing the nRF Provisioning library...");

			int ret = nrf_provisioning_init(&mmode, &dmode);

			if (ret) {
				LOG_ERR("Failed to initialize provisioning client, error: %d", ret);
			}
			provisioning_started = true;
		}
	}

	if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Mark network as down. */
		LOG_INF("Network connectivity lost!");
		k_event_clear(&prov_events,
			      NETWORK_UP | NETWORK_IPV4_CONNECTED | NETWORK_IPV6_CONNECTED);
		k_event_post(&prov_events, NETWORK_DOWN);
	}
}

/* Set up any requirements for provisioning on boot */
static int prepare_provisioning(void)
{
	/* Start tracking network availability */
	net_mgmt_init_event_callback(&l4_callback, l4_event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&l4_callback);
	return 0;
}

SYS_INIT(prepare_provisioning, APPLICATION, 0);

int main(void)
{
	LOG_INF("nRF Device Provisioning Sample");

	LOG_INF("Enabling connectivity...");
	conn_mgr_all_if_connect(true);

	return 0;
}
