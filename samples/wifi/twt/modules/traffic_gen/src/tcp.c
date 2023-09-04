/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief TCP Traffic generator
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(traffic_gen, CONFIG_TRAFFIC_GEN_LOG_LEVEL);

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
#include <zephyr/drivers/gpio.h>

#include "net_private.h"

#include <zephyr/net/socket.h>
#include "traffic_gen.h"

#define BUFFER_SIZE CONFIG_TRAFFIC_GEN_PAYLOAD_SIZE
#define MAX_EMPTY_MSG_LOOP_CNT 5

struct traffic_gen_report remote_report;

int init_tcp_client(struct traffic_gen_config *tg_config)
{
	int sockfd = 0;

	/* Create tcp socket */
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		LOG_ERR("Failed to create tcp client socket %d", errno);
		return -errno;
	}

	return sockfd;
}

int send_tcp_uplink_traffic(struct traffic_gen_config *tg_config)
{
	char empty_data = '\0';
	int ret = 0, bytes;
	struct sockaddr_in server_addr;
	int total_duration = tg_config->duration * 1000; /* Converting into ms */
	int prev_packet_time_ms = 0, current_packet_time_ms, jitter;
	int jitter_sum_ms = 0;
	uint32_t current_time, start_time;
	int elapsed_time = 0, throughput_kbps = 0;

	/* Set server address */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(tg_config->port);
	if (inet_pton(AF_INET, tg_config->server_ip, &server_addr.sin_addr) <= 0) {
		LOG_ERR("Invalid IPV4 address format");
		close(tg_config->data_sock_fd);
		return -errno;
	}

	/* Connect to the server */
	ret = connect(tg_config->data_sock_fd,
				  (struct sockaddr *)&server_addr,
				  sizeof(server_addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect TCP remote (%s): %s\n", tg_config->server_ip,
				strerror(errno));
		close(tg_config->data_sock_fd);
		return -errno;
	}

	LOG_INF("Sending TCP uplink Traffic");

	memset(tg_config->buffer, 'A', BUFFER_SIZE);
	memset(&remote_report, 0, sizeof(struct traffic_gen_report));

	/* returns time in ms from the boot */
	start_time = k_uptime_get_32();

	while ((k_uptime_get_32() - start_time) < total_duration) {
		bytes = send(tg_config->data_sock_fd, tg_config->buffer, tg_config->payload_len, 0);
		if (bytes < 0) {
			LOG_INF("Failed to send TWT uplink traffic %d", errno);
			break;
		}

		remote_report.bytes_received += bytes;
		remote_report.packets_received++;

		/* Calculate throughput */
		current_time = k_uptime_get_32();
		elapsed_time = (current_time - start_time) / 1000; /* Convert to seconds */

		/* Calculate jitter */
		current_packet_time_ms = current_time;
		jitter = current_packet_time_ms - prev_packet_time_ms;
		jitter_sum_ms += jitter;
		prev_packet_time_ms = current_packet_time_ms;
		k_sleep(K_USEC(10));
	}

	if (remote_report.packets_received > 1) {
		int average_jitter_ms = jitter_sum_ms / (remote_report.packets_received - 1);

		throughput_kbps = (remote_report.bytes_received * 8) / (elapsed_time * 1000);
		remote_report.average_jitter = average_jitter_ms;
		remote_report.elapsed_time = elapsed_time;
		remote_report.throughput = throughput_kbps;
	} else {
		remote_report.average_jitter = 0;
	}

	/* Send empty msg to the server to indicate end of TX */
	k_sleep(K_SECONDS(1));

	for (int i = 0; i <= MAX_EMPTY_MSG_LOOP_CNT; i++) {
		ret = send(tg_config->data_sock_fd, &empty_data, 0, 0);
		if (ret < 0) {
			LOG_ERR("Failed to send TWT config info to TWT server %d", errno);
			return -errno;
		}
		k_sleep(K_MSEC(200));
	}

	/* Close data socket */
	close(tg_config->data_sock_fd);

	LOG_INF("Sent TCP uplink traffic for %d sec", tg_config->duration);

	return 0;
}

int init_tcp_server(struct traffic_gen_config *tg_config)
{
	int sockfd = 0;
	struct sockaddr_in server_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		LOG_ERR("Failed to create tcp server socket %d", errno);
		return -errno;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(tg_config->port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LOG_ERR("Bind failed");
		close(sockfd);
		return -errno;
	}

	if (listen(sockfd, 3) < 0) {
		LOG_ERR("Failed to listen on TCP socket %d", errno);
		close(sockfd);
		return -errno;
	}

	LOG_INF("TCP server is listening on port %d", tg_config->port);

	return sockfd;
}

int recv_tcp_downlink_traffic(struct traffic_gen_config *tg_config)
{
	int sockfd, bytes;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int prev_packet_time_ms = 0, current_packet_time_ms, jitter;
	int jitter_sum_ms = 0;
	uint32_t current_time, start_time;
	int elapsed_time = 0, throughput_kbps = 0;

	sockfd = accept(tg_config->data_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (sockfd < 0) {
		LOG_INF("Accept failed\n");
		close(tg_config->data_sock_fd);
		return -errno;
	}

	memset(&remote_report, 0, sizeof(struct traffic_gen_report));
	start_time = k_uptime_get_32();
	while (1) {
		bytes = recv(sockfd, tg_config->buffer, BUFFER_SIZE, 0);
		if (bytes <= 0) {
			LOG_INF("Finished TCP downlink traffic ");
			break;
		}

		remote_report.bytes_received += bytes;
		remote_report.packets_received++;

		/* Calculate throughput */
		current_time = k_uptime_get_32();
		elapsed_time = (current_time - start_time) / 1000; /* Convert to seconds */

		/* Calculate jitter */
		current_packet_time_ms = current_time;
		jitter = current_packet_time_ms - prev_packet_time_ms;
		jitter_sum_ms += jitter;
		prev_packet_time_ms = current_packet_time_ms;
	}

	if (remote_report.packets_received > 1) {
		int average_jitter_ms = jitter_sum_ms / (remote_report.packets_received - 1);

		throughput_kbps = (remote_report.bytes_received * 8) / (elapsed_time * 1000);
		remote_report.average_jitter = average_jitter_ms;
		remote_report.elapsed_time = elapsed_time;
		remote_report.throughput = throughput_kbps;
	} else {
		remote_report.average_jitter = 0;
	}

	close(sockfd);
	return 0;
}
