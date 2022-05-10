/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <assert.h>

#include <zephyr/net/ppp.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>

#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/shell/shell.h>

#include "mosh_print.h"
#include "ppp_ctrl.h"

/* ppp globals: */
extern int ppp_modem_data_socket_fd;
extern int ppp_data_socket_fd;
extern uint16_t used_mtu_mru;

extern struct k_sem ppp_sockets_sem;

#define PPP_MODEM_DATA_RCV_THREAD_STACK_SIZE 1024

#define PPP_MODEM_DATA_RCV_THREAD_PRIORITY K_PRIO_COOP(10) /* -6 */
#define PPP_MODEM_DATA_RCV_POLL_TIMEOUT_MS 1000 /* Milliseconds */
#define PPP_MODEM_DATA_RCV_BUFFER_SIZE PPP_MODEM_DATA_RCV_SND_BUFF_SIZE

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
		if (ppp_modem_data_socket_fd < 0 || ppp_data_socket_fd < 0) {
			/* Wait for sockets to be created */
			k_sem_take(&ppp_sockets_sem, K_FOREVER);
			continue;
		}
		fds[0].fd = ppp_modem_data_socket_fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		ret = poll(fds, 1, PPP_MODEM_DATA_RCV_POLL_TIMEOUT_MS);
		if (ret > 0) { /* && (fds[0].revents & POLLIN) */
			assert(used_mtu_mru <= PPP_MODEM_DATA_RCV_BUFFER_SIZE);

			recv_data_len =
				recv(ppp_modem_data_socket_fd, receive_buffer, used_mtu_mru, 0);
			if (recv_data_len > 0) {
				ret = sendto(ppp_data_socket_fd, receive_buffer, recv_data_len, 0,
					     (const struct sockaddr *)&dst,
					     sizeof(struct sockaddr_ll));
				if (ret < 0) {
					mosh_error("%s: cannot send data from modem to PPP link "
						   "- dropped data of len %d, err %d",
						   (__func__), recv_data_len, ret);
				}
			} else {
				/* We have set SO_RCVTIMEO and
				 * thus it is normal to have an ETIMEDOUT
				 */
				if (errno != ETIMEDOUT) {
					mosh_error("%s: recv() from modem failed, %d, errno: %d",
						   (__func__), recv_data_len, -errno);
				}
			}
		} else if (ret < 0) {
			mosh_error("%s: poll() failed %d", (__func__), ret);
		}
	}
}

K_THREAD_DEFINE(ppp_modem_dl_data_thread, PPP_MODEM_DATA_RCV_THREAD_STACK_SIZE,
		ppp_modem_dl_data_thread_handler, NULL, NULL, NULL,
		PPP_MODEM_DATA_RCV_THREAD_PRIORITY, 0, 0);
