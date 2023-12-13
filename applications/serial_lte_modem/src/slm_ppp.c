/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "slm_ppp.h"
#include "slm_at_host.h"
#include "slm_util.h"
#include <modem/pdn.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ppp.h>
#include <zephyr/posix/sys/socket.h>
#include <assert.h>

LOG_MODULE_REGISTER(slm_ppp, CONFIG_SLM_LOG_LEVEL);

static const struct device *ppp_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ppp_uart));
static int ppp_iface_idx;
static struct net_if *ppp_iface;

static uint8_t ppp_data_buf[1500];
static struct sockaddr_ll ppp_zephyr_dst_addr;

static struct k_thread ppp_data_passing_thread_id;
static K_THREAD_STACK_DEFINE(ppp_data_passing_thread_stack, KB(2));
static void ppp_data_passing_thread(void*, void*, void*);

/* We use only the default PDP context. */
enum { PDP_CID = 0 };

enum {
	ZEPHYR_FD_IDX, /* Raw Zephyr socket to pass data to/from the PPP link. */
	MODEM_FD_IDX, /* Raw modem socket to pass data to/from the LTE link. */
	PPP_FDS_COUNT
};
const char *const ppp_socket_names[PPP_FDS_COUNT] = {
	[ZEPHYR_FD_IDX] = "Zephyr",
	[MODEM_FD_IDX] = "modem"
};
static int ppp_fds[PPP_FDS_COUNT] = { -1, -1 };

static bool open_ppp_sockets(void)
{
	int ret;

	ppp_fds[ZEPHYR_FD_IDX] = socket(AF_PACKET, SOCK_RAW | SOCK_NATIVE, IPPROTO_RAW);
	if (ppp_fds[ZEPHYR_FD_IDX] < 0) {
		LOG_ERR("Zephyr socket creation failed (%d).", errno);
		return false;
	}

	ppp_zephyr_dst_addr = (struct sockaddr_ll){
		.sll_family = AF_PACKET,
		.sll_ifindex = ppp_iface_idx
	};
	ret = bind(ppp_fds[ZEPHYR_FD_IDX],
		   (const struct sockaddr *)&ppp_zephyr_dst_addr, sizeof(ppp_zephyr_dst_addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind Zephyr socket (%d).", errno);
		return false;
	}

	ppp_fds[MODEM_FD_IDX] = socket(AF_PACKET, SOCK_RAW, 0);
	if (ppp_fds[MODEM_FD_IDX] < 0) {
		LOG_ERR("Modem socket creation failed (%d).", errno);
		return false;
	}

	return true;
}

static void close_ppp_sockets(void)
{
	for (size_t i = 0; i != ARRAY_SIZE(ppp_fds); ++i) {
		if (ppp_fds[i] < 0) {
			continue;
		}
		if (close(ppp_fds[i])) {
			LOG_WRN("Failed to close %s socket (%d).",
				ppp_socket_names[i], errno);
		}
		ppp_fds[i] = -1;
	}
}

static bool configure_ppp_link_ip_addresses(struct ppp_context *ctx)
{
	char addr4[INET_ADDRSTRLEN];
	char addr6[INET6_ADDRSTRLEN];

	util_get_ip_addr(PDP_CID, addr4, addr6);

	if (*addr4) {
		if (inet_pton(AF_INET, addr4, &ctx->ipcp.my_options.address) != 1) {
			return false;
		}
	} else if (!*addr6) {
		LOG_ERR("No connectivity.");
		return false;
	}
	if (*addr6) {
		struct in6_addr in6;
		struct ipv6cp_options *const opts = &ctx->ipv6cp.my_options;

		if (inet_pton(AF_INET6, addr6, &in6) != 1) {
			return false;
		}
		/* The interface identifier is the last 64 bits of the IPv6 address. */
		BUILD_ASSERT(sizeof(in6) >= sizeof(opts->iid));
		/* Setting it doesn't seem to change anything in practice, but this
		 * is what it's meant for, so fill it anyway.
		 */
		memcpy(opts->iid, (uint8_t *)(&in6 + 1) - sizeof(opts->iid), sizeof(opts->iid));
	}
	return true;
}

static int slm_ppp_start(void)
{
	int ret;
	unsigned int mtu;
	struct ppp_context *const ctx = net_if_l2_data(ppp_iface);

	if (!configure_ppp_link_ip_addresses(ctx)) {
		return -EADDRNOTAVAIL;
	}

	ret = pdn_dynamic_params_get(PDP_CID, &ctx->ipcp.my_options.dns1_address,
				     &ctx->ipcp.my_options.dns2_address, &mtu);
	if (ret) {
		return ret;
	}

	/* Set the PPP MTU to that of the LTE link. */
	mtu = MIN(mtu, sizeof(ppp_data_buf));
	net_if_set_mtu(ppp_iface, mtu);
	LOG_DBG("MTU set to %u.", mtu);

	ret = net_if_up(ppp_iface);
	if (ret) {
		LOG_ERR("Failed to bring PPP interface up (%d).", ret);
		return ret;
	}

	if (!open_ppp_sockets()) {
		close_ppp_sockets();
		net_if_down(ppp_iface);
		return -ENOTCONN;
	}

	LOG_INF("PPP started.");

	k_thread_create(&ppp_data_passing_thread_id, ppp_data_passing_thread_stack,
			K_THREAD_STACK_SIZEOF(ppp_data_passing_thread_stack),
			ppp_data_passing_thread, NULL, NULL, NULL,
			K_PRIO_COOP(10), 0, K_NO_WAIT);
	k_thread_name_set(&ppp_data_passing_thread_id, "ppp_data_passing");

	return 0;
}

/** @return Whether PPP is fully started. */
static bool slm_ppp_is_started(void)
{
	return net_if_flag_is_set(ppp_iface, NET_IF_UP)
	    && ppp_fds[ZEPHYR_FD_IDX] >= 0
	    && ppp_fds[MODEM_FD_IDX] >= 0;
}

static void slm_ppp_stop(void)
{
	if (!slm_ppp_is_started()) {
		return;
	}
	/* First bring the interface down so that slm_ppp_is_started()
	 * returns false right away. This is to prevent trying to stop PPP at the
	 * same time from multiple sources: the original one (e.g. "AT#XPPP=0")
	 * and the data passing thread. The latter attempts to stop PPP when it receives
	 * an error on some of the sockets, which happens when they are closed.
	 */
	const int ret = net_if_down(ppp_iface);

	if (ret) {
		LOG_WRN("Failed to bring PPP interface down (%d).", ret);
	}

	close_ppp_sockets();

	k_thread_join(&ppp_data_passing_thread_id, K_SECONDS(1));

	LOG_INF("PPP stopped.");
}

int slm_ppp_init(void)
{
	if (!device_is_ready(ppp_uart_dev)) {
		return -EAGAIN;
	}

	ppp_iface_idx = net_if_get_by_name(CONFIG_NET_PPP_DRV_NAME);
	ppp_iface = net_if_get_by_index(ppp_iface_idx);
	if (!ppp_iface) {
		LOG_ERR("PPP network interface not found (%d).", ppp_iface_idx);
		return -ENODEV;
	}
	net_if_flag_set(ppp_iface, NET_IF_POINTOPOINT);

	LOG_DBG("PPP initialized.");
	return 0;
}

/* Handles AT#XPPP commands. */
int handle_at_ppp(enum at_cmd_type cmd_type)
{
	int ret;
	unsigned int op;
	enum {
		OP_STOP,
		OP_RESTART,
		OP_COUNT
	};

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND
	 || at_params_valid_count_get(&slm_at_param_list) != 2) {
		return -EINVAL;
	}

	ret = at_params_unsigned_int_get(&slm_at_param_list, 1, &op);
	if (ret) {
		return ret;
	} else if (op >= OP_COUNT) {
		return -EINVAL;
	}

	slm_ppp_stop();
	if (op == OP_RESTART) {
		return slm_ppp_start();
	}
	assert(op == OP_STOP);
	return 0;
}

static void ppp_data_passing_thread(void*, void*, void*)
{
	const size_t mtu = net_if_get_mtu(ppp_iface);
	struct pollfd fds[PPP_FDS_COUNT];

	for (size_t i = 0; i != ARRAY_SIZE(fds); ++i) {
		fds[i].fd = ppp_fds[i];
		fds[i].events = POLLIN;
	}

	while (true) {
		const int poll_ret = poll(fds, ARRAY_SIZE(fds), -1);

		if (poll_ret <= 0) {
			LOG_ERR("Sockets polling failed (%d, %d).", poll_ret, errno);
			slm_ppp_stop();
			return;
		}

		for (size_t src = 0; src != ARRAY_SIZE(fds); ++src) {
			const short revents = fds[src].revents;

			if (!revents) {
				continue;
			}
			if (!(revents & POLLIN)) {
				/* POLLERR/POLLNVAL happen when the sockets are closed
				 * or when the connection goes down.
				 */
				if ((revents ^ POLLERR) && (revents ^ POLLNVAL)) {
					LOG_WRN("Unexpected event 0x%x on %s socket.",
						revents, ppp_socket_names[src]);
				}
				slm_ppp_stop();
				return;
			}
			const ssize_t len = recv(fds[src].fd, ppp_data_buf, mtu, MSG_DONTWAIT);

			if (len <= 0) {
				if (len != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
					LOG_ERR("Failed to receive data from %s socket (%d, %d).",
						ppp_socket_names[src], len, errno);
				}
				continue;
			}
			ssize_t send_ret;
			const size_t dst = (src == ZEPHYR_FD_IDX) ? MODEM_FD_IDX : ZEPHYR_FD_IDX;
			void *dst_addr = (dst == MODEM_FD_IDX) ? NULL : &ppp_zephyr_dst_addr;
			socklen_t addrlen = (dst == MODEM_FD_IDX) ? 0 : sizeof(ppp_zephyr_dst_addr);

			LOG_DBG("Forwarding %zd bytes to %s socket.", len, ppp_socket_names[dst]);

			send_ret = sendto(fds[dst].fd, ppp_data_buf, len, 0, dst_addr, addrlen);
			if (send_ret == -1) {
				LOG_ERR("Failed to send %zd bytes to %s socket (%d).",
					len, ppp_socket_names[dst], errno);
			} else if (send_ret != len) {
				LOG_ERR("Only sent %zd out of %zd bytes to %s socket.",
					send_ret, len, ppp_socket_names[dst]);
			} else {
				LOG_DBG("Forwarded %zd bytes to %s socket.",
					send_ret, ppp_socket_names[dst]);
			}
		}
	}
}
