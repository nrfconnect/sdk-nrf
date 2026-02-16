/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otperf, CONFIG_OTPERF_LOG_LEVEL);

#include <zephyr/toolchain.h>

#include <string.h>

#include <openthread/udp.h>
#include <openthread.h>

#include "otperf_internal.h"
#include "otperf_session.h"

#define OTPERF_RESULT_RESPONSE_TIMEOUT 500
#define OTPERF_FULL_BUFFER_TIMEOUT     50

static uint8_t sample_packet[CONFIG_OTPERF_MAX_PACKET_SIZE];

struct otperf_results *curr_results;

otUdpSocket sock;
struct k_sem udp_response_semaphore;

static inline void otperf_upload_decode_stat(const uint8_t *data, size_t datalen,
					     struct otperf_results *results)
{
	struct otperf_server_hdr *stat;
	uint32_t flags;

	if (!results) {
		LOG_ERR("No results to write to");
		return;
	}

	if (datalen < sizeof(struct otperf_udp_datagram) + sizeof(struct otperf_server_hdr)) {
		LOG_ERR("Network packet too short");
	}

	stat = (struct otperf_server_hdr *)(data + sizeof(struct otperf_udp_datagram));
	flags = sys_be32_to_cpu(UNALIGNED_GET(&stat->flags));
	if (!(flags & OTPERF_FLAGS_VERSION1)) {
		LOG_ERR("Unexpected response flags");
	}

	results->nb_packets_rcvd = sys_be32_to_cpu(UNALIGNED_GET(&stat->datagrams));
	results->nb_packets_lost = sys_be32_to_cpu(UNALIGNED_GET(&stat->error_cnt));
	results->nb_packets_outorder = sys_be32_to_cpu(UNALIGNED_GET(&stat->outorder_cnt));
	results->total_len = (((uint64_t)sys_be32_to_cpu(UNALIGNED_GET(&stat->total_len1))) << 32) +
			     sys_be32_to_cpu(UNALIGNED_GET(&stat->total_len2));
	results->time_in_us = sys_be32_to_cpu(UNALIGNED_GET(&stat->stop_usec)) +
			      sys_be32_to_cpu(UNALIGNED_GET(&stat->stop_sec)) * USEC_PER_SEC;
	results->jitter_in_us = sys_be32_to_cpu(UNALIGNED_GET(&stat->jitter2)) +
				sys_be32_to_cpu(UNALIGNED_GET(&stat->jitter1)) * USEC_PER_SEC;
}

static void otperf_upload_fin(otUdpSocket *sock, uint32_t nb_packets, uint64_t end_time_us,
			      uint32_t packet_size, const otSockAddr *sock_address)
{
	struct otperf_udp_datagram *datagram;
	struct otperf_client_hdr_v1 *hdr;
	uint32_t secs = end_time_us / USEC_PER_SEC;
	uint32_t usecs = end_time_us % USEC_PER_SEC;
	int loop = CONFIG_OTPERF_UDP_REPORT_RETRANSMISSION_COUNT;
	int ret = 0;
	otInstance *instance = openthread_get_default_instance();
	otMessage *message;
	otMessageInfo messageInfo;

	while (loop-- > 0) {
		datagram = (struct otperf_udp_datagram *)sample_packet;

		/* Fill the packet header */
		datagram->id = sys_cpu_to_be32(-nb_packets);
		datagram->tv_sec = sys_cpu_to_be32(secs);
		datagram->tv_usec = sys_cpu_to_be32(usecs);

		hdr = (struct otperf_client_hdr_v1 *)(sample_packet + sizeof(*datagram));

		/* According to iperf documentation (in include/Settings.hpp),
		 * if the flags == 0, then the other values are ignored.
		 * But even if the values in the header are ignored, try
		 * to set there some meaningful values.
		 */
		hdr->flags = 0;
		hdr->num_of_threads = sys_cpu_to_be32(1);
		hdr->port = 0;
		hdr->buffer_len = sizeof(sample_packet) - sizeof(*datagram) - sizeof(*hdr);
		hdr->bandwidth = 0;
		hdr->num_of_bytes = sys_cpu_to_be32(packet_size);

		message = otUdpNewMessage(instance, NULL);
		if (message == NULL) {
			/* most Likely the queue is full. wait for it a bit. */
			k_sleep(K_MSEC(OTPERF_FULL_BUFFER_TIMEOUT));
			continue;
		}

		ret = otMessageAppend(message, sample_packet, packet_size);
		if (ret != OT_ERROR_NONE) {
			otMessageFree(message);
			/* most Likely the queue is full. wait for it a bit. */
			k_sleep(K_MSEC(OTPERF_FULL_BUFFER_TIMEOUT));
			continue;
		}

		memset(&messageInfo, 0, sizeof(messageInfo));
		messageInfo.mPeerAddr = sock_address->mAddress;
		messageInfo.mPeerPort = sock_address->mPort;

		ret = otUdpSend(instance, sock, message, &messageInfo);

		if (ret != OT_ERROR_NONE) {
			LOG_ERR("Unable to send statistics message via udp");
			otMessageFree(message);
			continue;
		}
		if (k_sem_take(&udp_response_semaphore, K_MSEC(OTPERF_RESULT_RESPONSE_TIMEOUT)) ==
		    -EAGAIN) {
			LOG_INF("Time out, no stat response");
		} else {
			break;
		}
	}
}

static void ot_uploader_udp_receive_cb(void *aContext, otMessage *aMessage,
				       const otMessageInfo *aMessageInfo)
{
	uint8_t stats[sizeof(struct otperf_udp_datagram) + sizeof(struct otperf_server_hdr)] = {0};
	uint16_t payload_len;
	uint16_t offset;

	offset = otMessageGetOffset(aMessage);
	payload_len = otMessageGetLength(aMessage) - offset;

	if (payload_len > sizeof(stats)) {
		LOG_INF("The prepared buffer is too small to fit the whole received message, "
			"truncating");
		payload_len = sizeof(stats);
	}

	if (otMessageRead(aMessage, offset, stats, payload_len) != payload_len) {
		LOG_ERR("Failed to retrieve the correct amount of data");
		return;
	}

	otperf_upload_decode_stat(stats, payload_len, curr_results);

	k_sem_give((struct k_sem *)aContext);
}

static int udp_upload(otUdpSocket *sock, const struct otperf_upload_params *param,
		      struct otperf_results *results)
{
	size_t header_size =
		sizeof(struct otperf_udp_datagram) + sizeof(struct otperf_client_hdr_v1);
	uint32_t duration_in_ms = param->duration_ms;
	uint32_t packet_size = param->packet_size;
	uint32_t rate_in_kbps = param->rate_kbps;
	uint32_t packet_duration_us = otperf_packet_duration(packet_size, rate_in_kbps);
	uint32_t packet_duration_ticks = k_us_to_ticks_ceil32(packet_duration_us);
	uint32_t nb_packets = 0U;
	uint64_t usecs64;
	int64_t start_time, end_time;
	otInstance *instance = openthread_get_default_instance();
	otMessage *message;
	otMessageInfo messageInfo;
	otError error;

	curr_results = results;

	if (packet_size > CONFIG_OTPERF_MAX_PACKET_SIZE) {
		LOG_ERR("Packet size too large! max size: %u", CONFIG_OTPERF_MAX_PACKET_SIZE);
		packet_size = CONFIG_OTPERF_MAX_PACKET_SIZE;
	} else if (packet_size < sizeof(struct otperf_udp_datagram)) {
		LOG_ERR("Packet size set to the min size: %zu", header_size);
		packet_size = header_size;
	}

	/* Start the loop */
	start_time = k_uptime_ticks();
	end_time = start_time + k_ms_to_ticks_ceil64(duration_in_ms);

	int64_t next_scheduled_tick = start_time;

	/* Default data payload */
	(void)memset(sample_packet, 'n', sizeof(sample_packet));

	while (next_scheduled_tick < end_time) {
		struct otperf_udp_datagram *datagram;
		struct otperf_client_hdr_v1 *hdr;
		uint32_t secs, usecs;
		int64_t current_loop_time;

		nb_packets++;
		int64_t ticks_to_wait = next_scheduled_tick - k_uptime_ticks();

		if (ticks_to_wait > 0) {
			k_sleep(K_TICKS(ticks_to_wait));
		}

		next_scheduled_tick += packet_duration_ticks;

		/* Timestamp */
		current_loop_time = k_uptime_ticks();

		usecs64 = param->unix_offset_us +
			  k_ticks_to_us_floor64(current_loop_time - start_time);
		secs = usecs64 / USEC_PER_SEC;
		usecs = usecs64 % USEC_PER_SEC;

		/* Fill the packet header */
		datagram = (struct otperf_udp_datagram *)sample_packet;

		datagram->id = sys_cpu_to_be32(nb_packets);
		datagram->tv_sec = sys_cpu_to_be32(secs);
		datagram->tv_usec = sys_cpu_to_be32(usecs);

		hdr = (struct otperf_client_hdr_v1 *)(sample_packet + sizeof(*datagram));
		hdr->flags = 0;
		hdr->num_of_threads = sys_cpu_to_be32(1);
		hdr->port = sys_cpu_to_be32(param->peer_addr.mPort);
		hdr->buffer_len = sizeof(sample_packet) - sizeof(*datagram) - sizeof(*hdr);
		hdr->bandwidth = sys_cpu_to_be32(rate_in_kbps);
		hdr->num_of_bytes = sys_cpu_to_be32(packet_size);

		message = otUdpNewMessage(instance, NULL);
		if (message == NULL) {
			/* Most likely the baudrate is to high for the radio to keep up */
			continue;
		}

		error = otMessageAppend(message, sample_packet, packet_size);
		if (error != OT_ERROR_NONE) {
			/* Most likely the baudrate is to high for the radio to keep up */
			otMessageFree(message);
			continue;
		}

		memset(&messageInfo, 0, sizeof(messageInfo));
		messageInfo.mPeerAddr = param->peer_addr.mAddress;
		messageInfo.mPeerPort = param->peer_addr.mPort;

		error = otUdpSend(instance, sock, message, &messageInfo);

		if (error != OT_ERROR_NONE) {
			LOG_ERR("Unable to send statistics message via udp");
			otMessageFree(message);
			continue;
		}
	}

	end_time = k_uptime_ticks();
	usecs64 = param->unix_offset_us + k_ticks_to_us_floor64(end_time - start_time);

	otperf_upload_fin(sock, nb_packets, usecs64, packet_size, &param->peer_addr);

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us = k_ticks_to_us_ceil64(end_time - start_time);
	results->packet_size = packet_size;

	return 0;
}

int otperf_udp_upload(const struct otperf_upload_params *param, struct otperf_results *result)
{
	otInstance *instance = openthread_get_default_instance();

	k_sem_init(&udp_response_semaphore, 0, 1);
	int ret;

	if (param == NULL || result == NULL) {
		return -EINVAL;
	}

	ret = otUdpOpen(instance, &sock, ot_uploader_udp_receive_cb, &udp_response_semaphore);
	if (ret != OT_ERROR_NONE) {
		LOG_ERR("Failed to open UDP socket: %d", ret);
		return -EIO;
	}

	ret = udp_upload(&sock, param, result);

	otUdpClose(instance, &sock);

	return ret;
}
