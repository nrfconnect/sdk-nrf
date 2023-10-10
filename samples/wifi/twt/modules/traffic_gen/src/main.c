/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Traffic generator for test Wi-Fi
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(traffic_gen, CONFIG_TRAFFIC_GEN_LOG_LEVEL);

#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/drivers/gpio.h>

#include "net_private.h"
#include "traffic_gen.h"
#include "traffic_gen_tcp.h"

#define TRAFFIC_GEN_CTRL_PORT 6666
#define REPORT_BUFFER_SIZE 100
#define REPORT_TIMEOUT 20

unsigned char report_buffer[REPORT_BUFFER_SIZE];

void traffic_gen_get_report(struct traffic_gen_config *tg_config)
{
	struct traffic_gen_report *report = (struct traffic_gen_report *)report_buffer;

	if (tg_config->role == TRAFFIC_GEN_ROLE_CLIENT) {
		if (!tg_config->traffic_gen_report_received) {
			memcpy(report_buffer,
				   (uint8_t *)&remote_report,
				   sizeof(struct traffic_gen_report));
			LOG_ERR("Server Report not received");
			LOG_INF("Printing Client Report:");
		} else {
			LOG_INF("Server Report:");
			LOG_INF("\t Total Bytes Received  : %-15u", ntohl(report->bytes_received));
			LOG_INF("\t Total Packets Received: %-15u",
					ntohl(report->packets_received));
			LOG_INF("\t Elapsed Time          : %-15u", ntohl(report->elapsed_time));
			LOG_INF("\t Throughput (Kbps)     : %-15u", ntohl(report->throughput));
			LOG_INF("\t Average Jitter (ms)   : %-15u", ntohl(report->average_jitter));
		}
	}

	LOG_INF("Client Report:");
	LOG_INF("\t Total Bytes Received  : %-15u", (report->bytes_received));
	LOG_INF("\t Total Packets Received: %-15u", (report->packets_received));
	LOG_INF("\t Elapsed Time          : %-15u", (report->elapsed_time));
	LOG_INF("\t Throughput (Kbps)     : %-15u", (report->throughput));
	LOG_INF("\t Average Jitter (ms)   : %-15u", (report->average_jitter));
}

int traffic_gen_wait_for_report(struct traffic_gen_config *tg_config)
{
	int ret = 0, bytes_received = 0;
	struct timeval timeout;

	tg_config->traffic_gen_report_received = 0;
	memset(report_buffer, 0, REPORT_BUFFER_SIZE);

	LOG_DBG("Waiting for report from the Server");

	/* Wait for a response upto the REPORT_TIMEOUT from the server */
	timeout.tv_sec = REPORT_TIMEOUT;
	timeout.tv_usec = 0;

	ret = setsockopt(tg_config->ctrl_sock_fd,
					 SOL_SOCKET,
					 SO_RCVTIMEO,
					 &timeout,
					 sizeof(timeout));
	if (ret < 0) {
		LOG_ERR("Failed to set socket option");
		return -errno;
	}

	if (tg_config->role == TRAFFIC_GEN_ROLE_CLIENT) {
		bytes_received = recv(tg_config->ctrl_sock_fd,
							  report_buffer,
							  REPORT_BUFFER_SIZE,
							  0);
	} else {
		memcpy(report_buffer,
			   (uint8_t *)&remote_report,
			   sizeof(struct traffic_gen_report));
	}

	if (bytes_received > 0) {
		LOG_DBG("Received server report");
		tg_config->traffic_gen_report_received = 1;
	} else if (bytes_received == 0) {
		LOG_ERR("Server report not received and connection closed by peer");
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		LOG_WRN("Timeout: Server report not received within %d seconds",
			    REPORT_TIMEOUT);
	} else {
		LOG_ERR("Server report recv failed: %d", errno);
	}

	close(tg_config->ctrl_sock_fd);

	return 0;
}

static int connect_to_server(struct traffic_gen_config *tg_config)
{
	struct sockaddr_in serv_addr;
	int sockfd;

	/* Create control path socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		LOG_ERR("Failed to create control path socket");
		return -errno;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TRAFFIC_GEN_CTRL_PORT);

	/* Convert IPv4 addresses from text to binary form */
	if (inet_pton(AF_INET, tg_config->server_ip, &serv_addr.sin_addr) <= 0) {
		LOG_ERR("Invalid address: Address not supported");
		close(sockfd);
		return -errno;
	}

	/* Connect to the TRAFFIC_GEN server */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		LOG_ERR("Failed to connect TRAFFIC_GEN server");
		LOG_ERR("Run the TRAFFIC_GEN server in other end before running TRAFFIC_GEN app");
		close(sockfd);
		return -errno;
	}

	LOG_DBG("Connected To TRAFFIC_GEN Server!!!");
	k_sleep(K_SECONDS(3));

	return sockfd;
}

static int send_config_info_to_server(struct traffic_gen_config *tg_config)
{
	int ret;
	struct server_config config;

	config.role = htonl(tg_config->role);
	config.type = htonl(tg_config->type);
	config.mode = htonl(tg_config->mode);
	config.duration = htonl(tg_config->duration);
	config.payload_len = htonl(tg_config->payload_len);

	ret = send(tg_config->ctrl_sock_fd, &config, sizeof(struct server_config), 0);
	if (ret < 0) {
		LOG_ERR("Failed to send TRAFFIC_GEN config info to TRAFFIC_GEN server %d", errno);
		return -errno;
	}

	LOG_DBG("Config info sent to the TRAFFIC_GEN server");

	return 1;
}

static int setup_ctrl_path(struct traffic_gen_config *tg_config)
{
	int ret;

	/* Connect to TRAFFIC_GEN server:
	 *  - send config info from TRAFFIC_GEN client to TRAFFIC_GEN server
	 *  - configure TRAFFIC_GEN server, as per config info from TRAFFIC_GEN client
	 *  - after traffic completeion wait for the TRAFFIC_GEN server report
	 */
	ret = connect_to_server(tg_config);
	if (ret < 0) {
		return -1;
	}

	tg_config->ctrl_sock_fd = ret;

	/* send config info to server */
	ret = send_config_info_to_server(tg_config);
	if (ret < 0) {
		return -1;
	}

	return 1;
}

/* Create uplink/downlink datapath socket */
static int setup_data_path(struct traffic_gen_config *tg_config)
{
	int ret = 0;

	if (tg_config->role == TRAFFIC_GEN_ROLE_CLIENT) {
		if (tg_config->type == TRAFFIC_GEN_TYPE_TCP) {
			ret = init_tcp_client(tg_config);
			if (ret < 0) {
				return -1;
			}
			tg_config->data_sock_fd = ret;
		} else {
			LOG_ERR("Unsupported Traffic type %d", tg_config->type);
			return -1;
		}
	} else if (tg_config->role == TRAFFIC_GEN_ROLE_SERVER) {
		if (tg_config->type == TRAFFIC_GEN_TYPE_TCP) {
			ret = init_tcp_server(tg_config);
			if (ret < 0) {
				return -1;
			}
			tg_config->data_sock_fd = ret;
		} else {
			LOG_ERR("Unsupported Traffic type %d", tg_config->type);
			return -1;
		}
	} else {
		LOG_ERR("Invalid role, please use only Client/Server");
		return -1;
	}

	return ret;
}

static int start_role(struct traffic_gen_config *tg_config)
{
	if (tg_config->role == TRAFFIC_GEN_ROLE_CLIENT) {
		if (tg_config->type == TRAFFIC_GEN_TYPE_TCP) {
			return send_tcp_uplink_traffic(tg_config);
		}
	} else if (tg_config->role == TRAFFIC_GEN_ROLE_SERVER) {
		if (tg_config->type == TRAFFIC_GEN_TYPE_TCP) {
			return recv_tcp_downlink_traffic(tg_config);
		}
	}

	return 0;
}

void traffic_gen_pause(struct traffic_gen_config *tg_config)
{
	tg_config->pause_traffic = true;
}

void traffic_gen_resume(struct traffic_gen_config *tg_config)
{
	tg_config->pause_traffic = false;
}

int traffic_gen_start(struct traffic_gen_config *tg_config)
{
	int ret;

	/* Create control path socket */
	ret = setup_ctrl_path(tg_config);
	if (ret < 0) {
		LOG_ERR("Failed to setup control path");
		return -1;
	}

	/* Wait for server configuration
	 *  - could be client mode
	 *  - could be server mode
	 *  - server will create data socket
	 *  - do uplink/downlink traffic based traffic configuration
	 */
	k_sleep(K_SECONDS(5));

	/* Create data path socket for uplink/downlink traffic */
	ret = setup_data_path(tg_config);
	if (ret < 0) {
		LOG_ERR("Failed to setup data path");
		close(tg_config->ctrl_sock_fd);
		return -1;
	}

	return start_role(tg_config);
}

void traffic_gen_init(struct traffic_gen_config *tg_config)
{
	memset((unsigned char *)tg_config, 0, sizeof(struct traffic_gen_config));

#ifdef CONFIG_TRAFFIC_GEN_CLIENT
	tg_config->role = TRAFFIC_GEN_ROLE_CLIENT;
	LOG_DBG("TRAFFIC_GEN ROLE: Client");
#elif CONFIG_TRAFFIC_GEN_SERVER
	tg_config->role = TRAFFIC_GEN_ROLE_SERVER;
	LOG_DBG("TRAFFIC_GEN ROLE: Server");
#else
	LOG_ERR("Configure TRAFFIC_GEN app as either client/server");
	return;
#endif
#ifdef CONFIG_TRAFFIC_GEN_TYPE_TCP
	tg_config->type = TRAFFIC_GEN_TYPE_TCP;
#else
	LOG_ERR("Only TCP traffic is supported");
#endif
	BUILD_ASSERT(strlen(CONFIG_TRAFFIC_GEN_REMOTE_IPV4_ADDR) > 0,
				 "Remote IPv4 address is not configured, use TRAFFIC_GEN_REMOTE_IPV4_ADDR");
	/* TODO: Make this configurable */
	tg_config->mode = 1;
	tg_config->duration = CONFIG_TRAFFIC_GEN_DURATION;
	tg_config->payload_len = CONFIG_TRAFFIC_GEN_PAYLOAD_SIZE;
	tg_config->server_ip = CONFIG_TRAFFIC_GEN_REMOTE_IPV4_ADDR;
	tg_config->port = CONFIG_TRAFFIC_GEN_REMOTE_PORT_NUM;
	tg_config->pause_traffic = false;
}
