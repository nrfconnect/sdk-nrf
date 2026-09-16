/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

LOG_MODULE_REGISTER(udp_sample, CONFIG_UDP_SAMPLE_LOG_LEVEL);

#define UDP_IP_HEADER_SIZE_IPV4 28
#define UDP_IP_HEADER_SIZE_IPV6 48

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK_IPV4 (NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED)
#define L4_EVENT_MASK_IPV6 (NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

/* Variables used to perform socket operations. */
static int fd;
static struct sockaddr_storage host_addr;
static sa_family_t host_addr_family;

/* Forward declarations */
static void server_transmission_work_fn(struct k_work *work);

/* Work item used to schedule periodic UDP transmissions. */
static K_WORK_DELAYABLE_DEFINE(server_transmission_work, server_transmission_work_fn);

static void server_transmission_work_fn(struct k_work *work)
{
	int err;
	char buffer[CONFIG_UDP_SAMPLE_DATA_UPLOAD_SIZE_BYTES] = {"\0"};
	const int ip_header_size = (host_addr_family == AF_INET6) ?
				    UDP_IP_HEADER_SIZE_IPV6 : UDP_IP_HEADER_SIZE_IPV4;

	LOG_INF("Transmitting UDP/IP payload of %d bytes to the "
		"IP address %s, port number %d",
		CONFIG_UDP_SAMPLE_DATA_UPLOAD_SIZE_BYTES + ip_header_size,
		CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC,
		CONFIG_UDP_SAMPLE_SERVER_PORT);

	err = send(fd, buffer, sizeof(buffer), 0);
	if (err < 0) {
		LOG_ERR("Failed to transmit UDP packet, %d", -errno);
		return;
	}

	(void)k_work_reschedule(&server_transmission_work,
				K_SECONDS(CONFIG_UDP_SAMPLE_DATA_UPLOAD_FREQUENCY_SECONDS));
}

/* Determine IP address family and construct the server address structure. */
static int server_addr_construct(void)
{
	bool ret;

	ret = net_ipaddr_parse(CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC,
				strlen(CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC),
				(struct sockaddr *)&host_addr);
	if (!ret) {
		LOG_ERR("net_ipaddr_parse, failed to parse %s",
			CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC);
		return -EINVAL;
	}

	host_addr_family = ((struct sockaddr *)&host_addr)->sa_family;

	if (host_addr_family == AF_INET6) {
		struct sockaddr_in6 *server6 = ((struct sockaddr_in6 *)&host_addr);

		if (server6->sin6_port == 0) {
			server6->sin6_port = htons(CONFIG_UDP_SAMPLE_SERVER_PORT);
		}
	} else {
		struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

		if (server4->sin_port == 0) {
			server4->sin_port = htons(CONFIG_UDP_SAMPLE_SERVER_PORT);
		}
	}

	return 0;
}

static void on_net_event_l4_connected(void)
{
	int err;

	socklen_t addr_len = (host_addr_family == AF_INET6) ?
				sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

	fd = socket(host_addr_family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		LOG_ERR("Failed to create UDP socket: %d", -errno);
		FATAL_ERROR();
		return;
	}

	err = connect(fd, (struct sockaddr *)&host_addr, addr_len);
	if (err < 0) {
		LOG_ERR("connect, error: %d", -errno);
		FATAL_ERROR();
		return;
	}

	(void)k_work_reschedule(&server_transmission_work, K_NO_WAIT);
}

static void on_net_event_l4_disconnected(void)
{
	(void)close(fd);
	(void)k_work_cancel_delayable(&server_transmission_work);
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_IPV4_CONNECTED:
	case NET_EVENT_L4_IPV6_CONNECTED:
		LOG_INF("Network connectivity established");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_IPV4_DISCONNECTED:
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		on_net_event_l4_disconnected();
		break;
	default:
		/* Don't care */
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

int main(void)
{
	int err;

	LOG_INF("UDP sample has started");

	err = server_addr_construct();
	if (err) {
		LOG_INF("server_addr_construct, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler,
		(host_addr_family == AF_INET6) ? L4_EVENT_MASK_IPV6 : L4_EVENT_MASK_IPV4);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	/* Resend connection status if the sample is built for NATIVE SIM.
	 * This is necessary because the network interface is automatically brought up
	 * at SYS_INIT() before main() is called.
	 * This means that either L4 connected events fire before the
	 * appropriate handler l4_event_handler() is registered.
	 */
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM)) {
		conn_mgr_mon_resend_status();
	}

	return 0;
}
