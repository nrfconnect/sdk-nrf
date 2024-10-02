/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <assert.h>

#include <modem/lte_lc.h>

#include <zephyr/net/ppp.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>

#include <zephyr/net/ethernet.h>

#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/shell/shell.h>

#include "link_api.h"
#include "mosh_print.h"

#include "ppp_settings.h"
#include "ppp_ctrl.h"

extern struct k_work_q mosh_common_work_q;

/* ppp globals: */
struct net_if *ppp_iface_global;
K_SEM_DEFINE(ppp_sockets_sem, 0, 2);

/* Socket to send and recv data to/from modem */
int ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

/* Socket to send and recv data to/from Zephyr PPP link */
int ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

/* LTE link MTU */
uint16_t used_mtu_mru;

/* Work for auto starting/stopping ppp according to default PDN
 * activation status
 */
struct ppp_ctrl_pdn_status_worker_data {
	struct k_work work;
	bool default_pdn_active;
} ppp_ctrl_pdn_status_worker_data;

static const struct device *ppp_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ppp_uart));

/* Work for a re-starting PPP */
static struct k_work ppp_ctrl_restart_work;

static bool ppp_up;

static volatile bool ppp_restarting;
static volatile bool ppp_prot_running;

/******************************************************************************/

static void ppp_ctrl_stop_net_if(void)
{
	struct ppp_context *ctx;
	int idx = 0; /* Note: PPP iface index assumed to be 0 */

	ctx = net_ppp_context_get(idx);
	if (!ctx && !ctx->iface) {
		return;
	}

	net_if_down(ctx->iface);
}

static int ppp_ctrl_start_net_if(void)
{
	struct ppp_context *ctx;
	struct net_if *iface;
	struct pdp_context_info *pdp_context_info;
	int idx = 0; /* Note: PPP iface index assumed to be 0 */
	int ret = 0;

	ctx = net_ppp_context_get(idx);
	if (!ctx) {
		mosh_error("PPP context not found.\n");
		goto return_error;
	}

	/* Note: no multicontext support, only at default PDN context */
	pdp_context_info = link_api_get_pdp_context_info_by_pdn_cid(0);
	if (pdp_context_info == NULL) {
		mosh_error("Default PDN context not active.\n");
		goto return_error;
	}

	iface = ctx->iface;
	ppp_iface_global = iface;

	/* Set PPP MTU according to LTE link IPv4 MTU: */
	if (pdp_context_info->ipv4_mtu) {
		if (pdp_context_info->ipv4_mtu <= PPP_MODEM_DATA_RCV_SND_BUFF_SIZE) {
			used_mtu_mru = pdp_context_info->ipv4_mtu;
		} else {
			mosh_warn("LTE link MTU (%d) cannot be set as PPP MTU. Setting to the max: %d\n",
				pdp_context_info->ipv4_mtu,
				PPP_MODEM_DATA_RCV_SND_BUFF_SIZE);
			used_mtu_mru = PPP_MODEM_DATA_RCV_SND_BUFF_SIZE;
		}
		net_if_set_mtu(iface, used_mtu_mru);
	} /* else: link MTU not known: default CONFIG_NET_PPP_MTU_MRU is used
	   *       as set by PPP Zephyr driver during initialization.
	   */

	memcpy(&(ctx->ipcp.my_options.address), &(pdp_context_info->ip_addr4),
	       sizeof(ctx->ipcp.my_options.address));
	memcpy(&ctx->ipcp.my_options.dns1_address,
	       &pdp_context_info->dns_addr4_primary,
	       sizeof(ctx->ipcp.my_options.dns1_address));
	memcpy(&ctx->ipcp.my_options.dns2_address,
	       &pdp_context_info->dns_addr4_secondary,
	       sizeof(ctx->ipcp.my_options.dns2_address));

	free(pdp_context_info);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);

	ret = net_if_up(iface);
	if (ret) {
		mosh_warn("Cannot bring PPP up, err: %d", ret);
		goto return_error;
	}

	return 0;

return_error:
	return -1;
}

/******************************************************************************/

static void ppp_ctrl_link_default_pdn_status_handler(struct k_work *work_item)
{
	struct ppp_ctrl_pdn_status_worker_data *data_ptr =
		CONTAINER_OF(work_item, struct ppp_ctrl_pdn_status_worker_data, work);

	if (data_ptr->default_pdn_active == true) {
		mosh_print("Default PDN is active: starting PPP automatically.");
		ppp_ctrl_start();
	} else {
		mosh_print("Default PDN is not active: stopping PPP automatically.");
		ppp_ctrl_stop();
	}
}

/******************************************************************************/

static void ppp_ctrl_restart_worker(struct k_work *work_item)
{
	ARG_UNUSED(work_item);

	ppp_restarting = true;
	ppp_ctrl_stop();
	ppp_ctrl_start();
	ppp_restarting = false;
	mosh_print("PPP: restarting is DONE");
}

/******************************************************************************/

static struct net_mgmt_event_callback ppp_ctrl_net_if_mgmt_event_ppp_cb;

static void ppp_ctrl_net_if_mgmt_event_handler(struct net_mgmt_event_callback *cb,
					    uint32_t mgmt_event,
					    struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IF_UP) {
		mosh_print("PPP net if up");
		return;
	}
	if (mgmt_event == NET_EVENT_IF_DOWN) {
		mosh_print("PPP net if down");
		return;
	}
}

/******************************************************************************/

static struct net_mgmt_event_callback ppp_ctrl_net_mgmt_event_ppp_cb;

static void ppp_ctrl_net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
					    uint32_t mgmt_event,
					    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_PPP_PHASE_RUNNING: {
		ppp_prot_running = true;
		mosh_print("PPP in running phase");
		break;
	}
	case NET_EVENT_PPP_PHASE_DEAD: {
		mosh_print("PPP in dead phase");
		if (ppp_prot_running) {
			/* To be able to reconnect the dial up from Windows UI,
			 * we need to bring PPP net_if and sockets down/up:
			 */
			if (!ppp_restarting && ppp_up) {
				mosh_print("PPP: restarting...");
				k_work_submit_to_queue(&mosh_common_work_q, &ppp_ctrl_restart_work);
			}
			ppp_prot_running = false;
		}
		break;
	}
	default:
		break;
	}
}

static struct net_mgmt_event_callback ipv4_level_net_mgmt_event_cb;
static void
ppp_ctrl_net_mgmt_event_ipv4_levelhandler(struct net_mgmt_event_callback *cb,
					uint32_t mgmt_event,
					struct net_if *iface)
{
	if (iface != ppp_iface_global) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		mosh_print("Dial up (IPv4) connection up");
	} else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
		mosh_print("Dial up (IPv4) connection down");
	}
}

static struct net_mgmt_event_callback ipv6_level_net_mgmt_event_cb;
static void
ppp_ctrl_net_mgmt_event_ipv6_levelhandler(struct net_mgmt_event_callback *cb,
					uint32_t mgmt_event,
					struct net_if *iface)
{
	if (iface != ppp_iface_global) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		mosh_print("Dial up (IPv6) connection up");
	} else if (mgmt_event == NET_EVENT_IPV6_ADDR_DEL) {
		mosh_print("Dial up (IPv6) connection down");
	}
}

static void ppp_ctrl_net_mgmt_events_subscribe(void)
{
	net_mgmt_init_event_callback(&ppp_ctrl_net_if_mgmt_event_ppp_cb,
				     ppp_ctrl_net_if_mgmt_event_handler,
				     (NET_EVENT_IF_UP |
				      NET_EVENT_IF_DOWN));

	net_mgmt_init_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb,
				     ppp_ctrl_net_mgmt_event_handler,
				     (NET_EVENT_IF_UP |
				      NET_EVENT_IF_DOWN |
				      NET_EVENT_PPP_PHASE_RUNNING |
				      NET_EVENT_PPP_PHASE_DEAD));

	net_mgmt_init_event_callback(&ipv4_level_net_mgmt_event_cb,
				     ppp_ctrl_net_mgmt_event_ipv4_levelhandler,
				     (NET_EVENT_IPV4_ADDR_ADD |
				      NET_EVENT_IPV4_ADDR_DEL));

	net_mgmt_init_event_callback(&ipv6_level_net_mgmt_event_cb,
				     ppp_ctrl_net_mgmt_event_ipv6_levelhandler,
				     (NET_EVENT_IPV6_ADDR_ADD |
				      NET_EVENT_IPV6_ADDR_DEL));

	net_mgmt_add_event_callback(&ppp_ctrl_net_if_mgmt_event_ppp_cb);
	net_mgmt_add_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb);
	net_mgmt_add_event_callback(&ipv4_level_net_mgmt_event_cb);
	net_mgmt_add_event_callback(&ipv6_level_net_mgmt_event_cb);
}

/******************************************************************************/

void ppp_ctrl_init(void)
{
	ppp_up = false;
	ppp_restarting = false;
	ppp_prot_running = true;

	used_mtu_mru = CONFIG_NET_PPP_MTU_MRU;

	ppp_ctrl_net_mgmt_events_subscribe();

	k_work_init(&ppp_ctrl_pdn_status_worker_data.work,
		    ppp_ctrl_link_default_pdn_status_handler);

	/* A work for doing the restart for ppp: */
	k_work_init(&ppp_ctrl_restart_work, ppp_ctrl_restart_worker);

	if (!device_is_ready(ppp_uart_dev)) {
		mosh_warn("PPP UART device not ready");
	}

	ppp_settings_init();
}

void ppp_ctrl_default_pdn_active(bool default_pdn_active)
{
	ppp_ctrl_pdn_status_worker_data.default_pdn_active = default_pdn_active;
	k_work_submit_to_queue(&mosh_common_work_q, &ppp_ctrl_pdn_status_worker_data.work);
}

/******************************************************************************/

static int ppp_ctrl_modem_sckt_create(void)
{
	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_warn("PPP modem sckt already up - nothing to do");
		return 0;
	}

	ppp_modem_data_socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
	if (ppp_modem_data_socket_fd < 0) {
		mosh_error("modem data socket creation failed: (%d)\n", -errno);
		goto return_error;
	} else {
		mosh_print("modem data socket %d created for modem data", ppp_modem_data_socket_fd);
	}

#ifdef SO_SNDTIMEO
	struct timeval tv;

	/* blocking socket and we do not want to block for long: 1 sec timeout
	 * for both sending and receiving:
	 */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(ppp_modem_data_socket_fd, SOL_SOCKET, SO_SNDTIMEO,
		       (struct timeval *)&tv, sizeof(struct timeval)) < 0) {
		mosh_warn("Unable to set socket SO_SNDTIMEO - continue");
	}
#ifdef SO_RCVTIMEO
	/* We want to set SO_RCVTIMEO for workarounding problems that are seen
	 * if closing sckt from another thread while in nrf_recv().
	 */
	if (setsockopt(ppp_modem_data_socket_fd, SOL_SOCKET, SO_RCVTIMEO,
		       (struct timeval *)&tv, sizeof(struct timeval)) < 0) {
		mosh_warn("Unable to set socket SO_RCVTIMEO - continue");
	}
#endif
#endif
	return 0;

return_error:
	return -1;
}

static int ppp_ctrl_zephyr_sckt_create(void)
{
	int ret;

	if (ppp_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_warn("PPP Zephyr data socket already up - nothing to do");
		return 0;
	}

	/* Create raw Zephyr socket for passing data to/from ppp link: */
	ppp_data_socket_fd = socket(AF_PACKET, SOCK_RAW | SOCK_NATIVE,
				    htons(IPPROTO_RAW));
	if (ppp_data_socket_fd < 0) {
		mosh_error("PPP Zephyr data socket creation failed: (%d)\n", -errno);
		goto return_error;
	} else {
		mosh_print("PPP Zephyr data socket %d created", ppp_data_socket_fd);
	}

	/* Bind to PPP net if: */
	struct sockaddr_ll dst = { 0 };

	dst.sll_ifindex = net_if_get_by_iface(ppp_iface_global);
	dst.sll_family = AF_PACKET;

	ret = bind(ppp_data_socket_fd, (const struct sockaddr *)&dst,
		       sizeof(struct sockaddr_ll));
	if (ret < 0) {
		mosh_error("Failed to bind PPP data socket : %d", errno);
		(void)close(ppp_data_socket_fd);
		goto return_error;
	}
	return 0;
return_error:
	return -1;

}

/******************************************************************************/

static int ppp_ctrl_sckts_create(void)
{
	int err;

	err = ppp_ctrl_zephyr_sckt_create();
	if (err) {
		mosh_error("Cannot create zephyr sckt for ppp: %d", err);
		return err;
	}

	err = ppp_ctrl_modem_sckt_create();
	if (err) {
		mosh_error("Cannot create modem sckt for ppp: %d", err);
		return err;
	}

	/* Give a semaphore for both snd and rcv */
	k_sem_give(&ppp_sockets_sem);
	k_sem_give(&ppp_sockets_sem);
	return 0;
}


static void ppp_ctrl_close_sckts(void)
{
	int err;
	int tmp_sckt_id;

	k_sem_reset(&ppp_sockets_sem);

	if (ppp_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		tmp_sckt_id = ppp_data_socket_fd;

		/* Change global sckt variables already before actual close to avoid going
		 * to poll() etc in UL/DL threads when closing.
		 */
		ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
		mosh_print("Closing PPP zephyr sckt");
		err = close(tmp_sckt_id);
		if (err) {
			mosh_print("Closing of PPP zephyr sckt failed, errno %d", -errno);
		}
	}

	/* Give some time for closing zephyr socket. */
	k_sleep(K_SECONDS(2));

	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_print("Closing PPP modem sckt");
		tmp_sckt_id = ppp_modem_data_socket_fd;
		ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
		err = close(tmp_sckt_id);
		if (err) {
			mosh_print("Closing of PPP modem sckt failed, errno %d", -errno);
		}
	}

}

/******************************************************************************/

int ppp_ctrl_start(void)
{
	if (ppp_up) {
		mosh_warn("PPP already up.\n");
		return 0;
	}

	if (ppp_ctrl_start_net_if()) {
		goto clear;
	}

	if (ppp_ctrl_sckts_create()) {
		goto clear;
	}
	ppp_up = true;
	mosh_print("PPP: started");

	return 0;

clear:
	ppp_up = false;
	mosh_error("PPP cannot be started");
	ppp_ctrl_stop_net_if();
	ppp_ctrl_close_sckts();
	return -1;
}

/******************************************************************************/

void ppp_ctrl_stop(void)
{
	ppp_up = false;
	ppp_ctrl_stop_net_if();
	ppp_ctrl_close_sckts();

	mosh_print("PPP: stopped");
}
