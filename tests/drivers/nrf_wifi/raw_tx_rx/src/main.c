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
#include <fmac_main.h>
#include <fmac_api.h>
#include <fmac_api_common.h>
#include "net_private.h"
#if 1 //ndef CONFIG_RAW_TX_BURST
#include <zephyr/ztest.h>
#endif
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#define BEACON_PAYLOAD_LENGTH	     256
#define QOS_PAYLOAD_LENGTH	     32
#define CONTINUOUS_MODE_TRANSMISSION 0
#define FIXED_MODE_TRANSMISSION	     1

#define IEEE80211_SEQ_CTRL_SEQ_NUM_MASK 0xFFF0
#define IEEE80211_SEQ_NUMBER_INC	BIT(4) /* 0-3 is fragment number */
#define NRF_WIFI_MAGIC_NUM_RAWTX	0x12345678

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
struct nrf_wifi_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

int raw_socket_fd;
int rx_raw_socket_fd;

bool packet_send = true;
unsigned int rx_total_received_bytes = 0;
unsigned int rx_received_bytes_last_sec = 0;
#define ONE_MB 1000000
#define ONE_KB 1000

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
static void tx_thread_transmit(void); 

static unsigned int total_tx_bytes, total_tx_bytes_tmp;
static unsigned int first_pkt_timestamp;
static unsigned int last_pkt_timestamp;

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE, rx_thread_oneshot, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

K_THREAD_DEFINE(transmit_thread_id, STACK_SIZE, tx_thread_transmit, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

#define RDW(addr)           (*(volatile unsigned int *)(addr))
#define WRW(addr, data)     (*(volatile unsigned int *)(addr) = (data))

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
	uint16_t QoS; /* QoS Control field */
} wifi_qos_packet;

/* Statically define a wifi_qos_packet header using values from the beacon */
static const wifi_qos_packet test_qos_header = {
	.frame_control = htons(0x8800), // QoS Data frame type/subtype
	.duration = 0x0000,
	.address1 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // DA from beacon
	.address2 = {0xA0, 0x69, 0x60, 0xE3, 0x52, 0x15}, // SA from beacon
	.address3 = {0xA0, 0x69, 0x60, 0xE3, 0x52, 0x15}, // BSSID from beacon
	.sequence_control = 0x0001,
	.QoS = 0x0000
};

static int wait_for_tx_complete(int timeout_ms)
{
    int ret = -ETIMEDOUT; // Default to timeout
    int elapsed_ms = 0;
    int sleep_interval_ms = 2; // Your original sleep interval

    // Record the start time
    unsigned int start_time = k_uptime_get_32();

    while (!tx_complete) {
        if (elapsed_ms >= timeout_ms) {
			LOG_INF("Timeout occurred while waiting for TX completion: %d ms", timeout_ms);
            // Timeout occurred
            break;
        }

        // Sleep for a short interval
        k_msleep(sleep_interval_ms);

        // Update elapsed time (ensure it doesn't exceed timeout_ms if sleep_interval_ms is large)
        elapsed_ms = k_uptime_get_32() - start_time;
        if (elapsed_ms > timeout_ms) {
            elapsed_ms = timeout_ms; // Cap elapsed_ms at timeout_ms
        }
    }

    if (tx_complete) {
        ret = 0; // Success
    }

    return ret;
}


void configurePlayoutCapture(uint32_t pktLen, uint32_t holdOff, uint32_t flushBytes)
{
#if 0
		// Switch off protection to RF regs
	WRW((uintptr_t)0x48030000, 0x00000000);
	// Use Cabled connection for EMU-EMU
 	WRW((uintptr_t)0x4802000C, 0x2);
	return;
#endif
#if (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP && CONFIG_EMULATOR_SYSTEMC) || (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU)
	unsigned int value;
	LOG_INF("%s: Setting Playout capture settings",__func__);
	// Set AXI Master to APP so we can access the RF
	NRF_WIFICORE_RPURFBUS->RFCTRL.AXIMASTERACCESS = 0x31;
	while(NRF_WIFICORE_RPURFBUS->RFCTRL.AXIMASTERACCESS != 0x31);
#ifdef CONFIG_RAW_TX_RX_LOOPBACK
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0, 0x2);
    value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0);
    LOG_INF("%s: WRW setting to value 0x02 is 0x%x",__func__, value);
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4, holdOff);
    value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4);
    LOG_INF("%s: WRW setting to value 0x04 is 0x%x",__func__, value);
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8, (pktLen + holdOff + flushBytes));
    value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8);
    LOG_INF("%s: WRW setting to value 0x08 is 0x%x",__func__, value);
#endif
#ifdef CONFIG_RAW_RX_BURST
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0, 0x3);
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0);
	LOG_INF("%s: WRW setting to value 0x03 is 0x%x",__func__, value);
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4, holdOff);
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4);
	LOG_INF("%s: WRW setting to value 0x04 is 0x%x",__func__, value);
	WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8, (pktLen + holdOff + flushBytes));
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8);
	LOG_INF("%s: WRW setting to value 0x08 is 0x%x",__func__, value);
#endif
#ifdef CONFIG_RAW_TX_BURST
    WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0, 0x3);
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x0);
	LOG_INF("%s: WRW setting to value 0x03 is 0x%x",__func__, value);
	WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4, 0x7F);
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x4);
	LOG_INF("%s: WRW setting to value 0x04 is 0x%x",__func__, value);
	WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8, 1);
	value = RDW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0x8);
	LOG_INF("%s: WRW setting to value 0x08 is 0x%x",__func__, value);
#endif
	// Switch to the RF playout
	WRW((uintptr_t)NRF_WIFICORE_RPURFBUS + 0xC, 0x1);
	LOG_INF("%s: Playput capture settings configured",__func__);
#endif /* (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP && CONFIG_EMULATOR_SYSTEMC) || (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU) */
}

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
#if 0
	ret = setsockopt(rx_raw_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
				sizeof(timeo_optval));
	if (ret < 0) {
		LOG_ERR("Failed to set socket options : %s", strerror(errno));
		return -errno;
	}
#endif
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

static void fill_raw_tx_pkt_hdr(struct raw_tx_pkt_header *raw_tx_pkt, 
				unsigned char payload_size,
				unsigned char rate_value,
				unsigned char rate_flags,
				unsigned char queue_num)
{
	/* Raw Tx Packet header */
	raw_tx_pkt->magic_num = NRF_WIFI_MAGIC_NUM_RAWTX;
	raw_tx_pkt->data_rate = rate_value;
	raw_tx_pkt->packet_length = sizeof(test_qos_header) + payload_size;
	raw_tx_pkt->tx_mode = rate_flags;
	raw_tx_pkt->queue = queue_num;
}

int wifi_send_raw_tx_pkt(int sockfd, char *test_frame, size_t buf_length, struct sockaddr_ll *sa)
{
	int ret = sendto(sockfd, test_frame, buf_length, 0, (struct sockaddr *)sa, sizeof(*sa));
	if (ret < 0) {
		LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
		return ret;
	}

	total_tx_bytes += ret;
	if (!first_pkt_timestamp) {
		first_pkt_timestamp = k_uptime_get_32();
	} else {
		last_pkt_timestamp = k_uptime_get_32();
	}

	return ret;
}

static int wifi_send_single_raw_tx_packet(int sockfd, char *test_frame, size_t buf_length,
					  struct sockaddr_ll *sa)
{
	int ret = wifi_send_raw_tx_pkt(sockfd, test_frame, buf_length, sa);
	if (ret < 0) {
		LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
		return ret;
	}

	LOG_DBG("Wi-Fi RaW TX Packet sent via socket returning success");
	return ret;
}

static char *g_test_frame = NULL;
static unsigned int g_buf_length;

static int wifi_send_raw_tx_packets_init(unsigned int payload_size, 
					 unsigned char rate_value,
					 unsigned char rate_flags,
					 unsigned char queue_num)
{
	struct raw_tx_pkt_header packet;

	fill_raw_tx_pkt_hdr(&packet, payload_size, rate_value, rate_flags, queue_num);

	g_test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
			      payload_size);
	if (!g_test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return -1;
	}

	g_buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
		       payload_size;
	memcpy(g_test_frame, &packet, sizeof(struct raw_tx_pkt_header));
	memcpy(g_test_frame + sizeof(struct raw_tx_pkt_header), &test_qos_header,
	       sizeof(test_qos_header));
	memset(g_test_frame + sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header), 0xAB,
	       payload_size);
	return 0;
}

static int wifi_send_raw_tx_packets_tx(void)
{
	int ret;

	LOG_DBG("Wi-Fi sending RAW TX Packet");
	ret = wifi_send_single_raw_tx_packet(raw_socket_fd, g_test_frame, g_buf_length, &sa);
	if (ret < 0) {
		LOG_ERR("Failed to send raw tx packets");
	} else {
		tx_sent++;
		LOG_DBG("Wi-Fi RAW TX Packet sent successfully");
	}

	return ret;
}

static void wifi_send_raw_tx_packets_deinit(void)
{
	if (g_test_frame) {
		free(g_test_frame);
		g_test_frame = NULL;
	}
}

static int wifi_send_raw_tx_packets(void)
{
	struct raw_tx_pkt_header packet;
	char *test_frame = NULL;
	unsigned int buf_length;
	int ret;

	fill_raw_tx_pkt_hdr(&packet, 
			     CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE,
			     CONFIG_RAW_TX_PKT_SAMPLE_RATE_VALUE,
			     CONFIG_RAW_TX_PKT_SAMPLE_RATE_FLAGS,
			     CONFIG_RAW_TX_PKT_SAMPLE_QUEUE_NUM);

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
			    CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE);
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return -1;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
		     CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE;
	memcpy(test_frame, &packet, sizeof(struct raw_tx_pkt_header));
	memcpy(test_frame + sizeof(struct raw_tx_pkt_header), &test_qos_header,
	       sizeof(test_qos_header));
	memset(test_frame + sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header), 0xAB,
	       CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE);

	LOG_INF("Wi-Fi sending RAW TX Packet");
	ret = wifi_send_single_raw_tx_packet(raw_socket_fd, test_frame, buf_length, &sa);
	if (ret < 0) {
		LOG_ERR("Failed to send raw tx packets");
		free(test_frame);
		return -1;
	}

	free(test_frame);
	return 0;
}

static int process_single_rx_packet(struct packet_data *packet)
{
	int received;
	int count =0;
	memset(packet->recv_buffer, 0, RECV_BUFFER_SIZE);
/* removing print statements on receive during RX throuhput */
#if defined(CONFIG_RAW_RX_BURST_LOGGING)
	LOG_INF("waiting on packet to be received");
#endif
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
#if defined(CONFIG_RAW_RX_BURST_LOGGING)
	LOG_INF("packet received: sizeof of received frame is %d", received);
/* Only enable below code in case packet sanity is to be checked */
#if 0
	for (count = 0; count < received; count++)
	{
		printf("0x%x ", packet->recv_buffer[count]);
		if (((count % 14) == 0) && count != 0)
			printf("\n");
	}
#endif
#endif
	rx_received_bytes_last_sec += received;
	rx_total_received_bytes += received;
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

static void tx_thread_transmit() {
	int count = CONFIG_RAW_TX_TRANSMIT_COUNT;

	LOG_INF("TX burst count is set is %d", CONFIG_RAW_TX_TRANSMIT_COUNT);
	while (count--)
	{
		wifi_send_raw_tx_packets();
		k_sleep(K_MSEC(1));
	}
}

static void initialize(void) {
	static bool initialized = false;


	if (!initialized) {
		initialized = true;
	} else {
		return;
	}
	configurePlayoutCapture(0, 0, 0);
	/* wait for supplicant Init */
	k_sleep(K_MSEC(10));
	/* MONITOR mode */
	//int mode = BIT(1);
	//wifi_set_mode(mode);
	wifi_set_tx_injection_mode();
	wifi_set_channel();
	setup_raw_pkt_socket(&sa);
	k_sleep(K_MSEC(10));

}

static void cleanup(void) {
	close(raw_socket_fd);
}

static int wifi_send_recv_tx_packets_serial(unsigned int num_pkts)
{
	int ret = 0;
	struct raw_tx_pkt_header packet;
	char *test_frame = NULL;
	unsigned int buf_length;

	LOG_INF("serial Transmit and receive function");
	fill_raw_tx_pkt_hdr(&packet, 
			     CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE,
			     CONFIG_RAW_TX_PKT_SAMPLE_RATE_VALUE,
			     CONFIG_RAW_TX_PKT_SAMPLE_RATE_FLAGS,
			     CONFIG_RAW_TX_PKT_SAMPLE_QUEUE_NUM);

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
			    CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE);
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return -1;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header) +
		     CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE;
	memcpy(test_frame, &packet, sizeof(struct raw_tx_pkt_header));
	memcpy(test_frame + sizeof(struct raw_tx_pkt_header), &test_qos_header,
	       sizeof(test_qos_header));
	memset(test_frame + sizeof(struct raw_tx_pkt_header) + sizeof(test_qos_header), 0xAB,
	       CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE);

	for (int i = 0; i < num_pkts; i++) {
		k_sleep(K_MSEC(3));
		LOG_INF("sending packet to lower layer");
		ret = sendto(raw_socket_fd, test_frame, buf_length, 0,
				(struct sockaddr *)&sa, sizeof(sa));
		if (ret < 0) {
			LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
			return -1 ;
		}
		process_single_rx_packet(&test_packet);
	}

	free(test_frame);
	LOG_INF("RAW TX RX success");
	return 0;
}

#ifdef CONFIG_RAW_TX_RX_LOOPBACK
ZTEST(nrf_wifi, test_raw_tx_rx)
{
	int count = CONFIG_RAW_TX_RX_TRANSMIT_COUNT;

	//configurePlayoutCapture(0xCA60, 0x7f, 0x100);
	//k_sleep(K_MSEC(5));

	setup_rawrecv_socket(&dst);
#ifdef CONFIG_RX_LOOPBACK_THREAD
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
#else
	LOG_INF("TX count is set is %d", CONFIG_RAW_TX_RX_TRANSMIT_COUNT);
	zassert_false(wifi_send_recv_tx_packets_serial(count), "Failed to send raw tx packet");
	close(rx_raw_socket_fd);

#endif
}
#endif
#ifdef CONFIG_RAW_TX_BURST
#include <getopt.h>

static int cmd_raw_tx(const struct shell *shell, size_t argc, char **argv)
{
	unsigned int count = CONFIG_RAW_TX_TRANSMIT_COUNT;
	unsigned int payload_size = CONFIG_RAW_TX_PKT_SAMPLE_PAYLOAD_SIZE;
	unsigned int rate_value = CONFIG_RAW_TX_PKT_SAMPLE_RATE_VALUE;
	unsigned int rate_flags = CONFIG_RAW_TX_PKT_SAMPLE_RATE_FLAGS;
	unsigned int queue_num = CONFIG_RAW_TX_PKT_SAMPLE_QUEUE_NUM;
	unsigned int pumping_rate = CONFIG_RAW_TX_PKT_SAMPLE_PUMPING_RATE;
	unsigned int delay_us = 0;

	static struct option long_options[] = {
		{"count",        required_argument, 0, 'c'},
		{"payload-size", required_argument, 0, 's'},
		{"rate-value",   required_argument, 0, 'r'},
		{"rate-flags",   required_argument, 0, 'f'},
		{"queue-num",    required_argument, 0, 'q'},
		{"pumping-rate", required_argument, 0, 'p'},
		{"help",        no_argument,       0, 'h'},
		{0, 0, 0, 0}
	};

	int opt, option_index = 0;
	/* Reset getopt's static state in case of repeated calls */
	optind = 1;

	while ((opt = getopt_long(argc, argv, "c:s:r:f:q:p:h", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'c':
			count = atoi(optarg);
			break;
		case 's':
			payload_size = atoi(optarg);
			break;
		case 'r':
			rate_value = atoi(optarg);
			break;
		case 'f':
			rate_flags = atoi(optarg);
			break;
		case 'q':
			queue_num = atoi(optarg);
			break;
		case 'p':
			pumping_rate = atoi(optarg);
			break;
		case 'h':
		default:
			shell_error(shell, "Usage: raw_tx [--count N | -c N] [--payload-size N | -s N] [--rate-value N | -r N] [--rate-flags N | -f N] [--queue-num N | -q N] [--pumping-rate N | -p N]");
			return -1;
		}
	}

	first_pkt_timestamp = 0;
	last_pkt_timestamp = 0;
	tx_sent_complete = false;
	tx_sent = 0;
	tx_complete = false;

	initialize();

	total_tx_bytes = 0;
	wifi_send_raw_tx_packets_init(payload_size, rate_value, rate_flags, queue_num);

	unsigned int payload_bits = (sizeof(test_qos_header) + payload_size) * 8;
	if (pumping_rate > 0) {
		unsigned int pumping_rate_bps = pumping_rate * 1000000U;
		delay_us = (payload_bits * 1000000U) / pumping_rate_bps;
	}

	shell_print(shell, "Test settings: count: %d, size: %d, rate: %d, flags: %d, queue: %d, delay_us: %u, PPS: %u, pumping_rate: %u Mbps",
		count,
		sizeof(test_qos_header) + payload_size,
		rate_value,
		rate_flags,
		queue_num,
		delay_us,
		delay_us > 0 ? (1000000U / delay_us) : 0,
		pumping_rate);

	while (count--)
	{
		wifi_send_raw_tx_packets_tx();
		if (delay_us > 0) {
			k_usleep(delay_us);
		}
	}

	tx_sent_complete = true;
	shell_print(shell, "Waiting for TX completion: sent: %d", tx_sent);
	wait_for_tx_complete(500);

	nrf_wifi_fmac_get_throughput_bytes(ctx->rpu_ctx, &total_tx_bytes_tmp);
	wifi_send_raw_tx_packets_deinit();

	shell_print(shell, "TX count is %d", total_tx_bytes);

	return 0;
}

SHELL_CMD_ARG_REGISTER(raw_tx, NULL,
	"Send raw TX packets. Usage: raw_tx [--count N | -c N] [--payload-size N | -s N] [--rate-value N | -r N] [--rate-flags N | -f N] [--queue-num N | -q N] [--pumping-rate N | -p N]",
	cmd_raw_tx,
	0, 12);

#endif /* CONFIG_RAW_TX_BURST */


#ifdef CONFIG_RAW_RX_BURST
ZTEST(nrf_wifi, test_raw_rx_single_transmit)
{
	unsigned int prev_time;
	unsigned int curr_time;
	unsigned int throughput_count = 0;
	setup_rawrecv_socket(&dst);
	/* Only setting sizeof(test_beacon_frame) here as the raw header should
	 * is stripped in driver. Adding values for holdoff, flushbytes and burst
	 * as provided by TLM team
	 **/ 

	configurePlayoutCapture(0xCA60, 0x7f, 0x100);
	/**
	 * serialized implementation to avoid threads.
	 * Keep receiving packets till throughput count is reached
	 * calculate throughput in Kb per interval time.
	 * for example - if throughput count is 1000 and
	 * duration is 1 sec, get throughput every 1 sec for 1000 iterations.
	 * This code approximately calculates around 1 sec.
	 * Not creating threads so that TLM does not become slower.
	 **/
	LOG_INF("TX to be sent to lower layer is 1");
	k_sleep(K_MSEC(1));
	zassert_false(wifi_send_raw_tx_packets(), "Failed to send raw tx packet");

	/* get kernel ticks in milliseconds */
	prev_time = k_uptime_get();

	/**
	 * receive packet and calculate throughput for received RX every
	 * througput interval time. The throughput interval is placed
	 * milliseconds and refer the prj.conf for current settings.
	 **/
	while (throughput_count < CONFIG_RAW_RX_THROUGHPUT_COUNT)
	{
		/* Below API waits on recv of packets.
		 * current wait timeout is 10 milliseconds
		 */
		process_single_rx_packet(&test_packet);
		curr_time = k_uptime_get();
#ifdef CONFIG_RAW_RX_BURST_LOGGING
		LOG_INF("curr_uptime is %d, prev_uptime is %d, difference is %d",
				curr_time, prev_time, (curr_time - prev_time));
#endif
		if ((curr_time - prev_time) >= CONFIG_RX_THROUGHPUT_INTERVAL) {
			prev_time = curr_time;
			LOG_INF("Receive throughput in Kb for iteration %d  = %f", throughput_count,
					((((double)rx_received_bytes_last_sec) * 8)/ONE_KB));
			rx_received_bytes_last_sec = 0;
			throughput_count++;
		}
	}
	LOG_INF("Average receive throughput in Kb for number of iterations is %f",
			(((((double)rx_total_received_bytes) * 8)/ONE_KB)/CONFIG_RAW_RX_THROUGHPUT_COUNT));
	close(rx_raw_socket_fd);
}
#endif
#if 1 //ndef CONFIG_RAW_TX_BURST
ZTEST_SUITE(nrf_wifi, NULL, NULL, NULL, NULL, NULL);
#endif
