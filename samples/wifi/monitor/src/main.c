/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Monitor sample
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
LOG_MODULE_REGISTER(monitor, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/net/socket.h>

#define STACK_SIZE CONFIG_RX_THREAD_STACK_SIZE
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)

#define RECV_BUFFER_SIZE 1024
#define RAW_PKT_HDR 6

/* Enumeration for Wi-Fi frame types and subtypes */
enum FrameType {
	MANAGEMENT_FRAME = 0x00,
	CONTROL_FRAME = 0x01,
	DATA_FRAME = 0x02,
	RESERVED_FRAME = 0x03,
	UNKNOWN_FRAME = 0xFF
};

enum ControlSubtype {
	ACK_SUBTYPE = 0x0D,
	RTS_SUBTYPE = 0x0B,
	CTS_SUBTYPE = 0x0C,
	BLOCK_ACK_SUBTYPE = 0x09,
	BLOCK_ACK_REQ_SUBTYPE = 0x08
};

enum DataSubtype {
	DATA_SUBTYPE = 0x00,
	QOS_DATA_SUBTYPE = 0x08,
	NULL_SUBTYPE = 0x04,
	QOS_NULL_SUBTYPE = 0x0C
};

enum ManagementSubtype {
	PROBE_REQ_SUBTYPE = 0x04,
	BEACON_SUBTYPE = 0x08,
	PROBE_RESPONSE_SUBTYPE = 0x05
    /* Add more subtypes as needed */
};

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

#define PKT_FC_TYPE_MASK 0x0C
#define PKT_FC_TYPE_SHIFT 2
#define PKT_FC_SUBTYPE_MASK 0xF0
#define PKT_FC_SUBTYPE_SHIFT 4

static unsigned char get_frame_type(unsigned char frameControl)
{
	return (frameControl & PKT_FC_TYPE_MASK) >> PKT_FC_TYPE_SHIFT;
}

static unsigned char get_frame_subtype(unsigned char frameControl)
{
	return (frameControl & PKT_FC_SUBTYPE_MASK) >> PKT_FC_SUBTYPE_SHIFT;
}

static void create_rx_thread(void);

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE,
		create_rx_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

/* Function to parse and update statistics for Wi-Fi packets */
static void parseAndUpdateStats(unsigned char *packet, packetStats *stats)
{
	macHeader *macHdr = (macHeader *)packet;

	/* Extract the frame type and subtype from the frame control field */
	unsigned char frameType = get_frame_type(macHdr->frameControl);
	unsigned char frameSubtype = get_frame_subtype(macHdr->frameControl);

	/* Update statistics based on the frame type and subtype */
	switch (frameType) {
	case CONTROL_FRAME:
		switch (frameSubtype) {
		case ACK_SUBTYPE:
			stats->ackCount++;
			break;
		case RTS_SUBTYPE:
			stats->rtsCount++;
			break;
		case CTS_SUBTYPE:
			stats->ctsCount++;
			break;
		case BLOCK_ACK_SUBTYPE:
			stats->blockAckCount++;
			break;
		case BLOCK_ACK_REQ_SUBTYPE:
			stats->blockAckReqCount++;
			break;
			/* Add more cases for other Data frame subtypes as needed */
		default:
			LOG_INF("Unknown Control frame subtype: %d", frameSubtype);
		}
		break;
	case DATA_FRAME:
		switch (frameSubtype) {
		case DATA_SUBTYPE:
			stats->dataCount++;
			break;
		case QOS_DATA_SUBTYPE:
			stats->qosDataCount++;
			break;
		case NULL_SUBTYPE:
			stats->nullCount++;
			break;
		case QOS_NULL_SUBTYPE:
			stats->qosNullCount++;
			break;
		/* Add more cases for other Data frame subtypes as needed */
		default:
			LOG_INF("Unknown Data frame subtype: %d", frameSubtype);
		}
		break;

	case MANAGEMENT_FRAME:
		switch (frameSubtype) {
		case PROBE_REQ_SUBTYPE:
			stats->probeReqCount++;
			break;
		case BEACON_SUBTYPE:
			stats->beaconCount++;
			break;
		case PROBE_RESPONSE_SUBTYPE:
			stats->probeResponseCount++;
			break;
		/* Add more cases for other Management frame subtypes as needed */
		default:
			LOG_INF("Unknown Management frame subtype: %d", frameSubtype);
		}
		break;
	case RESERVED_FRAME:
		/* Handle reserved frame types as needed */
		LOG_INF("Reserved frame type: %d", frameType);
		break;
	default:
		/* Handle unknown frame types as needed */
		LOG_INF("Unknown frame type: %d", frameType);
		break;
	}
}

static int printStats(void)
{
	/* Print the updated statistics */
	LOG_INF("Management Frames:");
	LOG_INF("\tBeacon Count: %d", stats.beaconCount);
	LOG_INF("\tProbe Request Count: %d", stats.probeReqCount);
	LOG_INF("\tProbe Response Count: %d", stats.probeResponseCount);
	LOG_INF("Control Frames:");
	LOG_INF("\t ACK Count %d", stats.ackCount);
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

static int wifi_set_channel(void)
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = { 0 };
	int ret;

	channel_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	channel_info.if_index = net_if_get_by_iface(iface);
	channel_info.channel = CONFIG_MONITOR_MODE_CHANNEL;
	if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
	    (channel_info.channel > WIFI_CHANNEL_MAX)) {
		LOG_ERR("Invalid channel number %d. Range is %d-%d",
			channel_info.channel, WIFI_CHANNEL_MIN, WIFI_CHANNEL_MAX);
		return -1;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface,
		       &channel_info, sizeof(channel_info));
	if (ret) {
		LOG_ERR(" Channel setting failed %d Channel %d\n", ret, channel_info.channel);
		return -1;
	}

	LOG_INF("Wi-Fi channel set to %d for interface %d",
		channel_info.channel, channel_info.if_index);

	return 0;
}

static int wifi_set_mode(void)
{
	int ret;
	struct net_if *iface;
	struct wifi_mode_info mode_info = { 0 };

	mode_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	mode_info.if_index = net_if_get_by_iface(iface);
	mode_info.mode = WIFI_MONITOR_MODE;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed %d", ret);
		return -1;
	}

	LOG_INF("Interface (%d) now setup in Wi-Fi monitor mode", mode_info.if_index);

	return 0;
}

static int process_rx_packet(struct packet_data *packet)
{
	uint32_t start_time;
	int ret = 0;
	int received;
	int stats_print_period = CONFIG_STATS_PRINT_TIMEOUT * 1000; /* Converting to ms */

	LOG_INF("Wi-Fi monitor mode RX thread started");
	start_time = k_uptime_get_32();

	do {
		received = recv(packet->recv_sock, packet->recv_buffer,
				sizeof(packet->recv_buffer), 0);
		if (received <= 0) {
			if (errno == EAGAIN) {
				continue;
			}

			if (received < 0) {
				LOG_ERR("Monitor : recv error %s", strerror(errno));
				ret = -errno;
			}
			break;
		}

		parseAndUpdateStats(&packet->recv_buffer[RAW_PKT_HDR], &stats);

		if ((k_uptime_get_32() - start_time) >= stats_print_period)	{
			printStats();
			start_time = k_uptime_get_32();
		}
	} while (true);

	return ret;
}

static int setup_raw_socket(int *sock)
{
	struct sockaddr_ll dst = { 0 };
	int ret;

	*sock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
	if (*sock < 0) {
		LOG_ERR("Failed to create RAW socket : %d",
				errno);
		return -errno;
	}

	dst.sll_ifindex = net_if_get_by_iface(net_if_get_first_wifi());
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
	struct packet_data sock_packet;

	ret = setup_raw_socket(&sock_packet.recv_sock);
	if (ret < 0) {
		return;
	}

	ret = setsockopt(sock_packet.recv_sock, SOL_SOCKET, SO_RCVTIMEO,
			&timeo_optval, sizeof(timeo_optval));
	if (ret < 0) {
		LOG_ERR("Failed to set socket options : %s", strerror(errno));
		return;
	}

	while (!ret) {
		ret = process_rx_packet(&sock_packet);
		if (ret < 0) {
			return;
		}
	}
}

int main(void)
{
	int ret;

	ret = wifi_set_mode();
	if (ret) {
		return -1;
	}

	ret = wifi_set_channel();
	if (ret) {
		return -1;
	}

	k_thread_start(receiver_thread_id);

	return 0;
}
