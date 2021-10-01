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

#include "mosh_print.h"
#include "ppp_ctrl.h"

/* ppp globals: */
extern struct net_if *ppp_iface_global;
extern int ppp_modem_data_socket_fd;
extern int ppp_data_socket_fd;
extern uint16_t used_mtu_mru;
extern struct k_sem zsocket_sem;

/* Local defines: */
#define PPP_MODEM_DATA_SND_THREAD_STACK_SIZE 1024

#define PPP_MODEM_DATA_SND_THREAD_PRIORITY K_PRIO_COOP(10) /* -6 */

#define PPP_MODEM_DATA_SND_POLL_TIMEOUT_MS 1000                 /* Milliseconds */
#define PPP_MODEM_DATA_SND_BUFFER_SIZE PPP_MODEM_DATA_RCV_SND_BUFF_SIZE
#define PPP_MODEM_DATA_SND_PKT_BUF_ALLOC_TIMEOUT K_MSEC(500)

static uint8_t buf_tx[PPP_MODEM_DATA_SND_BUFFER_SIZE];

static void ppp_modem_data_snd_error_check(void)
{
	if (errno == EINVAL || errno == ENOTCONN) {
		/* Temporarily workaround underlying issues by closing and recreating sckts */
		ppp_ctrl_close_sckts();

		/* Give some time for closing */
		k_sleep(K_SECONDS(1));

		ppp_ctrl_sckts_create();
	}
}

static void ppp_modem_ul_data_thread_handler(void)
{
	struct pollfd fds[1];
	struct net_if *iface;

	int ret = 0;
	int recv_data_len = 0;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	assert(iface != NULL);

	while (true) {
		if (ppp_data_socket_fd < 0 || ppp_modem_data_socket_fd < 0) {
			/* Wait for a socket to be created */
			k_sem_take(&zsocket_sem, K_FOREVER);
			continue;
		} else {
			/* Poll for the recv data from PPP to be sent to modem: */
			fds[0].fd = ppp_data_socket_fd;
			fds[0].events = POLLIN;
			fds[0].revents = 0;


			ret = poll(fds, 1, PPP_MODEM_DATA_SND_POLL_TIMEOUT_MS);
			if (ret > 0) { /* && (fds[0].revents & POLLIN) */
				assert(used_mtu_mru <= PPP_MODEM_DATA_SND_BUFFER_SIZE);

				/* Receive data from PPP link: */
				recv_data_len =
					recv(ppp_data_socket_fd, buf_tx, used_mtu_mru, 0);
				if (recv_data_len > 0) {
					/* Send to modem: */
					ret = send(ppp_modem_data_socket_fd,
						   buf_tx, recv_data_len, 0);

					/* Note: no worth to handle partial sends for raw sockets */
					if (ret < 0) {
						mosh_error(
							"ppp_mdm_data_snd: send() failed to modem: (%d), ret %d, data len: %d\n",
							-errno, ret, recv_data_len);
						ppp_modem_data_snd_error_check();
					} else if (ret != recv_data_len) {
						mosh_error(
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
