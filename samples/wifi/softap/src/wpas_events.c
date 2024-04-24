/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_event.h>
#include <ctrl_iface_zephyr.h>
#include <supp_events.h>
#include <supp_main.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(wpas_event, CONFIG_LOG_DEFAULT_LEVEL);

#define WPAS_READY_TIMEOUT_MS 500

#define WPA_SUPP_EVENTS (NET_EVENT_WPA_SUPP_READY)

static struct net_mgmt_event_callback net_wpa_supp_cb;
extern struct k_work sap_init_work;
K_SEM_DEFINE(wpa_supp_ready_sem, 0, 1);

static void handle_wpa_supp_ready(struct net_mgmt_event_callback *cb)
{
	k_sem_give(&wpa_supp_ready_sem);
	k_work_submit(&sap_init_work);
}

static void wpa_supp_event_handler(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WPA_SUPP_READY:
		handle_wpa_supp_ready(cb);
		break;
	default:
		LOG_DBG("Unhandled event (%d)", mgmt_event);
		break;
	}
}

int wait_for_wpa_s_ready(void)
{
	const struct device *dev = NULL;
	struct wpa_supplicant *wpa_s;
	int ret;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	if (!dev) {
		wpa_printf(MSG_ERROR, "Device instance not found");
		return -1;
	}

	wpa_s = z_wpas_get_handle_by_ifname(dev->name);
	if (wpa_s) {
		LOG_INF("WPA Supplicant is already ready: %s", dev->name);
		return 0;
	}

	ret = k_sem_take(&wpa_supp_ready_sem, K_MSEC(WPAS_READY_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_INF("Timeout waiting for WPA Supplicant to be ready (%d ms)",
			WPAS_READY_TIMEOUT_MS);
		return -1;
	}

	wpa_s = z_wpas_get_handle_by_ifname(dev->name);
	if (!wpa_s) {
		LOG_INF("WPA Supplicant ready event received, but no handle found for %s",
			 dev->name);
		return -1;
	}

	LOG_INF("WPA Supplicant is ready: %s", dev->name);

	return 0;
}



int wpa_supp_events_register(void)
{
	net_mgmt_init_event_callback(&net_wpa_supp_cb, wpa_supp_event_handler, WPA_SUPP_EVENTS);
	net_mgmt_add_event_callback(&net_wpa_supp_cb);

	return 0;
}
