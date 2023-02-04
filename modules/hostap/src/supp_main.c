/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * WPA Supplicant / main() function for Zephyr OS
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <zephyr/logging/log.h>
#include <sys/fcntl.h>
LOG_MODULE_REGISTER(wpa_supplicant, LOG_LEVEL_DBG);

#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"

#include "fst/fst.h"
#include "includes.h"
#include "p2p_supplicant.h"
#include "driver_i.h"

#include "supp_main.h"
#include "wpa_cli_zephyr.h"

K_SEM_DEFINE(z_wpas_ready_sem, 0, 1);

struct wpa_global *global;

static int z_wpas_event_sockpair[2];

static void z_wpas_start(void);

K_THREAD_DEFINE(z_wpa_s_tid,
				CONFIG_WPA_SUPP_THREAD_STACK_SIZE,
				z_wpas_start,
				NULL,
				NULL,
				NULL,
				0,
				0,
				0);

#ifdef CONFIG_MATCH_IFACE
static int z_wpas_init_match(struct wpa_global *global)
{
	/*
	 * The assumption is that the first driver is the primary driver and
	 * will handle the arrival / departure of interfaces.
	 */
	if (wpa_drivers[0]->global_init && !global->drv_priv[0]) {
		global->drv_priv[0] = wpa_drivers[0]->global_init(global);
		if (!global->drv_priv[0]) {
			wpa_printf(MSG_ERROR,
				   "Failed to initialize driver '%s'",
				   wpa_drivers[0]->name);
			return -1;
		}
	}

	return 0;
}
#endif /* CONFIG_MATCH_IFACE */

struct wpa_supplicant *z_wpas_get_handle_by_ifname(const char* ifname)
{
	struct wpa_supplicant *wpa_s = NULL;
	int ret = k_sem_take(&z_wpas_ready_sem, K_SECONDS(2));

	if (ret) {
		wpa_printf(MSG_ERROR, "%s: WPA supplicant not ready: %d", __func__, ret);
		return NULL;
	}

	k_sem_give(&z_wpas_ready_sem);

	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR,
		"%s: Unable to get wpa_s handle for %s\n", __func__, ifname);
		return NULL;
	}

	return wpa_s;
}

#include "config.h"
static void iface_cb(struct net_if *iface, void *user_data)
{
	struct wpa_interface *ifaces = user_data;
	struct net_linkaddr *link_addr = NULL;
	int ifindex;
	char own_addr[NET_LINK_ADDR_MAX_LENGTH];
	char *ifname;

	ifindex = net_if_get_by_iface(iface);
	link_addr = &iface->if_dev->link_addr;
	/* 802.15.4 interface, ignore */
	if (link_addr->len > NET_LINK_ADDR_MAX_LENGTH) {
		return;
	}
	os_memcpy(own_addr, link_addr->addr, link_addr->len);
	ifname = os_strdup(iface->if_dev->dev->name);

	wpa_printf(
		MSG_INFO,
		"iface_cb: iface %s ifindex %d %02x:%02x:%02x:%02x:%02x:%02x",
		ifname ? ifname: "Unknown", ifindex, own_addr[0], own_addr[1],
		own_addr[2], own_addr[3], own_addr[4], own_addr[5]);

	/* TODO : make this user configurable*/
	ifaces[0].ifname = "wlan0";

	os_free(ifname);
}

static void z_wpas_event_sock_handler(int sock, void *eloop_ctx, void *sock_ctx)
{
	int ret;
	ARG_UNUSED(eloop_ctx);
	ARG_UNUSED(sock_ctx);
	struct wpa_supplicant_event_msg msg;

	ret = recv(sock, &msg, sizeof(msg), 0);

	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Failed to recv the message: %s", strerror(errno));
		return;
	}

	if (ret != sizeof(msg)) {
		wpa_printf(MSG_ERROR, "Recieved incomplete message: got: %d, expected:%d",
			ret, sizeof(msg));
		return;
	}

	if (msg.ignore_msg) {
		return;
	}

	wpa_printf(MSG_DEBUG, "Passing message %d to wpa_supplicant", msg.event);

	wpa_supplicant_event(msg.ctx, msg.event, msg.data);

	if (msg.data) {
		os_free(msg.data);
	}
}

static int register_wpa_event_sock(void)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, z_wpas_event_sockpair);

	if (ret != 0) {
		wpa_printf(MSG_ERROR, "Failed to initialize socket: %s", strerror(errno));
		return -1;
	}

	eloop_register_read_sock(z_wpas_event_sockpair[0], z_wpas_event_sock_handler, NULL, NULL);

	return 0;
}

int z_wpas_send_event(const struct wpa_supplicant_event_msg *msg)
{
	int ret;
	unsigned int retry = 0;

	if (z_wpas_event_sockpair[1] < 0) {
		return -1;
	}

retry_send:
	ret = send(z_wpas_event_sockpair[1], msg, sizeof(*msg), 0);
	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK) {
			k_msleep(2);
			if (retry++ < 3) {
				goto retry_send;
			} else {
				wpa_printf(MSG_WARNING, "Dummy socket send fail (max retries): %s",
					strerror(errno));
				return -1;
			}
		} else {
			wpa_printf(MSG_WARNING, "Dummy socket send fail: %s",
				strerror(errno));
			return -1;
		}
	}

	return 0;
}

static void z_wpas_start(void)
{
	int i;
	struct wpa_interface *ifaces, *iface;
	int iface_count, exitcode = -1;
	struct wpa_params params;

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = CONFIG_WPA_SUPP_DEBUG_LEVEL;

	wpa_printf(MSG_INFO, "%s: %d Starting wpa_supplicant thread with debug level: %d\n",
		  __func__, __LINE__, params.wpa_debug_level);

	iface = ifaces = os_zalloc(sizeof(struct wpa_interface));
	if (ifaces == NULL) {
		return;
	}
	iface_count = 1;

	exitcode = 0;
	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize wpa_supplicant");
		exitcode = -1;
		goto out;
	} else {
		wpa_printf(MSG_INFO, "Successfully initialized "
					 "wpa_supplicant");
	}

	if (fst_global_init()) {
		wpa_printf(MSG_ERROR, "Failed to initialize FST");
		exitcode = -1;
		goto out;
	}

#if defined(CONFIG_FST) && defined(CONFIG_CTRL_IFACE)
	if (!fst_global_add_ctrl(fst_ctrl_cli)) {
		wpa_printf(MSG_WARNING, "Failed to add CLI FST ctrl");
	}
#endif

	net_if_foreach(iface_cb, ifaces);

	ifaces[0].ctrl_interface = "zephyr";
	params.ctrl_interface = "zephyr";
	wpa_printf(MSG_INFO, "Using interface %s\n", ifaces[0].ifname);

	for (i = 0; exitcode == 0 && i < iface_count; i++) {
		struct wpa_supplicant *wpa_s;

		if ((ifaces[i].confname == NULL &&
			 ifaces[i].ctrl_interface == NULL) ||
			ifaces[i].ifname == NULL) {
			if (iface_count == 1 && (params.ctrl_interface ||
#ifdef CONFIG_MATCH_IFACE
						 params.match_iface_count ||
#endif /* CONFIG_MATCH_IFACE */
						 params.dbus_ctrl_interface))
				break;
			wpa_printf(MSG_INFO,
				   "Failed to initialize interface %d\n", i);
			exitcode = -1;
			break;
		}
		wpa_printf(MSG_INFO, "Initializing interface %d: %s\n", i,
			   ifaces[i].ifname);
		wpa_s = wpa_supplicant_add_iface(global, &ifaces[i], NULL);
		if (wpa_s == NULL) {
			exitcode = -1;
			break;
		}
		z_wpa_ctrl_init(wpa_s);
		wpa_s->conf->filter_ssids = 1;
		wpa_s->conf->ap_scan = 1;
	}

	register_wpa_event_sock();

#ifdef CONFIG_MATCH_IFACE
	if (exitcode == 0) {
		exitcode = z_wpas_init_match(global);
	}
#endif /* CONFIG_MATCH_IFACE */

	if (exitcode == 0) {
		k_sem_give(&z_wpas_ready_sem);
		exitcode = wpa_supplicant_run(global);
	}

	eloop_unregister_read_sock(z_wpas_event_sockpair[0]);

	z_wpa_ctrl_deinit();
	wpa_supplicant_deinit(global);

	fst_global_deinit();

	close(z_wpas_event_sockpair[0]);
	close(z_wpas_event_sockpair[1]);

out:
	os_free(ifaces);
#ifdef CONFIG_MATCH_IFACE
	os_free(params.match_ifaces);
#endif /* CONFIG_MATCH_IFACE */
	os_free(params.pid_file);

	wpa_printf(MSG_INFO, "z_wpas_start: exitcode %d", exitcode);
	return;
}
