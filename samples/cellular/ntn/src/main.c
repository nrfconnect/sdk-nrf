/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

LOG_MODULE_REGISTER(ntn_sample, CONFIG_NTN_SAMPLE_LOG_LEVEL);

#define UDP_IP_HEADER_SIZE 28

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
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

static K_SEM_DEFINE(data_sent, 0, 1);

/* Forward declarations */
static void server_transmission_work_fn(struct k_work *work);

/* Work item used to schedule periodic UDP transmissions. */
static K_WORK_DELAYABLE_DEFINE(server_transmission_work, server_transmission_work_fn);

static void server_transmission_work_fn(struct k_work *work)
{
	int err;
	char buffer[CONFIG_NTN_SAMPLE_DATA_UPLOAD_SIZE_BYTES] = {"\0"};

	snprintf(buffer, sizeof(buffer), "ntn sample");

	LOG_INF("Transmitting UDP payload of %d bytes to the IP address %s, port number %d",
		CONFIG_NTN_SAMPLE_DATA_UPLOAD_SIZE_BYTES + UDP_IP_HEADER_SIZE,
		CONFIG_NTN_SAMPLE_SERVER_ADDRESS_STATIC,
		CONFIG_NTN_SAMPLE_SERVER_PORT);

	err = send(fd, buffer, sizeof(buffer), 0);
	if (err < 0) {
		LOG_ERR("Failed to transmit UDP packet, %d", -errno);
		return;
	}

	k_sem_give(&data_sent);
}

static int server_addr_construct(void)
{
	int err;

	struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_NTN_SAMPLE_SERVER_PORT);

	err = inet_pton(AF_INET, CONFIG_NTN_SAMPLE_SERVER_ADDRESS_STATIC, &server4->sin_addr);
	if (err < 0) {
		LOG_ERR("inet_pton, error: %d", -errno);
		return err;
	}

	return 0;
}

static void on_net_event_l4_connected(void)
{
	int err;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		LOG_ERR("Failed to create UDP socket: %d", -errno);
		FATAL_ERROR();
		return;
	}

	err = connect(fd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr_in));
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
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
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

	LOG_INF("NTN sample has started");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	err = server_addr_construct();
	if (err) {
		LOG_INF("server_addr_construct, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	/* Get references to both interfaces */
	struct net_if *lte_if = net_if_get_by_index(net_if_get_by_name("cell_if"));
	struct net_if *ntn_if = net_if_get_by_index(net_if_get_by_name("ntn_if"));

	if (!lte_if || !ntn_if) {
		LOG_ERR("Failed to get network interfaces");
		FATAL_ERROR();
		return -ENODEV;
	}

	LOG_INF("Cellular interface found: %p, NTN interface found: %p", lte_if, ntn_if);

	LOG_INF("Bringing up Cellular network interface");

	/* Bring up cellular interface */
	err = net_if_up(lte_if);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	/* Connect to the cellular interface */
	err = conn_mgr_if_connect(lte_if);
	if (err) {
		LOG_ERR("conn_mgr_if_connect cellular, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	k_sem_take(&data_sent, K_FOREVER);

	LOG_INF("Going offline while keeping terrestrial registration context");

	/* Disconnect from the LTE cellular interface */
	err = conn_mgr_if_disconnect(lte_if);
	if (err) {
		LOG_ERR("conn_mgr_if_disconnect LTE, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	LOG_INF("Bringing up NTN network interface");

	/* Bring up NTN interface */
	err = net_if_up(ntn_if);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up NTN, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	/* Connect to the NTN cellular interface */
	err = conn_mgr_if_connect(ntn_if);
	if (err) {
		LOG_ERR("conn_mgr_if_connect NTN, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	k_sem_take(&data_sent, K_FOREVER);

	/* Put the NTN cellular interface to dormant mode */
	err = conn_mgr_if_disconnect(ntn_if);
	if (err) {
		LOG_ERR("conn_mgr_if_disconnect NTN, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	LOG_INF("Going offline while keeping NTN registration context");

	LOG_INF("Finished");


	return 0;
}
