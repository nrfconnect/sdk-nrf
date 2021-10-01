/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <assert.h>

#include <modem/lte_lc.h>

#include <net/ppp.h>

#include <net/net_ip.h>
#include <net/net_if.h>

#include <net/net_event.h>
#include <net/net_mgmt.h>

#include <posix/unistd.h>
#include <posix/netdb.h>

#include <net/ethernet.h>

#include <posix/poll.h>
#include <posix/sys/socket.h>
#include <shell/shell.h>

#include <settings/settings.h>

#include "link_api.h"
#include "mosh_print.h"

#include "ppp_settings.h"
#include "ppp_ctrl.h"

/* ppp globals: */
struct net_if *ppp_iface_global;
K_SEM_DEFINE(zsocket_sem, 0, 1);
K_SEM_DEFINE(msocket_sem, 0, 1);

/* Socket to send and recv data to/from modem */
int ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

/* Socket to send and recv data to/from Zephyr PPP link */
int ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

/* LTE link MTU */
uint16_t used_mtu_mru;

/* Work for auto starting/stopping ppp according to default PDN
 * activation status
 */
struct ppp_ctrl_worker_data {
	struct k_work work;
	bool default_pdn_active;
} ppp_ctrl_worker_data;

static const struct device *ppp_uart_dev;

/* Work for a starting/stopping net if */
static struct k_work ppp_net_if_start_work;
static struct k_work ppp_net_if_stop_work;

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
	if (pdp_context_info->mtu) {
		if (pdp_context_info->mtu <= PPP_MODEM_DATA_RCV_SND_BUFF_SIZE) {
			used_mtu_mru = pdp_context_info->mtu;
		} else {
			mosh_warn("LTE link MTU (%d) cannot be set as PPP MTU. Setting to the max: %d\n",
				pdp_context_info->mtu,
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
	net_if_up(iface);

	return 0;

return_error:
	return -1;
}

/******************************************************************************/

static void ppp_ctrl_link_default_pdn_status_handler(struct k_work *work_item)
{
	struct ppp_ctrl_worker_data *data_ptr =
		CONTAINER_OF(work_item, struct ppp_ctrl_worker_data, work);

	if (data_ptr->default_pdn_active == true) {
		mosh_print("Default PDN is active: starting PPP automatically.");
		ppp_ctrl_start();
	} else {
		mosh_print("Default PDN is not active: stopping PPP automatically.");
		ppp_ctrl_stop();
	}
}

/******************************************************************************/
static void ppp_ctrl_net_if_stop(struct k_work *work_item)
{
	ARG_UNUSED(work_item);

	ppp_ctrl_stop_net_if();

	mosh_print("PPP net_if: stopped");

}

static void ppp_ctrl_net_if_start(struct k_work *work_item)
{
	ARG_UNUSED(work_item);

	if (!ppp_ctrl_start_net_if()) {
		mosh_print("PPP net_if: started");
	} else {
		mosh_print("PPP net_if: starting failed");
	}
}

/******************************************************************************/

static struct net_mgmt_event_callback ppp_ctrl_net_mgmt_event_ppp_cb;

static void ppp_ctrl_net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
					    uint32_t mgmt_event,
					    struct net_if *iface)
{
	if ((mgmt_event & (NET_EVENT_PPP_CARRIER_ON |
			   NET_EVENT_PPP_CARRIER_OFF)) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_PPP_CARRIER_ON) {
		mosh_print("PPP carrier ON");
		return;
	}

	if (mgmt_event == NET_EVENT_PPP_CARRIER_OFF) {
		mosh_print("PPP carrier OFF");
		return;
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

		/* To be able to reconnect the dial up, we need to bring PPP net_if down/up: */
		mosh_print("PPP net_if: restarting...");
		k_work_submit(&ppp_net_if_stop_work);
		k_work_submit(&ppp_net_if_start_work);
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
	net_mgmt_init_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb,
				     ppp_ctrl_net_mgmt_event_handler,
				     (NET_EVENT_PPP_CARRIER_ON |
				      NET_EVENT_PPP_CARRIER_OFF));

	net_mgmt_init_event_callback(&ipv4_level_net_mgmt_event_cb,
				     ppp_ctrl_net_mgmt_event_ipv4_levelhandler,
				     (NET_EVENT_IPV4_ADDR_ADD |
				      NET_EVENT_IPV4_ADDR_DEL));

	net_mgmt_init_event_callback(&ipv6_level_net_mgmt_event_cb,
				     ppp_ctrl_net_mgmt_event_ipv6_levelhandler,
				     (NET_EVENT_IPV6_ADDR_ADD |
				      NET_EVENT_IPV6_ADDR_DEL));

	net_mgmt_add_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb);
	net_mgmt_add_event_callback(&ipv4_level_net_mgmt_event_cb);
	net_mgmt_add_event_callback(&ipv6_level_net_mgmt_event_cb);
}

/******************************************************************************/

void ppp_ctrl_init(void)
{
	used_mtu_mru = CONFIG_NET_PPP_MTU_MRU;

	ppp_ctrl_net_mgmt_events_subscribe();

	k_work_init(&ppp_ctrl_worker_data.work,
		    ppp_ctrl_link_default_pdn_status_handler);

	/* 2 works for doing the restart for ppp net_if: */
	k_work_init(&ppp_net_if_start_work, ppp_ctrl_net_if_start);
	k_work_init(&ppp_net_if_stop_work, ppp_ctrl_net_if_stop);

	ppp_uart_dev = device_get_binding(CONFIG_NET_PPP_UART_NAME);
	if (!ppp_uart_dev) {
		mosh_warn("Cannot get ppp dev binding");
		ppp_uart_dev = NULL;
	}

	ppp_settings_init();
}

void ppp_ctrl_default_pdn_active(bool default_pdn_active)
{
	ppp_ctrl_worker_data.default_pdn_active = default_pdn_active;
	k_work_submit(&ppp_ctrl_worker_data.work);
}

/******************************************************************************/

static int ppp_ctrl_modem_sckt_create(void)
{
	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_warn("PPP modem sckt already up - nothing to do");
		goto return_error;
	}

	ppp_modem_data_socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
	if (ppp_modem_data_socket_fd < 0) {
		mosh_error("modem data socket creation failed: (%d)!!!!\n", -errno);
		goto return_error;
	} else {
		mosh_print("modem data socket %d created for modem data", ppp_modem_data_socket_fd);
	}

#ifdef SO_SNDTIMEO
	struct timeval tv;

	/* blocking socket and we do not want to block for long: 1 sec timeout for sending: */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(ppp_modem_data_socket_fd, SOL_SOCKET, SO_SNDTIMEO,
		       (struct timeval *)&tv, sizeof(struct timeval)) < 0) {
		mosh_error("Unable to set socket SO_SNDTIMEO");
		goto return_error;
	}
#endif
	return 0;

return_error:
	return -1;
}

static int ppp_ctrl_zephyr_sckt_create(void)
{
	if (ppp_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_warn("PPP Zephyr data socket already up - nothing to do");
		goto return_error;
	}
	/* Create raw Zephyr socket for passing data to/from ppp link: */
	ppp_data_socket_fd = socket(AF_PACKET, SOCK_RAW | SOCK_NATIVE, IPPROTO_RAW);
	if (ppp_data_socket_fd < 0) {
		mosh_error("PPP Zephyr data socket creation failed: (%d)!!!!\n", -errno);
		goto return_error;
	} else {
		mosh_print("PPP Zephyr data socket %d created", ppp_data_socket_fd);
	}

	/* Bind to PPP net if: */
	struct sockaddr_ll dst = { 0 };

	dst.sll_ifindex = net_if_get_by_iface(ppp_iface_global);
	dst.sll_family = AF_PACKET;

	int ret = bind(ppp_data_socket_fd, (const struct sockaddr *)&dst,
		       sizeof(struct sockaddr_ll));
	if (ret < 0) {
		mosh_error("Failed to bind PPP data socket : %d", errno);
		goto return_error;
	}
	return 0;
return_error:
	return -1;

}

/******************************************************************************/

int ppp_ctrl_sckts_create(void)
{
	int err = ppp_ctrl_modem_sckt_create();

	if (err) {
		mosh_error("Cannot create modem sckt for ppp: %d", err);
		return err;
	}
	k_sem_give(&msocket_sem);

	err = ppp_ctrl_zephyr_sckt_create();
	if (err) {
		mosh_error("Cannot create zephyr sckt for ppp: %d", err);
		return err;
	}
	k_sem_give(&zsocket_sem);
	return 0;
}


void ppp_ctrl_close_sckts(void)
{

	k_sem_reset(&msocket_sem);
	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_print("Closing PPP modem sckt");
		(void)close(ppp_modem_data_socket_fd);
		ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
	}
	k_sem_reset(&zsocket_sem);
	if (ppp_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_print("Closing PPP zephyr sckt");
		(void)close(ppp_data_socket_fd);
		ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
	}
}

/******************************************************************************/

int ppp_ctrl_start(void)
{
	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		mosh_warn("PPP already up.\n");
		goto return_error;
	}

	if (ppp_ctrl_start_net_if()) {
		goto return_error;
	}

	if (ppp_ctrl_sckts_create()) {
		goto return_error;
	}

	return 0;

return_error:
	return -1;
}

/******************************************************************************/

void ppp_ctrl_stop(void)
{
	ppp_ctrl_stop_net_if();
	ppp_ctrl_close_sckts();
}
