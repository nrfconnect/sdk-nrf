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

#include <posix/unistd.h>
#include <posix/netdb.h>

#include <posix/poll.h>
#include <posix/sys/socket.h>
#include <shell/shell.h>

/* ppp globals: */
extern int ppp_modem_data_socket_fd;
extern int ppp_data_socket_fd;
extern const struct shell *shell_global;

#define PPP_MODEM_DATA_RCV_THREAD_STACK_SIZE 1024

#define PPP_MODEM_DATA_RCV_THREAD_PRIORITY K_PRIO_COOP(10) /* -6 */
#define PPP_MODEM_DATA_RCV_POLL_TIMEOUT_MS 1000 /* Milliseconds */
#define PPP_MODEM_DATA_RCV_BUFFER_SIZE CONFIG_NET_PPP_MTU_MRU
#define PPP_MODEM_DATA_RCV_PKT_BUF_ALLOC_TIMEOUT K_MSEC(500)

static char receive_buffer[PPP_MODEM_DATA_RCV_BUFFER_SIZE];

static void ppp_modem_dl_data_thread_handler(void)
{
	struct pollfd fds[1];
	struct net_if *iface;

	int ret = 0;
	int recv_data_len = 0;
	struct sockaddr_ll dst = { 0 };

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	assert(iface != NULL);

	dst.sll_ifindex = net_if_get_by_iface(iface);

	while (true) {
		if (ppp_modem_data_socket_fd < 0) {
			/* No raw socket to modem in use, so no use calling poll() */
			k_sleep(K_MSEC(PPP_MODEM_DATA_RCV_POLL_TIMEOUT_MS));
			continue;
		}
		fds[0].fd = ppp_modem_data_socket_fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		ret = poll(fds, 1, PPP_MODEM_DATA_RCV_POLL_TIMEOUT_MS);
		if (ret > 0) { /* && (fds[0].revents & POLLIN) */
			recv_data_len =
				recv(ppp_modem_data_socket_fd, receive_buffer,
				     PPP_MODEM_DATA_RCV_BUFFER_SIZE, 0);
			if (recv_data_len > 0) {
				ret = sendto(ppp_data_socket_fd, receive_buffer,
					     recv_data_len, 0,
					     (const struct sockaddr *)&dst,
					     sizeof(struct sockaddr_ll));
				if (ret < 0) {
					shell_error(
						shell_global,
						"%s: cannot send data from mdm to PPP link "
						"- dropped data of len %d\n",
						(__func__), recv_data_len);
				}
			} else {
				shell_error(shell_global,
					    "%s: recv() from modem failed %d\n",
					    (__func__), recv_data_len);
			}
		} else if (ret < 0) {
			shell_error(shell_global, "%s: poll() failed %d\n",
				    (__func__), ret);
		}
	}
}

K_THREAD_DEFINE(ppp_modem_dl_data_thread, PPP_MODEM_DATA_RCV_THREAD_STACK_SIZE,
		ppp_modem_dl_data_thread_handler, NULL, NULL, NULL,
		PPP_MODEM_DATA_RCV_THREAD_PRIORITY, 0, 0);
