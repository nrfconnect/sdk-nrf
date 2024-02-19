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

#ifdef CONFIG_NET_CONFIG_SETTINGS
#include <zephyr/net/net_config.h>
#endif
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
	CF_END_SUBTYPE = 0x0E,
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
	PROBE_RESPONSE_SUBTYPE = 0x05,
	ACTION_SUBTYPE = 0x0D
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
	int actionCount;
	int cfEndCount;
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

#ifdef CONFIG_NET_CAPTURE
static const struct device *capture_dev;
#endif

#ifdef CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>

/* Fixed address as the static IP given from Kconfig will be
 * applied to Wi-Fi interface.
 */
#define CONFIG_NET_CONFIG_USB_IPV4_ADDR "192.0.2.1"
#define CONFIG_NET_CONFIG_USB_IPV4_MASK "255.255.255.0"

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
		case CF_END_SUBTYPE:
			stats->cfEndCount++;
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
		case ACTION_SUBTYPE:
			stats->actionCount++;
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
	LOG_INF("\tAction Count: %d", stats.actionCount);
	LOG_INF("Control Frames:");
	LOG_INF("\t ACK Count %d", stats.ackCount);
	LOG_INF("\t RTS Count %d", stats.rtsCount);
	LOG_INF("\t CTS Count %d", stats.ctsCount);
	LOG_INF("\t Block Ack Count %d", stats.blockAckCount);
	LOG_INF("\t Block Ack Req Count %d", stats.blockAckReqCount);
	LOG_INF("\t CF End Count %d", stats.cfEndCount);
	LOG_INF("Data Frames:");
	LOG_INF("\tData Count: %d", stats.dataCount);
	LOG_INF("\tQoS Data Count: %d", stats.qosDataCount);
	LOG_INF("\tNull Count: %d", stats.nullCount);
	LOG_INF("\tQoS Null Count: %d", stats.qosNullCount);
	LOG_INF("\n");

	return 0;
}

static int wifi_set_reg(void)
{
	struct net_if *iface;
	struct wifi_reg_domain reg = { 0 };
	int ret;

	reg.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	reg.country_code[0] = CONFIG_MONITOR_MODE_REG_DOMAIN_ALPHA2[0];
	reg.country_code[1] = CONFIG_MONITOR_MODE_REG_DOMAIN_ALPHA2[1];
	reg.force = false;

	ret = net_mgmt(NET_REQUEST_WIFI_REG_DOMAIN, iface, &reg, sizeof(reg));
	if (ret) {
		LOG_ERR("Regulatory setting failed %d", ret);
		return -1;
	}

	LOG_INF("Regulatory set to %c%c for interface %d", reg.country_code[0],
		reg.country_code[1], net_if_get_by_iface(iface));

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

	close(sock_packet.recv_sock);
}

#ifdef CONFIG_NET_CAPTURE
static int wifi_net_capture(void)
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
#ifdef CONFIG_USB_DEVICE_STACK
	struct in_addr addr;
#endif
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
	if (sizeof(CONFIG_NET_CONFIG_USB_IPV4_ADDR) > 1) {
		if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_USB_IPV4_ADDR, &addr)) {
			printk("Invalid address: %s", CONFIG_NET_CONFIG_USB_IPV4_ADDR);
			return -1;
		}
		net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
	}

	if (sizeof(CONFIG_NET_CONFIG_USB_IPV4_MASK) > 1) {
		/* If not empty */
		if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_USB_IPV4_MASK, &addr)) {
			printk("Invalid netmask: %s", CONFIG_NET_CONFIG_USB_IPV4_MASK);
		} else {
			net_if_ipv4_set_netmask(iface, &addr);
		}
	}
#endif

#ifdef CONFIG_NET_CONFIG_SETTINGS
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, we start DHCPv4 on Wi-Fi interface always, independent of the ordering.
	 */
	/* TODO: Replace device name with DTS settings later */
	const struct device *dev = device_get_binding("wlan0");
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	/* As both are Ethernet, we need to set specific interface*/
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");
#endif

	ret = wifi_set_mode();
	if (ret) {
		return -1;
	}

	ret = wifi_set_reg();
	if (ret) {
		return -1;
	}

	ret = wifi_set_channel();
	if (ret) {
		return -1;
	}

#ifdef CONFIG_NET_CAPTURE
	ret = wifi_net_capture();
	if (ret) {
		return -1;
	}
	LOG_INF("Network capture of Wi-Fi traffic enabled");
#endif

	k_thread_start(receiver_thread_id);

	k_thread_join(receiver_thread_id, K_FOREVER);

#ifdef CONFIG_NET_CAPTURE
	ret = net_capture_disable(capture_dev);
	if (ret < 0) {
		LOG_ERR("Failed to disable network capture : %d", ret);
	}
#endif

	return 0;
}
