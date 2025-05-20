/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Promiscuous sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(promiscuous, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_NET_CONFIG_SETTINGS
#include <zephyr/net/net_config.h>
#endif

#include <zephyr/net/ethernet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#include "wifi_connection.h"

#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#define RAW_PKT_HDR 6

/* Enumeration for Wi-Fi frame types and subtypes */
enum frame_type {
	MANAGEMENT_FRAME = 0x00,
	CONTROL_FRAME = 0x01,
	DATA_FRAME = 0x02,
	RESERVED_FRAME = 0x03,
	UNKNOWN_FRAME = 0xFF
};

enum control_subtype {
	CF_END_SUBTYPE = 0x0E,
	ACK_SUBTYPE = 0x0D,
	RTS_SUBTYPE = 0x0B,
	CTS_SUBTYPE = 0x0C,
	BLOCK_ACK_SUBTYPE = 0x09,
	BLOCK_ACK_REQ_SUBTYPE = 0x08
};

enum data_subtype {
	DATA_SUBTYPE = 0x00,
	QOS_DATA_SUBTYPE = 0x08,
	NULL_SUBTYPE = 0x04,
	QOS_NULL_SUBTYPE = 0x0C
};

enum management_subtype {
	PROBE_REQ_SUBTYPE = 0x04,
	BEACON_SUBTYPE = 0x08,
	PROBE_RESPONSE_SUBTYPE = 0x05,
	ACTION_SUBTYPE = 0x0D
	/* Add more subtypes as needed */
};

struct packet_data {
	int send_sock;
	int recv_sock;
	char recv_buffer[CONFIG_PROMISCUOUS_SAMPLE_RECV_BUFFER_SIZE];
};

/* Structure for the Frame Control */
typedef struct {
	uint16_t protocolVersion : 2;
	uint16_t type            : 2;
	uint16_t subtype         : 4;
	uint16_t toDS            : 1;
	uint16_t fromDS          : 1;
	uint16_t moreFragments   : 1;
	uint16_t retry           : 1;
	uint16_t powerManagement : 1;
	uint16_t moreData        : 1;
	uint16_t protectedFrame  : 1;
	uint16_t order           : 1;
} frame_control;

/* Structure for wifi packet stats */
typedef struct {
	int ack_count;
	int rts_count;
	int cts_count;
	int block_ack_count;
	int block_ack_req_count;
	int probe_req_count;
	int beacon_count;
	int probe_response_count;
	int data_count;
	int qos_data_count;
	int null_count;
	int qos_null_count;
	int action_count;
	int cf_end_count;
	int unknown_mgmt_count;
	int unknown_ctrl_count;
	int unknown_data_count;
	int reserved_frame_count;
	int unknown_frame_count;
	/* Add more counters for other types as needed */
} packet_stats;

packet_stats stats = {0};

/* Structure for the MAC header */
static void create_rx_thread(void);

K_THREAD_DEFINE(receiver_thread_id, CONFIG_PROMISCUOUS_SAMPLE_RX_THREAD_STACK_SIZE,
		create_rx_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

#ifdef CONFIG_NET_CAPTURE
static const struct device *capture_dev;
#endif

#ifdef CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>

static struct in_addr addr = { { { 192, 0, 2, 1 } } };
static struct in_addr mask = { { { 255, 255, 255, 0 } } };

int init_usb(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Cannot enable USB (%d)", ret);
		return ret;
	}

	return 0;
}
#endif

/* Function to parse and update statistics for Wi-Fi packets */
static void parse_and_update_stats(unsigned char *packet, packet_stats *stats)
{
	uint16_t *FrameControlField = (uint16_t *)packet;
	frame_control *fc = (frame_control *)FrameControlField;

	/* Extract the frame type and subtype from the frame control field */
	unsigned char frame_type = fc->type;
	unsigned char frame_subtype = fc->subtype;

	/* Update statistics based on the frame type and subtype */
	switch (frame_type) {
	case CONTROL_FRAME:
		switch (frame_subtype) {
		case ACK_SUBTYPE:
			stats->ack_count++;
			break;
		case RTS_SUBTYPE:
			stats->rts_count++;
			break;
		case CTS_SUBTYPE:
			stats->cts_count++;
			break;
		case BLOCK_ACK_SUBTYPE:
			stats->block_ack_count++;
			break;
		case BLOCK_ACK_REQ_SUBTYPE:
			stats->block_ack_req_count++;
			break;
		case CF_END_SUBTYPE:
			stats->cf_end_count++;
			/* Add more cases for other Data frame subtypes as needed */
		default:
			stats->unknown_ctrl_count++;
			LOG_DBG("Unknown Control frame subtype: %d", frame_subtype);
		}
		break;
	case DATA_FRAME:
		switch (frame_subtype) {
		case DATA_SUBTYPE:
			stats->data_count++;
			break;
		case QOS_DATA_SUBTYPE:
			stats->qos_data_count++;
			break;
		case NULL_SUBTYPE:
			stats->null_count++;
			break;
		case QOS_NULL_SUBTYPE:
			stats->qos_null_count++;
			break;
		/* Add more cases for other Data frame subtypes as needed */
		default:
			stats->unknown_data_count++;
			LOG_DBG("Unknown Data frame subtype: %d", frame_subtype);
		}
		break;
	case MANAGEMENT_FRAME:
		switch (frame_subtype) {
		case PROBE_REQ_SUBTYPE:
			stats->probe_req_count++;
			break;
		case BEACON_SUBTYPE:
			stats->beacon_count++;
			break;
		case PROBE_RESPONSE_SUBTYPE:
			stats->probe_response_count++;
			break;
		case ACTION_SUBTYPE:
			stats->action_count++;
			break;
		/* Add more cases for other Management frame subtypes as needed */
		default:
			stats->unknown_mgmt_count++;
			LOG_DBG("Unknown Management frame subtype: %d", frame_subtype);
		}
		break;
	case RESERVED_FRAME:
		/* Handle reserved frame types as needed */
		stats->reserved_frame_count++;
		LOG_DBG("Reserved frame type: %d", frame_type);
		break;
	default:
		/* Handle unknown frame types as needed */
		stats->unknown_frame_count++;
		LOG_DBG("Unknown frame type: %d", frame_type);
		break;
	}
}

static void print_stats(void)
{
	/* Print the updated statistics */
	LOG_INF("Management Frames:");
	LOG_INF("\tBeacon Count: %d", stats.beacon_count);
	LOG_INF("\tProbe Request Count: %d", stats.probe_req_count);
	LOG_INF("\tProbe Response Count: %d", stats.probe_response_count);
	LOG_INF("\tAction Count: %d", stats.action_count);
	LOG_INF("Control Frames:");
	LOG_INF("\t ACK Count %d", stats.ack_count);
	LOG_INF("\t RTS Count %d", stats.rts_count);
	LOG_INF("\t CTS Count %d", stats.cts_count);
	LOG_INF("\t Block Ack Count %d", stats.block_ack_count);
	LOG_INF("\t Block Ack Req Count %d", stats.block_ack_req_count);
	LOG_INF("\t CF End Count %d", stats.cf_end_count);
	LOG_INF("Data Frames:");
	LOG_INF("\tData Count: %d", stats.data_count);
	LOG_INF("\tQoS Data Count: %d", stats.qos_data_count);
	LOG_INF("\tNull Count: %d", stats.null_count);
	LOG_INF("\tQoS Null Count: %d", stats.qos_null_count);
	LOG_INF("Unknown Frames:");
	LOG_INF("\tUnknown Management Frame Count: %d", stats.unknown_mgmt_count);
	LOG_INF("\tUnknown Control Frame Count: %d", stats.unknown_ctrl_count);
	LOG_INF("\tUnknown Data Frame Count: %d", stats.unknown_data_count);
	LOG_INF("\tReserved Frame Count: %d", stats.reserved_frame_count);
	LOG_INF("\tUnknown Frame Count: %d", stats.unknown_frame_count);
	LOG_INF("\n");
}

static int process_rx_packet(struct packet_data *packet)
{
	uint32_t start_time;
	int ret = 0;
	int received;
	int stats_print_period =
		CONFIG_PROMISCUOUS_SAMPLE_STATS_PRINT_TIMEOUT * 1000; /* Converting to ms */

	LOG_INF("Wi-Fi promiscuous mode RX thread started");
	start_time = k_uptime_get_32();

	do {
		received = recv(packet->recv_sock, packet->recv_buffer,
				sizeof(packet->recv_buffer), 0);
		if (received <= 0) {
			if (errno == EAGAIN) {
				continue;
			}

			if (received < 0) {
				LOG_ERR("recv error %s", strerror(errno));
				ret = -errno;
			}
			break;
		}

		parse_and_update_stats(&packet->recv_buffer[RAW_PKT_HDR], &stats);

		if ((k_uptime_get_32() - start_time) >= stats_print_period)	{
			print_stats();
			start_time = k_uptime_get_32();
		}
	} while (true);

	return ret;
}

static int setup_raw_socket(int *sock)
{
	struct sockaddr_ll dst = { 0 };
	int ret;

	*sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
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
		LOG_ERR("Failed to bind packet socket : %s", strerror(errno));
		return -errno;
	}

	return 0;
}

/* handle incoming wifi packets in promiscuous mode */
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

	close(sock_packet.recv_sock);

#ifdef CONFIG_NET_CAPTURE
	ret = net_capture_disable(capture_dev);
	if (ret < 0) {
		LOG_ERR("Failed to disable network capture : %d", ret);
	}
#endif
}

static int wifi_set_mode(int mode_val)
{
	struct net_if *iface = NULL;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	if (net_eth_promisc_mode(iface, true)) {
		LOG_ERR("Promiscuous mode enable failed");
		return -1;
	}

	LOG_INF("Promiscuous mode enabled");
	return 0;
}

#ifdef CONFIG_NET_CAPTURE
static int wifi_setup_net_capture(void)
{
	struct net_if *iface;
	const char *remote, *local, *peer;
	int ret;

	/* For now hardcode these addresses, as they never change */
	remote = "192.0.2.2";
	local = "2001:db8:200::1";
	peer = "2001:db8:200::2";

	ret = net_capture_setup(remote, local, peer, &capture_dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup network capture : %d", ret);
		return ret;
	}

	/* Wi-Fi is the capture interface */
	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	ret = net_capture_enable(capture_dev, iface);
	if (ret < 0) {
		LOG_ERR("Failed to enable network capture : %d", ret);
		return ret;
	}

	return 0;
}
#endif

int main(void)
{
	int ret;

#ifdef CONFIG_USB_DEVICE_STACK
	init_usb();

	/* Redirect static IP address to netusb*/
	const struct device *usb_dev = device_get_binding("eth_netusb");
	struct net_if *iface = net_if_lookup_by_dev(usb_dev);

	if (!iface) {
		printk("Cannot find network interface: %s", "eth_netusb");
		return -1;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
#endif

#ifdef CONFIG_NET_CONFIG_SETTINGS
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, we start DHCPv4 on Wi-Fi interface always, independent of the ordering.
	 */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	/* As both are Ethernet, we need to set specific interface*/
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");
#endif

	if (wifi_set_mode(true)) {
		return -1;
	}

	/* TODO: Implement waiting for WPA supplicant to manage the Wi-Fi interface.*/
	ret = try_wifi_connect();
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_NET_CAPTURE
	ret = wifi_setup_net_capture();
	if (ret) {
		return -1;
	}
	LOG_INF("Network capture of Wi-Fi traffic enabled");
#endif

	k_thread_start(receiver_thread_id);

	return 0;
}
