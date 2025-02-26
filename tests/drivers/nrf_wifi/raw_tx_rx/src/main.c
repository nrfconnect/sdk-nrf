/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/socket.h>

#include "net_private.h"

#include <zephyr/ztest.h>

#define BEACON_PAYLOAD_LENGTH	     256
#define QOS_PAYLOAD_LENGTH	     32
#define CONTINUOUS_MODE_TRANSMISSION 0
#define FIXED_MODE_TRANSMISSION	     1

#define IEEE80211_SEQ_CTRL_SEQ_NUM_MASK 0xFFF0
#define IEEE80211_SEQ_NUMBER_INC	BIT(4) /* 0-3 is fragment number */
#define NRF_WIFI_MAGIC_NUM_RAWTX	0x12345678

int raw_socket_fd;
int rx_raw_socket_fd;

bool packet_send = true;

#define RECV_BUFFER_SIZE    1024
#define RAW_PKT_DATA_OFFSET 6

struct packet_data {
	char recv_buffer[RECV_BUFFER_SIZE];
};

struct packet_data test_packet;
#define STACK_SIZE	CONFIG_RX_THREAD_STACK_SIZE
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
struct sockaddr_ll sa;
struct sockaddr_ll dst;

static void rx_thread_oneshot(void);

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE, rx_thread_oneshot, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

struct beacon {
	uint16_t frame_control;
	uint16_t duration;
	uint8_t da[6];
	uint8_t sa[6];
	uint8_t bssid[6];
	uint16_t seq_ctrl;
	uint8_t payload[BEACON_PAYLOAD_LENGTH];
} __packed;

typedef struct {
    /* Standard 802.11 frame header fields */
    uint16_t frame_control;  /* Frame control field (including subtype: QoS data) */
    uint16_t duration;
    uint8_t  address1[6];  /* Receiver address */
    uint8_t  address2[6];  /* Transmitter address */
    uint8_t  address3[6];  /* BSSID */
    uint16_t sequence_control; /* Sequence control field */

    /* QoS specific fields */
    uint16_t QoS;
    /* Data payload */
    uint8_t  data[QOS_PAYLOAD_LENGTH];
    uint32_t frame_check_sequence;
} wifi_qos_packet;

static struct beacon test_beacon_frame  = {
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
	LOG_INF("Mode setting set to mode_val = %d", mode_val);
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

static int setup_rawrecv_socket(struct sockaddr_ll *dst)
{
        int ret;
        struct timeval timeo_optval = {
                .tv_sec = 0,
                .tv_usec = 10000,
        };
	
	rx_raw_socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (rx_raw_socket_fd < 0) {
                LOG_ERR("Failed to create RAW socket : %d",
                                errno);
                return -errno;
        }

        dst->sll_ifindex = net_if_get_by_iface(net_if_get_first_wifi());
        dst->sll_family = AF_PACKET;

        ret = bind(rx_raw_socket_fd, (const struct sockaddr *)dst,
                        sizeof(struct sockaddr_ll));
        if (ret < 0) {
                LOG_ERR("Failed to bind packet socket : %d", errno);
                return -errno;
        }

	ret = setsockopt(rx_raw_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
				sizeof(timeo_optval));
	if (ret < 0) {
		LOG_ERR("Failed to set socket options : %s", strerror(errno));
		return -errno;
	}
	return 0;
}

static int setup_raw_pkt_socket(struct sockaddr_ll *sa)
{
	struct net_if *iface = NULL;
	int ret;

	raw_socket_fd = socket(AF_PACKET, SOCK_RAW, htons(IPPROTO_RAW));
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
	memcpy(test_frame + sizeof(struct raw_tx_pkt_header), &test_beacon_frame,
	       sizeof(test_beacon_frame));

	int ret = wifi_send_raw_tx_pkt(sockfd, test_frame, buf_length, sa);
	if (ret < 0) {
		LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
		return -1;
	}

	LOG_INF("Wi-Fi RaW TX Packet sent via socket returning success");
	return 0;
}

static int wifi_send_raw_tx_packets()
{
	int ret;
	struct raw_tx_pkt_header packet;
	char *test_frame = NULL;
	unsigned int buf_length;

	fill_raw_tx_pkt_hdr(&packet);

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame));
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return -1;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame);
	memcpy(test_frame, &packet, sizeof(struct raw_tx_pkt_header));

	LOG_INF("Wi-Fi sending RAW TX Packet");
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
	int count =0;
	memset(packet->recv_buffer, 0, RECV_BUFFER_SIZE);
	LOG_INF("waiting on packet to be received");
	received = recv(rx_raw_socket_fd, packet->recv_buffer, sizeof(packet->recv_buffer), 0);
	if (received <= 0) {
		if (errno == EAGAIN) {
			LOG_INF("EAGAIN error - received = %d", received);
			return 0;
		}

		if (received < 0) {
			LOG_ERR("recv error %s", strerror(errno));
			return -errno;
		}
	}

	LOG_INF("sizeof of received frame is %d", received);

	for (count = 0; count < received; count++)
	{
		printf("0x%x ", packet->recv_buffer[count]);
		if (((count % 14) == 0) && count != 0)
			printf("\n");
	}

	LOG_INF("packet received");
	return 0;
}

/* handle incoming wifi packets in monitor mode */
static void rx_thread_oneshot()
{
	while (packet_send)
	{
		process_single_rx_packet(&test_packet);
	}
}

static void initialize(void) {
	/* MONITOR mode */
	int mode = BIT(1);
	wifi_set_mode(mode);
	wifi_set_tx_injection_mode();
	wifi_set_channel();
	setup_raw_pkt_socket(&sa);
}

static void cleanup(void) {
	close(raw_socket_fd);
}

ZTEST(nrf_wifi, test_raw_tx_rx)
{
	/* TODO: Wait for interface to be operationally UP */
	int count = CONFIG_RAW_TX_RX_TRANSMIT_COUNT;

	setup_rawrecv_socket(&dst);
	k_thread_start(receiver_thread_id);

	LOG_INF("TX count is set is %d", CONFIG_RAW_TX_RX_TRANSMIT_COUNT);
	while (count--)
	{
		k_sleep(K_MSEC(3));
		zassert_false(wifi_send_raw_tx_packets(), "Failed to send raw tx packet");
		k_sleep(K_MSEC(3));
	}
	packet_send = false;
	LOG_INF("Waiting on RX Thread to exit");
	k_thread_join(receiver_thread_id, K_SECONDS(1));
	close(rx_raw_socket_fd);
	LOG_INF("RX Thread has exited");
}

/* #define RAW_TX_TEST */
/* currently all packets are being looped back. UMAC/LMAC should have
 * a configuration to differentiate loopback and burst mode and the configuration
 * can be set via network message to the driver/UMAC from the ZTEST application.
 * Enable code if you wish to test or when the configuration in available
 */
/* #define RAW_TX_TEST */
#ifdef RAW_TX_TEST
ZTEST(nrf_wifi, test_raw_tx)
{
	int count = CONFIG_RAW_TX_TRANSMIT_COUNT;

	LOG_INF("TX burst count is set is %d", CONFIG_RAW_TX_TRANSMIT_COUNT);
	while (count--)
	{
		k_sleep(K_MSEC(1));
		zassert_false(wifi_send_raw_tx_packets(), "Failed to send raw tx packet");
	}
}
#endif

ZTEST_SUITE(nrf_wifi, NULL, (void *)initialize, NULL, NULL, (void *)cleanup);
