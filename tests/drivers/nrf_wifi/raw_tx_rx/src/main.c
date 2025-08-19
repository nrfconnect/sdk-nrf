/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/ztest.h>

#ifdef CONFIG_SOC_NRF7120_PREENG
#include <soc.h>
#endif /* CONFIG_SOC_NRF7120_PREENG */

#define BEACON_PAYLOAD_LENGTH	     256
#define CONTINUOUS_MODE_TRANSMISSION 0
#define FIXED_MODE_TRANSMISSION	     1

#define IEEE80211_SEQ_CTRL_SEQ_NUM_MASK 0xFFF0
#define IEEE80211_SEQ_NUMBER_INC	BIT(4) /* 0-3 is fragment number */
#define NRF_WIFI_MAGIC_NUM_RAWTX	0x12345678

int raw_socket_fd;

#define RECV_BUFFER_SIZE    1024
#define RAW_PKT_DATA_OFFSET 6

struct packet_data {
	char recv_buffer[RECV_BUFFER_SIZE];
};

struct packet_data test_packet;
#define STACK_SIZE	CONFIG_RX_THREAD_STACK_SIZE
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
struct sockaddr_ll sa;

static void rx_thread_oneshot(void);

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE, rx_thread_oneshot, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

#define NRF_WIFICORE_RPURFBUS_BASE		     0x48020000UL
#define NRF_WIFICORE_RPURFBUS_RFCTRL		     ((NRF_WIFICORE_RPURFBUS_BASE) + 0x00010000UL)
#define NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS ((NRF_WIFICORE_RPURFBUS_RFCTRL) + 0x00000000UL)

#define RDW(addr)	(*(volatile unsigned int *)(addr))
#define WRW(addr, data) (*(volatile unsigned int *)(addr) = (data))

struct beacon {
	uint16_t frame_control;
	uint16_t duration;
	uint8_t da[6];
	uint8_t sa[6];
	uint8_t bssid[6];
	uint16_t seq_ctrl;
	uint8_t payload[BEACON_PAYLOAD_LENGTH];
} __packed;

static struct beacon test_beacon_frame = {
	.frame_control = htons(0X8000),
	.duration = 0X0000,
	.da = {0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF},
	/* Transmitter Address: A0:69:60:E3:52:15 */
	.sa = {0XA0, 0X69, 0X60, 0XE3, 0X52, 0X15},
	.bssid = {0XA0, 0X69, 0X60, 0XE3, 0X52, 0X15},
	.seq_ctrl = 0X0001,
	/* SSID: NRF_RAW_TX_PACKET_APP */
	.payload = {
		0X0C, 0XA2, 0X28, 0X00, 0X00, 0X00, 0X00, 0X00, 0X64, 0X00, 0X11, 0X04, 0X00, 0X15,
		0X4E, 0X52, 0X46, 0X5F, 0X52, 0X41, 0X57, 0X5F, 0X54, 0X58, 0X5F, 0X50, 0X41, 0X43,
		0X4B, 0X45, 0X54, 0X5F, 0X41, 0X50, 0X50, 0X01, 0X08, 0X82, 0X84, 0X8B, 0X96, 0X0C,
		0X12, 0X18, 0X24, 0X03, 0X01, 0X06, 0X05, 0X04, 0X00, 0X02, 0X00, 0X00, 0X2A, 0X01,
		0X04, 0X32, 0X04, 0X30, 0X48, 0X60, 0X6C, 0X30, 0X14, 0X01, 0X00, 0X00, 0X0F, 0XAC,
		0X04, 0X01, 0X00, 0X00, 0X0F, 0XAC, 0X04, 0X01, 0X00, 0X00, 0X0F, 0XAC, 0X02, 0X0C,
		0X00, 0X3B, 0X02, 0X51, 0X00, 0X2D, 0X1A, 0X0C, 0X00, 0X17, 0XFF, 0XFF, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00, 0X00, 0X2C, 0X01, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X3D, 0X16, 0X06, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X7F, 0X08, 0X04, 0X00,
		0X00, 0X02, 0X00, 0X00, 0X00, 0X40, 0XFF, 0X1A, 0X23, 0X01, 0X78, 0X10, 0X1A, 0X00,
		0X00, 0X00, 0X20, 0X0E, 0X09, 0X00, 0X09, 0X80, 0X04, 0X01, 0XC4, 0X00, 0XFA, 0XFF,
		0XFA, 0XFF, 0X61, 0X1C, 0XC7, 0X71, 0XFF, 0X07, 0X24, 0XF0, 0X3F, 0X00, 0X81, 0XFC,
		0XFF, 0XDD, 0X18, 0X00, 0X50, 0XF2, 0X02, 0X01, 0X01, 0X01, 0X00, 0X03, 0XA4, 0X00,
		0X00, 0X27, 0XA4, 0X00, 0X00, 0X42, 0X43, 0X5E, 0X00, 0X62, 0X32, 0X2F, 0X00}};

/* TODO: Copied from nRF70 Wi-Fi driver, need to be moved to a common place */
struct raw_tx_pkt_header {
	unsigned int magic_num;
	unsigned char data_rate;
	unsigned short packet_length;
	unsigned char tx_mode;
	unsigned char queue;
	unsigned char raw_tx_flag;
};

static void wifi_set_mode(int mode_val)
{
	int ret;
	struct net_if *iface = NULL;
	struct wifi_mode_info mode_info = {0};

	mode_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	mode_info.if_index = net_if_get_by_iface(iface);
	mode_info.mode = mode_val;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed %d", ret);
	}
}

static int wifi_set_tx_injection_mode(void)
{
	struct net_if *iface;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -1;
	}

	if (net_eth_txinjection_mode(iface, true)) {
		LOG_ERR("TX Injection mode enable failed");
		return -1;
	}

	LOG_INF("TX Injection mode enabled");
	return 0;
}

static int wifi_set_channel(void)
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = {0};
	int ret;

	channel_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -1;
	}

	channel_info.if_index = net_if_get_by_iface(iface);
	channel_info.channel = CONFIG_NRF_WIFI_RAW_TX_PKT_SAMPLE_CHANNEL;
	if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
	    (channel_info.channel > WIFI_CHANNEL_MAX)) {
		LOG_ERR("Invalid channel number. Range is (1-233)");
		return -1;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface, &channel_info, sizeof(channel_info));
	if (ret) {
		LOG_ERR(" Channel setting failed %d\n", ret);
		return -1;
	}

	LOG_INF("Wi-Fi channel set to %d", channel_info.channel);
	return 0;
}

static int setup_raw_pkt_socket(struct sockaddr_ll *sa)
{
	struct net_if *iface = NULL;
	int ret;

	raw_socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (raw_socket_fd < 0) {
		LOG_ERR("Unable to create a socket %d", errno);
		return -1;
	}

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	sa->sll_family = AF_PACKET;
	sa->sll_ifindex = net_if_get_by_iface(iface);

	/* Bind the socket */
	ret = bind(raw_socket_fd, (struct sockaddr *)sa, sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Error: Unable to bind socket to the network interface:%s",
			strerror(errno));
		return -1;
	}

	return 0;
}

static void fill_raw_tx_pkt_hdr(struct raw_tx_pkt_header *raw_tx_pkt)
{
	/* Raw Tx Packet header */
	raw_tx_pkt->magic_num = NRF_WIFI_MAGIC_NUM_RAWTX;
	raw_tx_pkt->data_rate = CONFIG_RAW_TX_PKT_SAMPLE_RATE_VALUE;
	raw_tx_pkt->packet_length = sizeof(test_beacon_frame);
	raw_tx_pkt->tx_mode = CONFIG_RAW_TX_PKT_SAMPLE_RATE_FLAGS;
	raw_tx_pkt->queue = CONFIG_RAW_TX_PKT_SAMPLE_QUEUE_NUM;
	/* The byte is reserved and used by the driver */
	raw_tx_pkt->raw_tx_flag = 0;
}

int wifi_send_raw_tx_pkt(int sockfd, char *test_frame, size_t buf_length, struct sockaddr_ll *sa)
{
	return sendto(sockfd, test_frame, buf_length, 0, (struct sockaddr *)sa, sizeof(*sa));
}

static int wifi_send_single_raw_tx_packet(int sockfd, char *test_frame, size_t buf_length,
					  struct sockaddr_ll *sa)
{
	int ret;

	memcpy(test_frame + sizeof(struct raw_tx_pkt_header), &test_beacon_frame,
	       sizeof(test_beacon_frame));
	ret = wifi_send_raw_tx_pkt(sockfd, test_frame, buf_length, sa);
	if (ret < 0) {
		LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
		return -1;
	}

	return 0;
}

static int wifi_send_raw_tx_packets(unsigned int num_pkts)
{
	int ret;
	struct raw_tx_pkt_header packet;
	char *test_frame = NULL;
	unsigned int buf_length;

	ARG_UNUSED(num_pkts);

	fill_raw_tx_pkt_hdr(&packet);

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame));
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return -1;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame);
	memcpy(test_frame, &packet, sizeof(struct raw_tx_pkt_header));

	ret = wifi_send_single_raw_tx_packet(raw_socket_fd, test_frame, buf_length, &sa);
	if (ret < 0) {
		LOG_ERR("Failed to send raw tx packets");
	}

	free(test_frame);
	return ret;
}

static int process_single_rx_packet(struct packet_data *packet)
{
	int received;

	LOG_INF("Wi-Fi monitor mode RX thread started");

	received = recv(raw_socket_fd, packet->recv_buffer, sizeof(packet->recv_buffer), 0);
	if (received <= 0) {
		if (errno == EAGAIN) {
			return 0;
		}

		if (received < 0) {
			LOG_ERR("Monitor : recv error %s", strerror(errno));
			return -errno;
		}
	}

	return 0;
}

/* handle incoming wifi packets in monitor mode */
static void rx_thread_oneshot(void)
{
	int ret;
	struct timeval timeo_optval = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	ret = setsockopt(raw_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
			 sizeof(timeo_optval));
	if (ret < 0) {
		LOG_ERR("Failed to set socket options : %s", strerror(errno));
		return;
	}

	process_single_rx_packet(&test_packet);
}

ZTEST(nrf_wifi, test_single_raw_tx_rx)
{
	struct net_if *iface;
	struct wifi_filter_info filter_info = {0};
	int ret;
	/* MONITOR mode */
	int mode = BIT(1);

	wifi_set_mode(mode);
	zassert_false(wifi_set_tx_injection_mode(), "Failed to set TX injection mode");
	zassert_equal(wifi_set_channel(), 0, "Failed to set channel");
	/* Configure packet filter with buffer size 1550 */
	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface for packet filter");
		return;
	}

	filter_info.oper = WIFI_MGMT_SET;
	filter_info.if_index = net_if_get_by_iface(iface);
	filter_info.filter = WIFI_PACKET_FILTER_ALL;
	filter_info.buffer_size = 1550;

	ret = net_mgmt(NET_REQUEST_WIFI_PACKET_FILTER, iface, &filter_info, sizeof(filter_info));
	if (ret) {
		LOG_ERR("Packet filter setting failed %d", ret);
	} else {
		LOG_INF("Packet filter set with buffer size %d", filter_info.buffer_size);
	}
	zassert_false(setup_raw_pkt_socket(&sa), "Setting socket for raw pkt transmission failed");
#ifdef CONFIG_SOC_NRF7120_PREENG
	configure_playout_capture(0, 1, 0x7F, 0xCA60, 0);
#endif /* CONFIG_SOC_NRF7120_PREENG */
	k_thread_start(receiver_thread_id);
	/* TODO: Wait for interface to be operationally UP */
	k_sleep(K_MSEC(50));
	zassert_false(wifi_send_raw_tx_packets(1), "Failed to send raw tx packets");
	zassert_not_equal(
		k_thread_join(receiver_thread_id, K_SECONDS(CONFIG_NRF_WIFI_RAW_RX_PKT_TIMEOUT_S)),
		0, "Thread join failed/timedout");
	zassert_mem_equal(&test_beacon_frame, &test_packet.recv_buffer[RAW_PKT_DATA_OFFSET],
			  sizeof(test_beacon_frame), "Mismatch in sent and received data");

	close(raw_socket_fd);
}

ZTEST_SUITE(nrf_wifi, NULL, NULL, NULL, NULL, NULL);
