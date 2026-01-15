/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(otperf, CONFIG_OTPERF_LOG_LEVEL);

#include <string.h>

#include <openthread/ip6.h>
#include <openthread/udp.h>
#include <openthread.h>

#include "otperf_internal.h"
#include "otperf_session.h"

static otperf_callback udp_session_cb;
static void *udp_user_data;
static bool udp_server_running;
static struct otSockAddr udp_server_addr;

static otUdpSocket current_socket;

static inline void build_reply(struct otperf_udp_datagram *hdr, struct otperf_server_hdr *stat,
			       uint8_t *buf)
{
	int pos = 0;
	struct otperf_server_hdr *stat_hdr;

	memcpy(&buf[pos], hdr, sizeof(struct otperf_udp_datagram));
	pos += sizeof(struct otperf_udp_datagram);

	stat_hdr = (struct otperf_server_hdr *)&buf[pos];

	stat_hdr->flags = sys_cpu_to_be32(stat->flags);
	stat_hdr->total_len1 = sys_cpu_to_be32(stat->total_len1);
	stat_hdr->total_len2 = sys_cpu_to_be32(stat->total_len2);
	stat_hdr->stop_sec = sys_cpu_to_be32(stat->stop_sec);
	stat_hdr->stop_usec = sys_cpu_to_be32(stat->stop_usec);
	stat_hdr->error_cnt = sys_cpu_to_be32(stat->error_cnt);
	stat_hdr->outorder_cnt = sys_cpu_to_be32(stat->outorder_cnt);
	stat_hdr->datagrams = sys_cpu_to_be32(stat->datagrams);
	stat_hdr->jitter1 = sys_cpu_to_be32(stat->jitter1);
	stat_hdr->jitter2 = sys_cpu_to_be32(stat->jitter2);
}

#define BUF_SIZE sizeof(struct otperf_udp_datagram) + sizeof(struct otperf_server_hdr)

static int otperf_receiver_send_stat(otUdpSocket *sock, const struct otSockAddr *addr,
				     struct otperf_udp_datagram *hdr,
				     struct otperf_server_hdr *stat)
{
	otInstance *instance = openthread_get_default_instance();
	otMessage *message;
	otMessageInfo messageInfo;
	otError error;
	uint8_t reply[BUF_SIZE];

	build_reply(hdr, stat, reply);

	message = otUdpNewMessage(instance, NULL);
	if (message == NULL) {
		LOG_ERR("Unable to create a message to send statistics");
		return -EIO;
	}

	error = otMessageAppend(message, reply, sizeof(reply));
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Unable to add data to the statistics message");
		otMessageFree(message);
		return -EIO;
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = addr->mAddress;
	messageInfo.mPeerPort = addr->mPort;

	error = otUdpSend(instance, sock, message, &messageInfo);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Unable to send statistics message via udp");
		otMessageFree(message);
		return -EIO;
	}

	return 0;
}

static void udp_received(otUdpSocket *sock, const struct otSockAddr *addr, uint8_t *data,
			 size_t datalen)
{
	struct otperf_udp_datagram *hdr;
	struct session *session;
	int32_t transit_time;
	int64_t time;
	int32_t id;

	if (datalen < sizeof(struct otperf_udp_datagram)) {
		LOG_WRN("Short iperf packet!");
		return;
	}

	hdr = (struct otperf_udp_datagram *)data;
	/* swap the data from network to host endianness */
	hdr->id2 = sys_be32_to_cpu(hdr->id2);
	hdr->id = sys_be32_to_cpu(hdr->id);
	hdr->tv_sec = sys_be32_to_cpu(hdr->tv_sec);
	hdr->tv_usec = sys_be32_to_cpu(hdr->tv_usec);

	time = k_uptime_ticks();

	session = get_session(addr);
	if (!session) {
		LOG_ERR("Cannot get a session!");
		return;
	}

	id = hdr->id;

	switch (session->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		if (id < 0) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (otperf_receiver_send_stat(sock, addr, hdr, &session->stat) < 0) {
				LOG_ERR("Failed to send the packet");
			}
		} else {
			otperf_reset_session_stats(session);
			session->state = STATE_ONGOING;
			session->start_time = time;

			/* Start a new session! */
			if (udp_session_cb != NULL) {
				udp_session_cb(OTPERF_SESSION_STARTED, NULL, udp_user_data);
			}
		}
		break;
	case STATE_ONGOING:
		if (id < 0) { /* Negative id means session end. */
			struct otperf_results results = {0};
			uint64_t duration;

			duration = k_ticks_to_us_ceil64(time - session->start_time);

			/* Update state machine */
			session->state = STATE_COMPLETED;

			/* Fill statistics */
			session->stat.flags = 0x80000000;
			session->stat.total_len1 = session->length >> 32;
			session->stat.total_len2 = session->length % 0xFFFFFFFF;
			session->stat.stop_sec = duration / USEC_PER_SEC;
			session->stat.stop_usec = duration % USEC_PER_SEC;
			session->stat.error_cnt = session->error;
			session->stat.outorder_cnt = session->outorder;
			session->stat.datagrams = session->counter;
			session->stat.jitter1 = 0;
			session->stat.jitter2 = session->jitter;

			if (otperf_receiver_send_stat(sock, addr, hdr, &session->stat) < 0) {
				LOG_ERR("Failed to send the packet");
			}

			results.nb_packets_rcvd = session->counter;
			results.nb_packets_lost = session->error;
			results.nb_packets_outorder = session->outorder;
			results.total_len = session->length;
			results.time_in_us = duration;
			results.jitter_in_us = session->jitter;
			if (session->counter == 0) {
				LOG_ERR("No packets were received during the session");
				results.packet_size = 0;
			} else {
				results.packet_size = session->length / session->counter;
			}

			if (udp_session_cb != NULL) {
				udp_session_cb(OTPERF_SESSION_FINISHED, &results, udp_user_data);
			}
		} else {
			/* Update counter */
			session->counter++;
			session->length += datalen;

			/* Compute jitter */
			transit_time = time_delta(k_ticks_to_us_ceil32(time),
						  hdr->tv_sec * USEC_PER_SEC + hdr->tv_usec);
			if (session->last_transit_time != 0) {
				int32_t delta_transit = transit_time - session->last_transit_time;

				delta_transit =
					(delta_transit < 0) ? -delta_transit : delta_transit;

				session->jitter += (delta_transit - session->jitter) / 16;
			}

			session->last_transit_time = transit_time;

			/* Check header id */
			if (id != session->next_id) {
				if (id < session->next_id) {
					session->outorder++;
				} else {
					session->error += id - session->next_id;
					session->next_id = id + 1;
				}
			} else {
				session->next_id++;
			}
		}
		break;
	default:
		break;
	}
}

static void udp_receiver_cleanup(void)
{
	otInstance *instance = openthread_get_default_instance();

	otUdpClose(instance, &current_socket);
	udp_server_running = false;
	udp_session_cb = NULL;

	otperf_session_reset();
}

static void ot_udp_receive_cb(void *aContext, otMessage *aMessage,
			      const otMessageInfo *aMessageInfo)
{
	static uint8_t buf[CONFIG_OTPERF_MAX_PACKET_SIZE];
	uint16_t payload_len;
	uint16_t offset;

	if (!udp_server_running) {
		return;
	}

	offset = otMessageGetOffset(aMessage);
	payload_len = otMessageGetLength(aMessage) - offset;

	if (payload_len > sizeof(buf)) {
		payload_len = sizeof(buf);
	}

	if (otMessageRead(aMessage, offset, buf, payload_len) != payload_len) {
		if (udp_session_cb) {
			udp_session_cb(OTPERF_SESSION_ERROR, NULL, udp_user_data);
		}
		return;
	}

	struct otSockAddr addr = {
		.mPort = aMessageInfo->mPeerPort,
		.mAddress = aMessageInfo->mPeerAddr,
	};
	udp_received(&current_socket, &addr, buf, payload_len);
}

static int otperf_udp_receiver_init(otInstance *instance)
{
	otError error;

	error = otUdpOpen(instance, &current_socket, ot_udp_receive_cb, NULL);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to open UDP socket: %d", error);
		return -EIO;
	}

	otSockAddr bindAddr = {.mPort = udp_server_addr.mPort,
			       .mAddress = udp_server_addr.mAddress};

	error = otUdpBind(instance, &current_socket, &bindAddr, OT_NETIF_THREAD);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Cannot bind IPv6 UDP port %d: %d", bindAddr.mPort, error);
		otUdpClose(instance, &current_socket);
		return -EIO;
	}

	LOG_INF("Listening on port %d", bindAddr.mPort);
	return 0;
}

int otperf_udp_download(const struct otperf_download_params *param, otperf_callback callback,
			void *user_data)
{
	struct otInstance *instance;
	int ret;

	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (udp_server_running) {
		return -EALREADY;
	}

	instance = openthread_get_default_instance();
	if (!instance) {
		return -ENODEV;
	}

	udp_session_cb = callback;
	udp_user_data = user_data;

	udp_server_addr = param->addr;

	ret = otperf_udp_receiver_init(instance);
	if (ret < 0) {
		udp_server_running = false;
		return ret;
	}

	udp_server_running = true;
	return 0;
}

int otperf_udp_download_stop(void)
{
	if (!udp_server_running) {
		return -EALREADY;
	}

	udp_receiver_cleanup();

	return 0;
}
