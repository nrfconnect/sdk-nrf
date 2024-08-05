/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Permission to use, copy, modify, and/or distribute this software for any         */
/* purpose with or without fee is hereby granted, provided that the above           */
/* copyright notice and this permission notice appear in all copies.                */

/* THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL                    */
/* WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                    */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL                     */
/* THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR                       */
/* CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING                        */
/* FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF                       */
/* CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT                       */
/* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS                          */
/* SOFTWARE. */

#include <zephyr/net/net_event.h>
#include <ctrl_iface_zephyr.h>
#include <supp_events.h>
#include <utils.h>
#include <supp_main.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(wpas_event, CONFIG_WFA_QT_LOG_LEVEL);

/* TODO: Handle other events */
#define WPA_SUPP_EVENTS (NET_EVENT_SUPPLICANT_READY)

static struct net_mgmt_event_callback net_wpa_supp_cb;

K_SEM_DEFINE(wpa_supp_ready_sem, 0, 1);

static void handle_wpa_supp_ready(struct net_mgmt_event_callback *cb)
{
	k_sem_give(&wpa_supp_ready_sem);
}

static void wpa_supp_event_handler(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event, struct net_if *iface)
{
	/* TODO: Handle other events */
	switch (mgmt_event) {
	case NET_EVENT_SUPPLICANT_READY:
		handle_wpa_supp_ready(cb);
		break;
	default:
		LOG_DBG("Unhandled event (%d)", mgmt_event);
		break;
	}
}

int wait_for_wpa_s_ready(void)
{
	struct wpa_supplicant *wpa_s = zephyr_get_handle_by_ifname(CONFIG_WFA_QT_DEFAULT_INTERFACE);
	int ret;

	if (wpa_s) {
		LOG_INF("WPA Supplicant is already ready: %s", CONFIG_WFA_QT_DEFAULT_INTERFACE);
		return 0;
	}

	ret = k_sem_take(&wpa_supp_ready_sem, K_MSEC(CONFIG_WPAS_READY_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_INF("Timeout waiting for WPA Supplicant to be ready (%d ms)",
			CONFIG_WPAS_READY_TIMEOUT_MS);
		return -1;
	}

	wpa_s = zephyr_get_handle_by_ifname(CONFIG_WFA_QT_DEFAULT_INTERFACE);
	if (!wpa_s) {
		LOG_INF("WPA Supplicant ready event received, but no handle found for %s",
			 CONFIG_WFA_QT_DEFAULT_INTERFACE);
		return -1;
	}

	/* Belts and braces: Check for ctrl_iface initialization */
	if (wpa_s->ctrl_iface->sock_pair[0] < 0) {
		LOG_INF("WPA Supplicant ready event received,"
			"but no control interface socket found for %s",
			CONFIG_WFA_QT_DEFAULT_INTERFACE);
		return -1;
	}

	LOG_INF("WPA Supplicant is ready: %s", CONFIG_WFA_QT_DEFAULT_INTERFACE);

	return 0;
}

int wpa_supp_events_register(void)
{
	net_mgmt_init_event_callback(&net_wpa_supp_cb, wpa_supp_event_handler, WPA_SUPP_EVENTS);
	net_mgmt_add_event_callback(&net_wpa_supp_cb);

	return 0;
}
