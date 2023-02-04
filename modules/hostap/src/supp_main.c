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
#include <l2_packet/l2_packet.h>

/* Should match with the driver name */
#define DEFAULT_IFACE_NAME "wlan0"

static struct net_mgmt_event_callback cb;
struct k_mutex iface_up_mutex;

struct wpa_global *global;

static int z_wpas_event_sockpair[2];

static void z_wpas_start(void);
static void z_wpas_iface_work_handler(struct k_work *item);

K_THREAD_DEFINE(z_wpa_s_tid,
				CONFIG_WPA_SUPP_THREAD_STACK_SIZE,
				z_wpas_start,
				NULL,
				NULL,
				NULL,
				0,
				0,
				0);

static K_THREAD_STACK_DEFINE(z_wpas_iface_wq_stack,
	CONFIG_WPA_SUPP_IFACE_WQ_STACK_SIZE);

/* TODO: Debug why wsing system workqueue blocks the driver dedicated
 * workqueue?
 */
static struct k_work_q z_wpas_iface_wq;
static K_WORK_DEFINE(z_wpas_iface_work,
	z_wpas_iface_work_handler);


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

static int z_wpas_get_iface_count(void)
{
	struct wpa_supplicant *wpa_s;
	unsigned count = 0;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
			count += 1;
	}
	return count;
}

static int z_wpas_add_interface(const char* ifname)
{
	struct wpa_supplicant *wpa_s;
	int ret;

	ret = z_wpa_cli_global_cmd_v("interface_add %s %s %s %s",
				ifname, "zephyr", "zephyr", "zephyr");
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add interface: %s", ifname);
		return -1;
	}

	/* This cannot be through control interface as need the handle */
	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (wpa_s == NULL) {
		wpa_printf(MSG_ERROR, "Failed to add iface: %s", ifname);
		return -1;
	}
	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan= 1;

	/* Default interface, kick start wpa_supplicant */
	if (z_wpas_get_iface_count() == 1) {
		k_mutex_unlock(&iface_up_mutex);
	}

	z_wpa_ctrl_init(wpa_s);

	return 0;
}

static int z_wpas_remove_interface(const char* ifname)
{
	int ret;

	wpa_printf(MSG_INFO, "Remove interface %s\n", ifname);

	ret = z_wpa_cli_global_cmd_v("interface_remove %s", ifname);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add interface: %s", ifname);
		return -1;
	}

	return 0;
}

static void iface_event_handler(struct net_mgmt_event_callback *cb,
							uint32_t mgmt_event, struct net_if *iface)
{
	const char *ifname = iface->if_dev->dev->name;

	wpa_printf(MSG_INFO, "Event: %d", mgmt_event);
	if (mgmt_event == NET_EVENT_IF_ADMIN_UP) {
		z_wpas_add_interface(ifname);
	} else if (mgmt_event == NET_EVENT_IF_ADMIN_DOWN) {
		z_wpas_remove_interface(ifname);
	}
}

static void register_iface_events(void)
{
	k_mutex_init(&iface_up_mutex);

	k_mutex_lock(&iface_up_mutex, K_FOREVER);
	net_mgmt_init_event_callback(&cb, iface_event_handler,
		NET_EVENT_IF_ADMIN_UP | NET_EVENT_IF_ADMIN_DOWN);
	net_mgmt_add_event_callback(&cb);
}

static void wait_for_interface_up(const char* iface_name)
{
	if (z_wpas_get_iface_count() == 0) {
		k_mutex_lock(&iface_up_mutex, K_FOREVER);
	}
}

#include "config.h"
static void iface_cb(struct net_if *iface, void *user_data)
{
	const char *ifname = iface->if_dev->dev->name;

	if (ifname == NULL) {
		return;
	}

	if (strncmp(ifname, DEFAULT_IFACE_NAME, strlen(ifname)) != 0)
	{
		return;
	}

	/* Check default interface */
	if (net_if_is_admin_up(iface)) {
		z_wpas_add_interface(ifname);
	}

	register_iface_events();
}


static void z_wpas_iface_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	int ret = k_sem_take(&z_wpas_ready_sem, K_SECONDS(5));
	if (ret) {
		wpa_printf(MSG_ERROR, "Timed out waiting for wpa_supplicant");
		return;
	}

	net_if_foreach(iface_cb, NULL);
	wait_for_interface_up(DEFAULT_IFACE_NAME);

	k_sem_give(&z_wpas_ready_sem);
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
	struct wpa_params params;
	int exitcode = -1;
	k_work_queue_init(&z_wpas_iface_wq);

	k_work_queue_start(&z_wpas_iface_wq,
					   z_wpas_iface_wq_stack,
					   K_THREAD_STACK_SIZEOF(z_wpas_iface_wq_stack),
					   7,
					   NULL);

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = CONFIG_WPA_SUPP_DEBUG_LEVEL;

	wpa_printf(MSG_INFO, "%s: %d Starting wpa_supplicant thread with debug level: %d\n",
		  __func__, __LINE__, params.wpa_debug_level);

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
	z_global_wpa_ctrl_init();

	register_wpa_event_sock();

	k_work_submit_to_queue(&z_wpas_iface_wq, &z_wpas_iface_work);

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
	z_global_wpa_ctrl_deinit();
	wpa_supplicant_deinit(global);

	fst_global_deinit();

	close(z_wpas_event_sockpair[0]);
	close(z_wpas_event_sockpair[1]);

out:
#ifdef CONFIG_MATCH_IFACE
	os_free(params.match_ifaces);
#endif /* CONFIG_MATCH_IFACE */
	os_free(params.pid_file);

	wpa_printf(MSG_INFO, "z_wpas_start: exitcode %d", exitcode);
	return;
}
