/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Raw Tx Packet sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_tx_packet, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

#include <zephyr/net/socket.h>

#include "net_private.h"
#include "wifi_connection.h"

#define BEACON_PAYLOAD_LENGTH 256
#define CONTINUOUS_MODE_TRANSMISSION 0
#define FIXED_MODE_TRANSMISSION 1

#define IEEE80211_SEQ_CTRL_SEQ_NUM_MASK 0xFFF0
#define IEEE80211_SEQ_NUMBER_INC BIT(4) /* 0-3 is fragment number */
#define NRF_WIFI_MAGIC_NUM_RAWTX 0x12345678

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
		0X0C, 0XA2, 0X28, 0X00, 0X00, 0X00, 0X00, 0X00, 0X64, 0X00,
		0X11, 0X04, 0X00, 0X15, 0X4E, 0X52, 0X46, 0X5F, 0X52, 0X41,
		0X57, 0X5F, 0X54, 0X58, 0X5F, 0X50, 0X41, 0X43, 0X4B, 0X45,
		0X54, 0X5F, 0X41, 0X50, 0X50, 0X01, 0X08, 0X82, 0X84, 0X8B,
		0X96, 0X0C, 0X12, 0X18, 0X24, 0X03, 0X01, 0X06, 0X05, 0X04,
		0X00, 0X02, 0X00, 0X00, 0X2A, 0X01, 0X04, 0X32, 0X04, 0X30,
		0X48, 0X60, 0X6C, 0X30, 0X14, 0X01, 0X00, 0X00, 0X0F, 0XAC,
		0X04, 0X01, 0X00, 0X00, 0X0F, 0XAC, 0X04, 0X01, 0X00, 0X00,
		0X0F, 0XAC, 0X02, 0X0C, 0X00, 0X3B, 0X02, 0X51, 0X00, 0X2D,
		0X1A, 0X0C, 0X00, 0X17, 0XFF, 0XFF, 0X00, 0X00, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X2C, 0X01, 0X01, 0X00, 0X00, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00, 0X00, 0X3D, 0X16, 0X06, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X7F, 0X08, 0X04, 0X00,
		0X00, 0X02, 0X00, 0X00, 0X00, 0X40, 0XFF, 0X1A, 0X23, 0X01,
		0X78, 0X10, 0X1A, 0X00, 0X00, 0X00, 0X20, 0X0E, 0X09, 0X00,
		0X09, 0X80, 0X04, 0X01, 0XC4, 0X00, 0XFA, 0XFF, 0XFA, 0XFF,
		0X61, 0X1C, 0XC7, 0X71, 0XFF, 0X07, 0X24, 0XF0, 0X3F, 0X00,
		0X81, 0XFC, 0XFF, 0XDD, 0X18, 0X00, 0X50, 0XF2, 0X02, 0X01,
		0X01, 0X01, 0X00, 0X03, 0XA4, 0X00, 0X00, 0X27, 0XA4, 0X00,
		0X00, 0X42, 0X43, 0X5E, 0X00, 0X62, 0X32, 0X2F, 0X00
	}
};

/* TODO: Copied from nRF70 Wi-Fi driver, need to be moved to a common place */
struct raw_tx_pkt_header {
	unsigned int magic_num;
	unsigned char data_rate;
	unsigned short packet_length;
	unsigned char tx_mode;
	unsigned char queue;
	unsigned char raw_tx_flag;
};

#ifdef CONFIG_RAW_TX_PKT_SAMPLE_NON_CONNECTED_MODE
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
	channel_info.channel = CONFIG_RAW_TX_PKT_SAMPLE_CHANNEL;
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
#endif

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

#ifdef CONFIG_RAW_TX_PKT_SAMPLE_INJECTION_ENABLE
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
#endif /* CONFIG_RAW_TX_PKT_SAMPLE_INJECTION_ENABLE */

static int setup_raw_pkt_socket(int *sockfd, struct sockaddr_ll *sa)
{
	struct net_if *iface = NULL;
	int ret;

	*sockfd = socket(AF_PACKET, SOCK_RAW, htons(IPPROTO_RAW));
	if (*sockfd < 0) {
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
	ret = bind(*sockfd, (struct sockaddr *)sa, sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Error: Unable to bind socket to the network interface:%s",
			strerror(errno));
		close(*sockfd);
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

int wifi_send_raw_tx_pkt(int sockfd, char *test_frame,
			size_t buf_length, struct sockaddr_ll *sa)
{
	return sendto(sockfd, test_frame, buf_length, 0,
			(struct sockaddr *)sa, sizeof(*sa));
}

static int get_pkt_transmit_count(unsigned int *mode_of_transmission,
					unsigned int *num_tx_pkts)
{
#ifdef CONFIG_RAW_TX_PKT_SAMPLE_TX_MODE_FIXED
	*mode_of_transmission = 1;
	*num_tx_pkts = CONFIG_RAW_TX_PKT_SAMPLE_FIXED_NUM_PACKETS;
	if (*num_tx_pkts == 0) {
		LOG_ERR("Can't send %d number of raw tx packets", *num_tx_pkts);
		return -1;
	}
	LOG_INF("Sending %d number of raw tx packets", *num_tx_pkts);
#else
	*mode_of_transmission = 0;
	*num_tx_pkts = UINT_MAX;
	LOG_INF("Sending raw tx packets continuously");
#endif
	return 0;
}

static void increment_seq_control(void)
{
	test_beacon_frame.seq_ctrl = (test_beacon_frame.seq_ctrl +
				      IEEE80211_SEQ_NUMBER_INC) &
				      IEEE80211_SEQ_CTRL_SEQ_NUM_MASK;
	if (test_beacon_frame.seq_ctrl > IEEE80211_SEQ_CTRL_SEQ_NUM_MASK) {
		test_beacon_frame.seq_ctrl = 0X0010;
	}
}

static void wifi_send_raw_tx_packets(void)
{
	struct sockaddr_ll sa;
	int sockfd, ret;
	struct raw_tx_pkt_header packet;
	char *test_frame = NULL;
	unsigned int buf_length, num_pkts, transmission_mode, num_failures = 0;

	ret = setup_raw_pkt_socket(&sockfd, &sa);
	if (ret < 0) {
		LOG_ERR("Setting socket for raw pkt transmission failed %d", errno);
		return;
	}

	fill_raw_tx_pkt_hdr(&packet);

	ret = get_pkt_transmit_count(&transmission_mode, &num_pkts);
	if (ret < 0) {
		close(sockfd);
		return;
	}

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame));
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame);
	memcpy(test_frame, &packet, sizeof(struct raw_tx_pkt_header));

	if (num_pkts == 1) {
		memcpy(test_frame + sizeof(struct raw_tx_pkt_header),
		       &test_beacon_frame, sizeof(test_beacon_frame));

		ret = wifi_send_raw_tx_pkt(sockfd, test_frame, buf_length, &sa);
		if (ret < 0) {
			LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
			close(sockfd);
			free(test_frame);
			return;
		}
	} else {
		for (int i = 0; i < num_pkts; i++) {
			memcpy(test_frame + sizeof(struct raw_tx_pkt_header),
			       &test_beacon_frame, sizeof(test_beacon_frame));

			ret = sendto(sockfd, test_frame, buf_length, 0,
					(struct sockaddr *)&sa, sizeof(sa));
			if (ret < 0) {
				LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
				num_failures++;
			}

			increment_seq_control();

			k_msleep(CONFIG_RAW_TX_PKT_SAMPLE_INTER_FRAME_DELAY_MS);
		}
	}

	LOG_INF("Sent %d packets with %d failures on socket", num_pkts, num_failures);

	/* close the socket */
	close(sockfd);
	free(test_frame);
}

int main(void)
{
	int mode;

	mode = BIT(0);
	/* This is to set mode to STATION */
	wifi_set_mode(mode);

#ifdef CONFIG_RAW_TX_PKT_SAMPLE_INJECTION_ENABLE
	if (wifi_set_tx_injection_mode()) {
		return -1;
	}
#endif

#ifdef CONFIG_RAW_TX_PKT_SAMPLE_CONNECTION_MODE
	int status;

	status = try_wifi_connect();
	if (status < 0) {
		return status;
	}
#else
	wifi_set_channel();
#endif
	wifi_send_raw_tx_packets();

	return 0;
}
