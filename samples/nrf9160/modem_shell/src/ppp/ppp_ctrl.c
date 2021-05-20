/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <assert.h>

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

#include "link_api.h"

#include "ppp_ctrl.h"

/* ppp globals: */
struct net_if *ppp_iface_global;
extern const struct shell *shell_global;

/* Socket to send and recv data to/from modem: */
int ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

/* Socket to send and recv data to/from Zephyr PPP link: */
int ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;

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
		shell_print(shell_global, "PPP carrier ON");
		return;
	}

	if (mgmt_event == NET_EVENT_PPP_CARRIER_OFF) {
		shell_print(shell_global, "PPP carrier OFF");
		return;
	}
}

static struct net_mgmt_event_callback ip_level_net_mgmt_event_cb;
static void
ppp_ctrl_net_mgmt_event_ip_levelhandler(struct net_mgmt_event_callback *cb,
					uint32_t mgmt_event,
					struct net_if *iface)
{
	if ((mgmt_event & (NET_EVENT_IPV4_ADDR_ADD |
			   NET_EVENT_IPV4_ADDR_DEL)) != mgmt_event) {
		return;
	}
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD &&
	    iface == ppp_iface_global) {
		shell_print(shell_global, "Dial up connection up");
		return;
	}
	if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL &&
	    iface == ppp_iface_global) {
		shell_print(shell_global, "Dial up connection down");
		return;
	}
}
static void ppp_ctrl_net_mgmt_events_subscribe(void)
{
	net_mgmt_init_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb,
				     ppp_ctrl_net_mgmt_event_handler,
				     (NET_EVENT_PPP_CARRIER_ON |
				      NET_EVENT_PPP_CARRIER_OFF));

	net_mgmt_init_event_callback(&ip_level_net_mgmt_event_cb,
				     ppp_ctrl_net_mgmt_event_ip_levelhandler,
				     (NET_EVENT_IPV4_ADDR_ADD |
				      NET_EVENT_IPV4_ADDR_DEL));

	net_mgmt_add_event_callback(&ppp_ctrl_net_mgmt_event_ppp_cb);
	net_mgmt_add_event_callback(&ip_level_net_mgmt_event_cb);
}

/******************************************************************************/

void ppp_ctrl_init(void)
{
	ppp_ctrl_net_mgmt_events_subscribe();
}

/******************************************************************************/

int ppp_ctrl_start(const struct shell *shell)
{
	struct ppp_context *ctx;
	struct net_if *iface;
	struct pdp_context_info *pdp_context_info;

	int idx = 0; /* Note: PPP iface index assumed to be 0 */

	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		shell_warn(shell, "PPP already up.\n");
		goto return_error;
	}

	ctx = net_ppp_context_get(idx);
	if (!ctx) {
		shell_error(shell, "PPP context not found.\n");
		goto return_error;
	}

	/* Note: no multicontext support, only at default PDN context */
	pdp_context_info = link_api_get_pdp_context_info_by_pdn_cid(0);
	if (pdp_context_info == NULL) {
		shell_error(shell, "PPP context not found.\n");
		goto return_error;
	}

	iface = ctx->iface;
	ppp_iface_global = iface;

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	memcpy(&(ctx->ipcp.my_options.address), &(pdp_context_info->ip_addr4),
	       sizeof(ctx->ipcp.my_options.address));
	memcpy(&ctx->ipcp.my_options.dns1_address,
	       &pdp_context_info->dns_addr4_primary,
	       sizeof(ctx->ipcp.my_options.dns1_address));
	memcpy(&ctx->ipcp.my_options.dns2_address,
	       &pdp_context_info->dns_addr4_secondary,
	       sizeof(ctx->ipcp.my_options.dns2_address));

	free(pdp_context_info);

	ppp_modem_data_socket_fd = socket(AF_PACKET, SOCK_RAW, 0);
	if (ppp_modem_data_socket_fd < 0) {
		shell_error(shell,
			    "modem data socket creation failed: (%d)!!!!\n",
			    -errno);
		goto return_error;
	} else {
		shell_info(shell, "modem data socket %d created for modem data",
			   ppp_modem_data_socket_fd);
	}

#ifdef SO_SNDTIMEO
	struct timeval tv;

	/* blocking socket and we do not want to block for long: 3 sec timeout for sending: */
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(ppp_modem_data_socket_fd, SOL_SOCKET, SO_SNDTIMEO,
		       (struct timeval *)&tv, sizeof(struct timeval)) < 0) {
		shell_error(shell, "Unable to set socket SO_SNDTIMEO");
	}
#endif

	net_if_up(iface);

	/* Create raw Zephyr socket for passing data to/from ppp link: */
	ppp_data_socket_fd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if (ppp_data_socket_fd < 0) {
		shell_error(shell,
			    "PPP data socket creation failed: (%d)!!!!\n",
			    -errno);
		goto return_error;
	} else {
		shell_info(shell, "PPP data socket %d created",
			   ppp_data_socket_fd);
	}

	/* Bind to PPP net if: */
	struct sockaddr_ll dst = { 0 };

	dst.sll_ifindex = net_if_get_by_iface(ppp_iface_global);
	dst.sll_family = AF_PACKET;

	int ret = bind(ppp_data_socket_fd, (const struct sockaddr *)&dst,
		       sizeof(struct sockaddr_ll));
	if (ret < 0) {
		shell_error(shell, "Failed to bind PPP data socket : %d",
			    errno);
		goto return_error;
	}

	return 0;

return_error:
	return -1;
}

/******************************************************************************/

void ppp_ctrl_stop(void)
{
	struct ppp_context *ctx;
	int idx = 0; /* Note: PPP iface index assumed to be 0 */

	if (ppp_modem_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		(void)close(ppp_modem_data_socket_fd);
		ppp_modem_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
	}
	if (ppp_data_socket_fd != PPP_MODEM_DATA_RAW_SCKT_FD_NONE) {
		(void)close(ppp_data_socket_fd);
		ppp_data_socket_fd = PPP_MODEM_DATA_RAW_SCKT_FD_NONE;
	}
	ctx = net_ppp_context_get(idx);
	if (!ctx && !ctx->iface) {
		return;
	}

	net_if_down(ctx->iface);
}
