/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>

#include <stdio.h>
#include <stdlib.h>

#include <net/wifi_ready.h>
#include <supp_events.h>
#include <zephyr/net/wifi_nm.h>

LOG_MODULE_REGISTER(wifi_ready, CONFIG_WIFI_READY_LIB_LOG_LEVEL);

static wifi_ready_callback_t wifi_ready_callbacks[CONFIG_WIFI_READY_MAX_CALLBACKS];
static unsigned char callback_count;
static K_MUTEX_DEFINE(wifi_ready_mutex);

#define WPA_SUPP_EVENTS (NET_EVENT_SUPPLICANT_READY) | \
			(NET_EVENT_SUPPLICANT_NOT_READY)

static struct net_mgmt_event_callback net_wpa_supp_cb;

/* In case Wi-Fi is already ready, call the callbacks */
static void wifi_ready_work_handler(struct k_work *item);

static void call_wifi_ready_callbacks(bool ready, struct net_if *iface)
{
	int i;

	k_mutex_lock(&wifi_ready_mutex, K_FOREVER);
	for (i = 0; i < ARRAY_SIZE(wifi_ready_callbacks); i++) {
		if (wifi_ready_callbacks[i].wifi_ready_cb &&
			((wifi_ready_callbacks[i].iface &&
			(wifi_ready_callbacks[i].iface == iface)) ||
		    !wifi_ready_callbacks[i].iface)) {
			wifi_ready_callbacks[i].wifi_ready_cb(ready);
		}
	}
	k_mutex_unlock(&wifi_ready_mutex);
}

static void wifi_ready_work_handler(struct k_work *item)
{
	wifi_ready_callback_t *cb = CONTAINER_OF(item, wifi_ready_callback_t, item);

	call_wifi_ready_callbacks(true, cb->iface);
}

static void handle_wpa_supp_event(struct net_if *iface, bool ready)
{
	LOG_DBG("Supplicant event %s for iface %p",
		ready ? "ready" : "not ready", iface);

	call_wifi_ready_callbacks(ready, iface);
}

static void wpa_supp_event_handler(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);

	LOG_DBG("Event received: %d", mgmt_event);
	switch (mgmt_event) {
	case NET_EVENT_SUPPLICANT_READY:
		handle_wpa_supp_event(iface, true);
		break;
	case NET_EVENT_SUPPLICANT_NOT_READY:
		handle_wpa_supp_event(iface, false);
		break;
	default:
		LOG_DBG("Unhandled event (%d)", mgmt_event);
		break;
	}
}

int register_wifi_ready_callback(wifi_ready_callback_t cb, struct net_if *iface)
{
	int ret = 0, i;
	unsigned int next_avail_idx = CONFIG_WIFI_READY_MAX_CALLBACKS;
	struct net_if *wpas_iface = NULL;

	k_mutex_lock(&wifi_ready_mutex, K_FOREVER);

	if (!cb.wifi_ready_cb) {
		LOG_ERR("Callback is NULL");
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(wifi_ready_callbacks); i++) {
		if (wifi_ready_callbacks[i].wifi_ready_cb == NULL) {
			next_avail_idx = i;
			break;
		}
	}

	if (next_avail_idx == CONFIG_WIFI_READY_MAX_CALLBACKS) {
		LOG_ERR("Reject callback registration, maximum count %d reached",
			CONFIG_WIFI_READY_MAX_CALLBACKS);
		ret = -ENOMEM;
		goto out;
	}

	/* Check if callback has already been registered for iface */
	for (i = 0; i < ARRAY_SIZE(wifi_ready_callbacks); i++) {
		if (wifi_ready_callbacks[i].iface == iface &&
		    wifi_ready_callbacks[i].wifi_ready_cb == cb.wifi_ready_cb) {
			LOG_ERR("Callback has already registered for iface");
			ret = -EALREADY;
			goto out;
		}
	}

	wifi_ready_callbacks[next_avail_idx].wifi_ready_cb = cb.wifi_ready_cb;
	wifi_ready_callbacks[next_avail_idx].iface = iface;

	if (++callback_count == 1) {
		net_mgmt_init_event_callback(&net_wpa_supp_cb,
			wpa_supp_event_handler, WPA_SUPP_EVENTS);
		net_mgmt_add_event_callback(&net_wpa_supp_cb);
	}

	if (iface) {
		wpas_iface = iface;
	} else {
		wpas_iface = net_if_get_first_wifi();
		if (!wpas_iface) {
			LOG_ERR("Failed to get Wi-Fi interface");
			ret = -ENODEV;
			goto out;
		}
	}

	/* In case Wi-Fi is already ready, call the callback */
	if (wifi_nm_get_instance_iface(wpas_iface)) {
		k_work_submit(&wifi_ready_callbacks[next_avail_idx].item);
	}

out:
	k_mutex_unlock(&wifi_ready_mutex);
	return ret;

}


int unregister_wifi_ready_callback(wifi_ready_callback_t cb, struct net_if *iface)
{
	int ret = 0, i;
	bool found = false;

	k_mutex_lock(&wifi_ready_mutex, K_FOREVER);

	if (!cb.wifi_ready_cb) {
		LOG_ERR("Callback is NULL");
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(wifi_ready_callbacks); i++) {
		if (wifi_ready_callbacks[i].iface == iface &&
		    wifi_ready_callbacks[i].wifi_ready_cb == cb.wifi_ready_cb) {
			wifi_ready_callbacks[i].wifi_ready_cb = NULL;
			wifi_ready_callbacks[i].iface = NULL;
			found = true;
		}
	}

	if (!found) {
		ret = -ENOENT;
		goto out;
	}

	if (--callback_count == 0) {
		net_mgmt_del_event_callback(&net_wpa_supp_cb);
	}

out:
	if (ret < 0) {
		LOG_ERR("Failed to unregister callback: %d", ret);
	}
	k_mutex_unlock(&wifi_ready_mutex);
	return ret;
}

static int wifi_ready_init(void)
{
	/* Initialize the work items */
	for (int i = 0; i < ARRAY_SIZE(wifi_ready_callbacks); i++) {
		k_work_init(&wifi_ready_callbacks[i].item, wifi_ready_work_handler);
	}

	return 0;
}

SYS_INIT(wifi_ready_init, APPLICATION, CONFIG_WIFI_READY_INIT_PRIORITY);
