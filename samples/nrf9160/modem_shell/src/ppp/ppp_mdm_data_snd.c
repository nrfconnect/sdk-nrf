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

#include <posix/poll.h>
#include <posix/sys/socket.h>
#include <shell/shell.h>

#include "ppp_ctrl.h"

#define UPLINK_DATA_CLONE_TIMEOUT K_MSEC(1000)

/* ppp globals: */
extern struct net_if *ppp_iface_global;
extern const struct shell *shell_global;
extern int ppp_modem_data_socket_fd;
extern int ppp_data_socket_fd;

/* Local defines: */
#define PPP_MODEM_DATA_SND_THREAD_STACK_SIZE 1024

#define PPP_MODEM_DATA_SND_THREAD_PRIORITY K_PRIO_COOP(10) /* -6 */

#define PPP_MODEM_DATA_SND_POLL_TIMEOUT_MS 1000                 /* Milliseconds */
#define PPP_MODEM_DATA_SND_BUFFER_SIZE CONFIG_NET_PPP_MTU_MRU
#define PPP_MODEM_DATA_SND_PKT_BUF_ALLOC_TIMEOUT K_MSEC(500)

static uint8_t buf_tx[PPP_MODEM_DATA_SND_BUFFER_SIZE];

static void ppp_modem_ul_data_thread_handler(void)
{
	struct pollfd fds[1];
	struct net_if *iface;

	int ret = 0;
	int recv_data_len = 0;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	assert(iface != NULL);

	while (true) {
		if (ppp_data_socket_fd < 0) {
			/* No raw socket to modem in use, so no use calling poll() */
			k_sleep(K_MSEC(PPP_MODEM_DATA_SND_POLL_TIMEOUT_MS));
			continue;
		} else {
			/* Poll for the recv data from PPP to be sent to modem: */
			fds[0].fd = ppp_data_socket_fd;
			fds[0].events = POLLIN;
			fds[0].revents = 0;

			ret = poll(fds, 1, PPP_MODEM_DATA_SND_POLL_TIMEOUT_MS);
			if (ret > 0) { /* && (fds[0].revents & POLLIN) */
				/* Receive data from PPP link: */
				recv_data_len =
					recv(ppp_data_socket_fd, buf_tx,
					     PPP_MODEM_DATA_SND_BUFFER_SIZE, 0);

				if (recv_data_len > 0) {
					/* Send to modem: */
					ret = send(ppp_modem_data_socket_fd,
						   buf_tx, recv_data_len, 0);

					/* Note: no worth to handle partial sends for raw sockets */
					if (ret < 0) {
						shell_error(
							shell_global,
							"ppp_mdm_data_snd: send() failed to modem: (%d), data len: %d\n",
							-errno, recv_data_len);
					} else if (ret != recv_data_len) {
						shell_error(
							shell_global,
							"ppp_mdm_data_snd: only partially sent to modem, only %d of original %d was sent",
							ret, recv_data_len);
					}
				}
			}
		}
	}
}

K_THREAD_DEFINE(ppp_modem_ul_data_thread, PPP_MODEM_DATA_SND_THREAD_STACK_SIZE,
		ppp_modem_ul_data_thread_handler, NULL, NULL, NULL,
		PPP_MODEM_DATA_SND_THREAD_PRIORITY, 0, 0);
