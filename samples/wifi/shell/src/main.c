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

#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_config.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

#define STACK_SIZE 1024
#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif
#define RECV_BUFFER_SIZE 1280

static struct k_sem quit_lock;

static void recv_packet(void);

static struct packet_data sock_packet;
static bool finish;

struct packet_data {
        int send_sock;
        int recv_sock;
        char recv_buffer[RECV_BUFFER_SIZE];
};

K_THREAD_DEFINE(receiver_thread_id, STACK_SIZE,
                recv_packet, NULL, NULL, NULL,
                THREAD_PRIORITY, 0, -1);

struct nrf_wifi_fmac_rawpkt_info {
	/* Magic number to distinguish packet is raw packet */
	unsigned int magic_number;
	/* Data rate of the packet */
	unsigned char data_rate;
	/* Packet length */
	unsigned short packet_length;
	/* Mode describing if packet is VHT, HT, HE or Legacy */
	unsigned char tx_mode;
	/* Wi-Fi access category mapping for packet */
	unsigned char queue;
	/* reserved parameter for driver */
	unsigned char reserved;
};

/* SSID: NRF_RAW_TX_PACKET_APP
 * Transmitter Address: a0:69:60:e3:52:15
 */
struct beacon {
	uint16_t frame_control;
	uint16_t duration;
	uint8_t da[6];
	uint8_t sa[6];
	uint8_t bssid[6];
	uint16_t seq_ctrl;
	uint8_t payload[256];
};

static const struct beacon test_beacon_frame = {
	.frame_control = htons(0x8000),
	.duration = 0x0000,
	.da = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	.sa = {0xa0, 0x69, 0x60, 0xe3, 0x52, 0x15},
	.bssid = {0xa0, 0x69, 0x60, 0xe3, 0x52, 0x15},
	.seq_ctrl = 0xb0a3,
	.payload = {
		0x0c, 0xa2, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00,
		0x11, 0x04, 0x00, 0x15, 0x4E, 0x52, 0x46, 0x5f, 0x52, 0x41,
		0x57, 0x5f, 0x54, 0x58, 0x5f, 0x50, 0x41, 0x43, 0x4b, 0x45,
		0x54, 0x5f, 0x41, 0x50, 0x50, 0x01, 0x08, 0x82, 0x84, 0x8b,
		0x96, 0x0c, 0x12, 0x18, 0x24, 0x03, 0x01, 0x06, 0x05, 0x04,
		0x00, 0x02, 0x00, 0x00, 0x2a, 0x01, 0x04, 0x32, 0x04, 0x30,
		0x48, 0x60, 0x6c, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac,
		0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00,
		0x0f, 0xac, 0x02, 0x0c, 0x00, 0x3b, 0x02, 0x51, 0x00, 0x2d,
		0x1a, 0x0c, 0x00, 0x17, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x2c, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x16, 0x06, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x08, 0x04, 0x00,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0xff, 0x1a, 0x23, 0x01,
		0x78, 0x10, 0x1a, 0x00, 0x00, 0x00, 0x20, 0x0e, 0x09, 0x00,
		0x09, 0x80, 0x04, 0x01, 0xc4, 0x00, 0xfa, 0xff, 0xfa, 0xff,
		0x61, 0x1c, 0xc7, 0x71, 0xff, 0x07, 0x24, 0xf0, 0x3f, 0x00,
		0x81, 0xfc, 0xff, 0xdd, 0x18, 0x00, 0x50, 0xf2, 0x02, 0x01,
		0x01, 0x01, 0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00,
		0x00, 0x42, 0x43, 0x5e, 0x00, 0x62, 0x32, 0x2f, 0x00
	}
};

static void quit(void)
{
        k_sem_give(&quit_lock);
}

static int start_socket(int *sock)
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
	
static int recv_packet_socket(struct packet_data *packet)
{
        int ret = 0;
        int received;
//	static int count = 0;
        LOG_ERR("Waiting for packets ...");

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

                        LOG_ERR("RAW : recv error %d", errno);
                        ret = -errno;
                        break;
                }
                LOG_ERR("code working received %d bytes", received);
#if 1 
                LOG_ERR("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", packet->recv_buffer[0],
			packet->recv_buffer[1], packet->recv_buffer[2], packet->recv_buffer[3], packet->recv_buffer[4],
	       		packet->recv_buffer[5], packet->recv_buffer[6], packet->recv_buffer[7], packet->recv_buffer[8],
			packet->recv_buffer[9], packet->recv_buffer[10], packet->recv_buffer[11], packet->recv_buffer[12],
			packet->recv_buffer[13], packet->recv_buffer[14], packet->recv_buffer[15], packet->recv_buffer[16],
			packet->recv_buffer[17], packet->recv_buffer[18], packet->recv_buffer[19], packet->recv_buffer[20],
			packet->recv_buffer[21]);
#endif			
        } while (true);

        return ret;
}

static void recv_packet(void)
{
        int ret;
        struct timeval timeo_optval = {
                .tv_sec = 1,
                .tv_usec = 0,
        };

        ret = start_socket(&sock_packet.recv_sock);
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
                ret = recv_packet_socket(&sock_packet);
                if (ret < 0) {
                        quit();
                        return;
                }
        }
}

static void wifi_set_channel(void)
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = {0};
	int ret;

	channel_info.oper = WIFI_MGMT_SET;

	if (channel_info.if_index == 0) {
		iface = net_if_get_first_wifi();
		if (iface == NULL) {
			LOG_ERR("Cannot find the default wifi interface");
			return;
		}
		channel_info.if_index = net_if_get_by_iface(iface);
	} else {
		iface = net_if_get_by_index(channel_info.if_index);
		if (iface == NULL) {
			LOG_ERR("Cannot find interface for if_index %d",
					      channel_info.if_index);
			return;
		}
	}

	if (channel_info.oper == WIFI_MGMT_SET) {
		channel_info.channel = 5;
		if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
			   (channel_info.channel > WIFI_CHANNEL_MAX)) {
			LOG_ERR("Invalid channel number. Range is (1-233)");
			return;
		}
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
	struct ethernet_req_params params = {0};

	mode_info.oper = WIFI_MGMT_SET;

	if (mode_info.if_index == 0) {
		iface = net_if_get_default();
		if (iface == NULL) {
			LOG_ERR("Cannot find the default wifi interface");
			return;
		}
		mode_info.if_index = net_if_get_by_iface(iface);
	} else {
		iface = net_if_get_by_index(mode_info.if_index);
		if (iface == NULL) {
			LOG_ERR("Cannot find interface for if_index %d",
				      mode_info.if_index);
			return;
		}
	}

	mode_info.mode = WIFI_STA_MODE;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed %d", ret);
	}

	printk("net eth txinjection mode being called");
        ret = net_eth_txinjection_mode(iface, true);

        if (ret != 0) {
                printk("iface %p txinjection mode failed: %d", iface, ret);
        }

        ret = net_mgmt(NET_REQUEST_ETHERNET_GET_TXINJECTION_MODE, iface,
                        &params, sizeof(struct ethernet_req_params));
        if (ret != 0) {
                printk("iface %p txinjection get mode failed: %d", iface, params.txinjection_mode);
        }
}

static int setup_raw_pkt_socket(int *sockfd, struct sockaddr_ll *sa)
{
	struct net_if *iface;
	int ret;

	*sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if (*sockfd < 0) {
		LOG_ERR("Unable to create a socket %d", errno);
		return -1;
	}

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Unable to get default interface");
		return -1;
	}

	sa->sll_family = AF_PACKET;
	sa->sll_ifindex = net_if_get_by_iface(iface);

	/* Bind the socket */
	ret = bind(*sockfd, (struct sockaddr *)sa, sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Error: Unable to bind socket to the network interface:%d", errno);
		close(*sockfd);
		return -1;
	}

	return 0;
}

static void fill_raw_tx_pkt_hdr(struct nrf_wifi_fmac_rawpkt_info *raw_tx_pkt)
{
	/* Raw Tx Packet header */
	raw_tx_pkt->magic_number = 0x12345678;
	raw_tx_pkt->data_rate = 9;
	raw_tx_pkt->packet_length = sizeof(test_beacon_frame);
	raw_tx_pkt->tx_mode = 1;
	raw_tx_pkt->queue = 2;
	raw_tx_pkt->reserved = 0;
}

static void wifi_send_raw_tx_packet(void)
{
	struct sockaddr_ll sa;
	int sockfd, ret;
	struct nrf_wifi_fmac_rawpkt_info packet;
	char *test_frame = NULL;
	int buf_length;

	ret = setup_raw_pkt_socket(&sockfd, &sa);
	if (ret < 0) {
		LOG_ERR("Setting socket for raw pkt transmission failed %d", errno);
		return;
	}

	fill_raw_tx_pkt_hdr(&packet);

	if (ret < 0) {
		close(sockfd);
		return;
	}

	test_frame = malloc(sizeof(struct nrf_wifi_fmac_rawpkt_info) + sizeof(test_beacon_frame));
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return;
	}

	buf_length = sizeof(struct nrf_wifi_fmac_rawpkt_info) + sizeof(test_beacon_frame);
	memcpy(test_frame, &packet, sizeof(struct nrf_wifi_fmac_rawpkt_info));
	memcpy(test_frame + sizeof(struct nrf_wifi_fmac_rawpkt_info),
				&test_beacon_frame, sizeof(test_beacon_frame));

	ret = sendto(sockfd, test_frame, buf_length, 0,
		(struct sockaddr *)&sa, sizeof(sa));

	if (ret < 0) {
		LOG_ERR("Unable to send beacon frame: %s", strerror(errno));
		close(sockfd);
		free(test_frame);
		return;
	}

	k_msleep(100);

	/* close the socket */
	close(sockfd);
	free(test_frame);
}

int main(void)
{
	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	k_thread_start(receiver_thread_id);

	wifi_set_mode();
	wifi_set_channel();
	k_msleep(100);
	wifi_send_raw_tx_packet();

	k_sem_take(&quit_lock, K_FOREVER);

	LOG_ERR("stopping");

	finish = true;

	k_thread_join(receiver_thread_id, K_FOREVER);

	if (sock_packet.recv_sock >= 0) {
		(void)close(sock_packet.recv_sock);
	}

	return 0;
}
