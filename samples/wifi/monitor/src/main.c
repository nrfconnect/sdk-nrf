/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Monitor sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(monitor, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <nrf_wifi/fw_if/umac_if/inc/default/fmac_structs.h>

#include "net_private.h"

#define STACK_SIZE 1024
#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define RECV_BUFFER_SIZE 1024
#define RAW_PKT_HDR 6

/* Structure for wifi packet stats */
typedef struct {
	int ackCount;
	int rtsCount;
	int ctsCount;
	int blockAckCount;
	int blockAckReqCount;
	int probeReqCount;
	int beaconCount;
	int probeResponseCount;
	int dataCount;
	int qosDataCount;
	int nullCount;
	int qosNullCount;
	/* Add more counters for other types as needed */
} packetStats;

packetStats stats = {0};

/* Structure for the MAC header */
typedef struct {
	unsigned char frameControl;
	unsigned char duration[2];
	unsigned char receiverAddr[6];
	unsigned char transmitterAddr[6];
	/* Add more fields based on your actual MAC header structure */
} macHeader;

struct packet_data {
	int send_sock;
	int recv_sock;
	char recv_buffer[RECV_BUFFER_SIZE];
};

static struct k_sem quit_lock;
static void create_rx_thread(void);
static struct packet_data sock_packet;
static bool finish;

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE,
		create_rx_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static void quit(void)
{
	k_sem_give(&quit_lock);
}

/* Function to parse and update statistics for Wi-Fi packets */
void parseAndUpdateStats(unsigned char *packet, packetStats *stats)
{
	macHeader *macHdr = (macHeader *)packet;

	/* Extract the frame type and subtype from the frame control field */
	unsigned char frameType = (macHdr->frameControl & 0x0C) >> 2;
	unsigned char frameSubtype = (macHdr->frameControl & 0xF0) >> 4;

	/* Update statistics based on the frame type and subtype */
	switch (frameType) {
	/* Control frame */
	case 0x01:
		switch (frameSubtype) {
		case 0x0D:
			stats->ackCount++;
			break;

		case 0x0B:
			stats->rtsCount++;
			break;

		case 0x0C:
			stats->ctsCount++;
			break;

		case 0x09:
			stats->blockAckCount++;
			break;

		case 0x08:
			stats->blockAckReqCount++;
			break;

			/* Add more cases for other Data frame subtypes as needed */
		default:
		}
		break;

	/* Data frame */
	case 0x02:
		switch (frameSubtype) {
		case 0x00:
			stats->dataCount++;
			break;

		case 0x08:
			stats->qosDataCount++;
			break;

		case 0x04:
			stats->nullCount++;
			break;

		case 0x0C:
			stats->qosNullCount++;
			break;

		/* Add more cases for other Data frame subtypes as needed */
		default:
		}
		break;

	/* Management frame */
	case 0x00:
		switch (frameSubtype) {
		case 0x04:
			stats->probeReqCount++;
			break;

		case 0x08:
			stats->beaconCount++;
			break;

		case 0x05:
			stats->probeResponseCount++;
			break;

		/* Add more cases for other Management frame subtypes as needed */
		default:
		}
		break;

	case 0x03:
		/* Handle reserved frame types as needed */
		break;

	default:
		/* Handle unknown frame types as needed */
		break;
	}
}

int printStats(void)
{
	/* Print the updated statistics */
	LOG_INF("Management Frames:");
	LOG_INF("\tBeacon Count: %d", stats.beaconCount);
	LOG_INF("\tProbe Request Count: %d", stats.probeReqCount);
	LOG_INF("\tProbe Response Count: %d", stats.probeResponseCount);
	LOG_INF("Control Frames:");
	LOG_INF("\t Ack Count %d", stats.ackCount);
	LOG_INF("\t RTS Count %d", stats.rtsCount);
	LOG_INF("\t CTS Count %d", stats.ctsCount);
	LOG_INF("\t Block Ack Count %d", stats.blockAckCount);
	LOG_INF("\t Block Ack Req Count %d", stats.blockAckReqCount);
	LOG_INF("Data Frames:");
	LOG_INF("\tData Count: %d", stats.dataCount);
	LOG_INF("\tQoS Data Count: %d", stats.qosDataCount);
	LOG_INF("\tNull Count: %d", stats.nullCount);
	LOG_INF("\tQoS Null Count: %d", stats.qosNullCount);
	LOG_INF("\n");

	return 0;
}

static void wifi_set_channel(void)
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = {0};
	int ret;

	channel_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	channel_info.if_index = net_if_get_by_iface(iface);
	channel_info.channel = CONFIG_MONITOR_MODE_CHANNEL;
	if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
		   (channel_info.channel > WIFI_CHANNEL_MAX)) {
		LOG_ERR("Invalid channel number. Range is (1-233)");
		return;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface,
		       &channel_info, sizeof(channel_info));
	if (ret) {
		LOG_ERR(" Channel setting failed %d\n", ret);
			return;
	}

	LOG_INF("Wi-Fi channel set to %d", channel_info.channel);
}

static void wifi_set_mode(void)
{
	int ret;
	struct net_if *iface;
	struct wifi_mode_info mode_info = {0};

	mode_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	mode_info.if_index = net_if_get_by_iface(iface);
	mode_info.mode = WIFI_MONITOR_MODE;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed %d", ret);
	}

	LOG_INF("Mode set to Monitor");
}

static int process_rx_packet(struct packet_data *packet)
{
	uint32_t start_time;
	int ret = 0;
	int received;
	int total_duration = CONFIG_STATS_PRINT_TIMEOUT * 1000; /* Converting to ms */

	LOG_INF("Waiting for packets ...");

	start_time = k_uptime_get_32();

	do {
		if (finish) {
			ret = -1;
			LOG_ERR("closing receive socket ...");
			break;
		}

		received = recv(packet->recv_sock, packet->recv_buffer,
				sizeof(packet->recv_buffer), 0);

		if (received < 0) {
			if (errno == EAGAIN) {
				continue;
			}

			LOG_ERR("Monitor : recv error %d", errno);
			ret = -errno;
			break;
		}

		parseAndUpdateStats(&packet->recv_buffer[RAW_PKT_HDR], &stats);

		if ((k_uptime_get_32() - start_time) >= total_duration)	{
			printStats();
			start_time = k_uptime_get_32();
		}

		received = 0;

	} while (true);

			return ret;
}

static int setup_raw_socket(int *sock)
{
	struct sockaddr_ll dst = { 0 };
	int ret;

	*sock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
	if (*sock < 0) {
		LOG_ERR("Failed to create %s socket : %d",
				IS_ENABLED(CONFIG_NET_SAMPLE_ENABLE_PACKET_DGRAM) ?
				"DGRAM" : "RAW",
				errno);
		return -errno;
	}

	dst.sll_ifindex = net_if_get_by_iface(net_if_get_default());
	dst.sll_family = AF_PACKET;

	ret = bind(*sock, (const struct sockaddr *)&dst,
			sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Failed to bind packet socket : %d", errno);
		return -errno;
	}

	return 0;
}

/* handle incoming wifi packets in monitor mode */
static void create_rx_thread(void)
{
	int ret;
	struct timeval timeo_optval = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	ret = setup_raw_socket(&sock_packet.recv_sock);
	if (ret < 0) {
		quit();
		return;
	}

	ret = setsockopt(sock_packet.recv_sock, SOL_SOCKET, SO_RCVTIMEO,
			&timeo_optval, sizeof(timeo_optval));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = process_rx_packet(&sock_packet);
		if (ret < 0) {
			quit();
			return;
		}
	}
}

int main(void)
{
	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	k_thread_start(receiver_thread_id);

	wifi_set_mode();

	wifi_set_channel();

	k_msleep(100);

	k_sem_take(&quit_lock, K_FOREVER);

	finish = true;

	k_thread_join(receiver_thread_id, K_FOREVER);

	if (sock_packet.recv_sock >= 0) {
		(void)close(sock_packet.recv_sock);
	}

	return 0;
}
