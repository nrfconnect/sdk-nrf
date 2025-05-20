/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zperf_utils.h"
#include <zephyr/net/zperf.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zperf_utils, CONFIG_LOG_DEFAULT_LEVEL);

void zperf_log_upload_stats(struct zperf_results *results, bool is_udp)
{
	unsigned int rate_in_kbps, client_rate_in_kbps;

	LOG_INF("Thread zperf Upload completed");

	if (results->client_time_in_us != 0U) {
		client_rate_in_kbps = (uint32_t)
			(((uint64_t)results->nb_packets_sent *
			(uint64_t)results->packet_size * (uint64_t)8 *
			(uint64_t)USEC_PER_SEC) /
			((uint64_t)results->client_time_in_us * 1024U));
	} else {
		client_rate_in_kbps = 0U;
	}

	if (is_udp) {
		if (results->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)results->total_len *
				  (uint64_t)8 * (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)results->time_in_us * 1024U));
		} else {
			rate_in_kbps = 0U;
		}

		if (!rate_in_kbps) {
			LOG_ERR("LAST PACKET NOT RECEIVED");
		}

		LOG_INF("Thread zperf UDP Statistics:\t\tserver\t\t(client)");
		LOG_INF("Duration:\t\t\t%llu us\t(%llu us)",
		results->time_in_us, results->client_time_in_us);
		LOG_INF("Rate:\t\t\t%u kbps\t\t(%u kbps)", rate_in_kbps, client_rate_in_kbps);
		LOG_INF("Num packets:\t\t%u\t\t(%u)",
		results->nb_packets_rcvd, results->nb_packets_sent);
		LOG_INF("Num packets outorder:\t%u", results->nb_packets_outorder);
		LOG_INF("Num packets lost:\t\t%u", results->nb_packets_lost);
		LOG_INF("Jitter:\t\t\t%u us", results->jitter_in_us);
	} else {
		LOG_INF("Thread zperf TCP Statistics:");
		LOG_INF("Duration:\t%llu us", results->client_time_in_us);
		LOG_INF("Rate:\t\t%u kbps", client_rate_in_kbps);
		LOG_INF("Num packets:\t%u", results->nb_packets_sent);
		LOG_INF("Num errors:\t%u (retry or fail)", results->nb_packets_errors);
	}
}

int zperf_upload(const char *peer_addr, uint32_t duration_sec,
	uint32_t packet_size_bytes,	uint32_t rate_bps, bool is_udp)
{
	struct zperf_upload_params param = { 0 };
	struct sockaddr_in6 ipv6 = { .sin6_family = AF_INET6 };
	struct sockaddr_in ipv4 = { .sin_family = AF_INET };
	struct zperf_results results = { 0 };
	int ret;
	char *port_str;

	param.options.priority = -1;
	param.duration_ms = MSEC_PER_SEC * duration_sec;
	param.packet_size = packet_size_bytes;
	param.rate_kbps = (rate_bps + 1023) / 1024;

	port_str = STRINGIFY(CONFIG_NET_CONFIG_THREAD_PORT);

	if (is_udp && !IS_ENABLED(CONFIG_NET_UDP)) {
		LOG_ERR("CONFIG_NET_UDP disabled");
		return -ENOTSUP;
	} else if (!is_udp && !IS_ENABLED(CONFIG_NET_TCP)) {
		LOG_WRN("CONFIG_NET_TCP disabled");
		return -ENOTSUP;
	}

	if (net_addr_pton(AF_INET6, peer_addr, &(ipv6.sin6_addr)) < 0) {
		if (net_addr_pton(AF_INET, peer_addr, &(ipv4.sin_addr)) < 0) {
			LOG_WRN("Invalid address %s", peer_addr);
			return -ENOEXEC;
		}

		ipv4.sin_port = htons(strtoul(port_str, NULL, 10));
		if (ipv4.sin_port) {
			LOG_WRN("Invalid port %s", port_str);
			return -EINVAL;
		}

		/* LOG_INF("Connecting to %s\n", net_sprint_ipv4_addr(&ipv4.sin_addr)); */
		memcpy(&param.peer_addr, &ipv4, sizeof(ipv4));
	} else {
		ipv6.sin6_port = htons(strtoul(port_str, NULL, 10));
		if (!ipv6.sin6_port) {
			LOG_WRN("Invalid port %s", port_str);
			return -EINVAL;
		}
		/* LOG_INF("Connecting to %s\n", net_sprint_ipv6_addr(&ipv6.sin6_addr)); */
		memcpy(&param.peer_addr, &ipv6, sizeof(ipv6));
	}

	LOG_INF("Thread zperf parameters");
	LOG_INF("Duration:\t\t%d ms", param.duration_ms);
	LOG_INF("Packet size:\t%u bytes", param.packet_size);
	LOG_INF("Rate:\t\t%u kbps", param.rate_kbps);
	LOG_INF("Starting Thread zperf upload...");

	if (is_udp) {
		ret = zperf_udp_upload(&param, &results);
		if (ret < 0) {
			LOG_ERR("Thread zperf UDP upload failed (%d)\n", ret);
			return ret;
		}
	} else {
		ret = zperf_tcp_upload(&param, &results);
		if (ret < 0) {
			LOG_ERR("Thread zperf TCP upload failed (%d)\n", ret);
			return ret;
		}
	}

	zperf_log_upload_stats(&results, is_udp);

	return 0;
}

void zperf_download_cb(enum zperf_status status,
			   struct zperf_results *result,
			   void *user_data)
{
	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("Thread zperf New session started.");
	break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
	break;
	case ZPERF_SESSION_FINISHED:
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)result->total_len * 8ULL *
				(uint64_t)USEC_PER_SEC) /
				((uint64_t)result->time_in_us * 1024ULL));
		} else {
			rate_in_kbps = 0U;
		}

		LOG_INF("Thread zperf download Session ended.");
		LOG_INF("Duration:\t\t\t%llu us", result->time_in_us);
		LOG_INF("Rate:\t\t\t%u kbps", rate_in_kbps);

		/* Applicable only for UDP cases */
		if (result->nb_packets_rcvd) {
			LOG_INF("Num packets received:\t%u", result->nb_packets_rcvd);
			LOG_INF("Num packets outorder:\t%u", result->nb_packets_outorder);
			LOG_INF("Num packets lost:\t\t%u", result->nb_packets_lost);
			LOG_INF("Jitter:\t\t\t%u us", result->jitter_in_us);
		}
	break;

	case ZPERF_SESSION_ERROR:
		LOG_ERR("Session error.");
		break;
	}
}

int zperf_download(bool is_udp)
{
	struct zperf_download_params param = { 0 };
	int ret;

	param.port = CONFIG_NET_CONFIG_THREAD_PORT;

	if (is_udp && !IS_ENABLED(CONFIG_NET_UDP)) {
		LOG_ERR("CONFIG_NET_UDP disabled");
		return -ENOTSUP;
	} else if (!is_udp && !IS_ENABLED(CONFIG_NET_TCP)) {
		LOG_WRN("CONFIG_NET_TCP disabled");
		return -ENOTSUP;
	}

	if (is_udp) {
		ret = zperf_udp_download(&param, zperf_download_cb, NULL);
	} else {
		ret = zperf_tcp_download(&param, zperf_download_cb, NULL);
	}

	if (ret == -EALREADY) {
		LOG_WRN("%s server already started!", is_udp ? "UDP" : "TCP");
		return -ENOEXEC;
	} else if (ret < 0) {
		LOG_ERR("Failed to start %s server (%d)", is_udp ? "UDP" : "TCP", ret);
		return -ENOEXEC;
	}

	k_yield();

	LOG_INF("%s Thread zperf server started on port %u", is_udp ? "UDP" : "TCP", param.port);

	return 0;
}
