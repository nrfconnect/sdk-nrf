/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Raw Tx Packet shell
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_tx_pkt, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"

#define BEACON_PAYLOAD_LENGTH 256
#define IEEE80211_SEQ_CTRL_SEQ_NUM_MASK 0xFFF0
#define IEEE80211_SEQ_NUMBER_INC BIT(4) /* 0-3 is fragment number */
#define NRF_WIFI_MAGIC_NUM_RAWTX 0x12345678

/* TODO: Copied from nRF70 Wi-Fi driver, need to be moved to a common place */

/**
 * @brief Transmit modes for raw packets.
 *
 */
enum nrf_wifi_fmac_rawtx_mode {
	/** Legacy mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_LEGACY,
	/** HT mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_HT,
	/** VHT mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_VHT,
	/** HE SU mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_HE_SU,
	/** HE ER SU mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_HE_ER_SU,
	/** HE TB mode. */
	NRF_WIFI_FMAC_RAWTX_MODE_HE_TB,
	/** Throughput max. */
	NRF_WIFI_FMAC_RAWTX_MODE_MAX
};

struct raw_tx_pkt_header {
	unsigned int magic_num;
	unsigned char data_rate;
	unsigned short packet_length;
	unsigned char tx_mode;
	unsigned char queue;
	unsigned char raw_tx_flag;
};

struct raw_tx_pkt_header raw_tx_pkt;

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

void fill_raw_tx_pkt_hdr(int rate_flags, int data_rate, int queue_num)
{
	raw_tx_pkt.magic_num = NRF_WIFI_MAGIC_NUM_RAWTX;
	raw_tx_pkt.data_rate = data_rate;
	raw_tx_pkt.packet_length = sizeof(test_beacon_frame);
	raw_tx_pkt.tx_mode = rate_flags;
	raw_tx_pkt.queue = queue_num;
	raw_tx_pkt.raw_tx_flag = 0;
}

int validate(int value, int min, int max, const char *param)
{
	if (value < min || value > max) {
		LOG_ERR("Please provide %s value between %d and %d", param, min, max);
		return 0;
	}

	return 1;
}

int validate_rate(int data_rate, int flag)
{
	if ((flag == NRF_WIFI_FMAC_RAWTX_MODE_LEGACY) && ((data_rate == 1) ||
							 (data_rate == 2) ||
							 (data_rate == 55) ||
							 (data_rate == 11) ||
							 (data_rate == 6) ||
							 (data_rate == 9) ||
							 (data_rate == 12) ||
							 (data_rate == 18) ||
							 (data_rate == 24) ||
							 (data_rate == 36) ||
							 (data_rate == 48) ||
							 (data_rate == 54))) {

		return 1;
	} else if (((flag >= NRF_WIFI_FMAC_RAWTX_MODE_HT &&
		    flag <= NRF_WIFI_FMAC_RAWTX_MODE_HE_ER_SU)) &&
		    ((data_rate >= 0) && (data_rate <= 7))) {
		return 1;
	}

	LOG_ERR("Invalid data rate %d", data_rate);
	return 0;
}

static int setup_raw_pkt_socket(int *sockfd, struct sockaddr_ll *sa)
{
	struct net_if *iface;
	int ret;

	*sockfd = socket(AF_PACKET, SOCK_RAW, htons(IPPROTO_RAW));
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

static void increment_seq_control(void)
{
	test_beacon_frame.seq_ctrl = (test_beacon_frame.seq_ctrl +
				      IEEE80211_SEQ_NUMBER_INC) &
				      IEEE80211_SEQ_CTRL_SEQ_NUM_MASK;
	if (test_beacon_frame.seq_ctrl > IEEE80211_SEQ_CTRL_SEQ_NUM_MASK) {
		test_beacon_frame.seq_ctrl = 0X0010;
	}
}

static void send_packet(const char *transmission_mode,
			unsigned int num_pkts, unsigned int delay)
{
	struct sockaddr_ll sa;
	int sockfd, ret;
	char *test_frame = NULL;
	int buf_length, num_failures = 0;

	ret = setup_raw_pkt_socket(&sockfd, &sa);
	if (ret < 0) {
		LOG_ERR("Setting socket for raw pkt transmission failed %d", errno);
		return;
	}

	test_frame = malloc(sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame));
	if (!test_frame) {
		LOG_ERR("Malloc failed for send buffer %d", errno);
		return;
	}

	buf_length = sizeof(struct raw_tx_pkt_header) + sizeof(test_beacon_frame);
	memcpy(test_frame, &raw_tx_pkt, sizeof(struct raw_tx_pkt_header));

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

		k_msleep(delay);
	}

	LOG_INF("Sent %d packets with %d failures on socket", num_pkts, num_failures);
	close(sockfd);
	free(test_frame);
}

static int parse_raw_tx_configure_args(const struct shell *sh,
				       size_t argc,
				       char *argv[],
				       int *flags, int *rate, int *queue)
{
	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"rate-flags", required_argument, 0, 'f'},
					       {"data-rate", required_argument, 0, 'd'},
					       {"queue-number", required_argument, 0, 'q'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	int opt_num = 0;

	while ((opt = getopt_long(argc, argv, "f:d:q:h", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'f':
			*flags = atoi(optarg);
			if (!validate(*flags, NRF_WIFI_FMAC_RAWTX_MODE_LEGACY,
				      NRF_WIFI_FMAC_RAWTX_MODE_HE_ER_SU, "Rate Flags")) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case 'd':
			*rate = atoi(optarg);
			if (!validate_rate(*rate, *flags)) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case 'q':
			*queue = atoi(optarg);
			if (!validate(*queue, 0, 4, "Queue Number")) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case'h':
			shell_help(sh);
			opt_num++;
			break;
		case '?':
		default:
			LOG_ERR("Invalid option or option usage: %s",
				argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}

	return opt_num;
}

static int cmd_configure_raw_tx_pkt(
		const struct shell *sh,
		size_t argc,
		char *argv[])
{
	int opt_num, rate_flags = -1, data_rate = -1, queue_num = -1;

	if (argc == 7) {
		opt_num = parse_raw_tx_configure_args(sh, argc, argv, &rate_flags,
						      &data_rate, &queue_num);
		if (opt_num < 0) {
			shell_help(sh);
			return -ENOEXEC;
		} else if (!opt_num) {
			LOG_ERR("No valid option(s) found");
			return -ENOEXEC;
		}
	}

	LOG_INF("Rate-flags: %d Data-rate:%d queue-num:%d",
		rate_flags, data_rate, queue_num);

	fill_raw_tx_pkt_hdr(rate_flags, data_rate, queue_num);

	return 0;
}

static int parse_raw_tx_send_args(const struct shell *sh,
				  size_t argc, char *argv[],
				  char **tx_mode, int *pkt_num, int *time_delay)
{
	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"mode", required_argument, 0, 'm'},
					       {"num-pkts", required_argument, 0, 'n'},
					       {"inter-frame-delay", required_argument, 0, 't'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	int opt_num = 0;

	while ((opt = getopt_long(argc, argv, "m:n:t:h", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'm':
			if (!((strcmp(optarg, "fixed") == 0) ||
			    (strcmp(optarg, "continuous") == 0))) {
				LOG_ERR("Invalid mode %s", optarg);
				return -ENOEXEC;
			}
			*tx_mode = optarg;
			opt_num++;
			break;
		case 't':
			*time_delay = atoi(optarg);
			if (*time_delay < 0) {
				LOG_ERR("Invalid delay %d", atoi(optarg));
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case 'n':
			*pkt_num = (strcmp(*tx_mode, "continuous") == 0) ? INT_MAX : atoi(optarg);
			if (*pkt_num <= 0) {
				LOG_ERR("Invalid num of packets %d", atoi(optarg));
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case'h':
			shell_help(sh);
			opt_num++;
			break;
		case '?':
		default:
			LOG_ERR("Invalid option or option usage: %s", argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}

	if (strcmp(*tx_mode, "continuous") == 0) {
		*pkt_num = INT_MAX;
	} else if ((strcmp(*tx_mode, "fixed") == 0) && (*pkt_num == 0)) {
		LOG_ERR("Please provide number of packets to be transmitted");
		return -ENOEXEC;
	}

	return opt_num;
}

static int cmd_send_raw_tx_pkt(
		const struct shell *sh,
		size_t argc, char *argv[])
{
	int opt_num;
	char *mode = NULL;
	int num_packets = 0, delay = 0;

	if (argc < 5 || argc > 7) {
		shell_help(sh);
		return -ENOEXEC;
	}

	LOG_INF("argument count: %d", argc);
	opt_num = parse_raw_tx_send_args(sh, argc, argv, &mode,
					 &num_packets, &delay);
	if (opt_num < 0) {
		shell_help(sh);
		return -ENOEXEC;
	} else if (!opt_num) {
		LOG_ERR("No valid option(s) found");
		return -ENOEXEC;
	}

	LOG_INF("Selected Mode: %s Num-Packets:%u Delay:%d", mode, num_packets, delay);
	send_packet(mode, num_packets, delay);

	return 0;
}

static int cmd_tx_injection_mode(const struct shell *sh,
				 size_t argc, char *argv[])
{
	struct net_if *iface = NULL;
	bool mode_val = 0;

	if (argc != 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "-h") == 0) {
		shell_help(sh);
		return 0;
	} else if ((strcmp(argv[1], "1") == 0) || (strcmp(argv[1], "0") == 0)) {
		mode_val = atoi(argv[1]);
	} else {
		LOG_ERR("Entered wrong input");
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_eth_txinjection_mode(iface, mode_val)) {
		LOG_ERR("TX Injection mode %s failed", mode_val ? "enabling" : "disabling");
		return -ENOEXEC;
	}

	LOG_INF("TX Injection mode :%s", mode_val ? "enabled" : "disabled");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	raw_tx_cmds,
	SHELL_CMD_ARG(mode, NULL,
		      "This command may be used to enable or disable TX Injection mode\n"
		      "[enable: 1, disable: 0]\n"
		      "[-h, --help] : Print out the help for the mode command\n"
		      "Usage: raw_tx mode 1 or 0\n",
		      cmd_tx_injection_mode,
		      2, 0),
	SHELL_CMD_ARG(configure, NULL,
		      "Configure raw TX packet header\n"
		      "This command may be used to configure raw TX packet header\n"
		      "parameters:\n"
		      "[-f, --rate-flags] : Rate flag value.\n"
		      "[-d, --data-rate] : Data rate value.\n"
		      "[-q, --queue-number] : Queue number.\n"
		      "[-h, --help] : Print out the help for the configure command\n"
		      "Usage: raw_tx configure -f 0 -d 9 -q 1\n",
		      cmd_configure_raw_tx_pkt,
		      7, 0),
	SHELL_CMD_ARG(send, NULL,
		      "Send raw TX packet\n"
		      "This command may be used to send raw TX packets\n"
		      "parameters:\n"
		      "[-m, --mode] : Mode of transmission (either continuous or fixed).\n"
		      "[-n, --number-of-pkts] : Number of packets to be transmitted.\n"
		      "[-t, --inter-frame-delay] : Delay between frames or packets in ms.\n"
		      "[-h, --help] : Print out the help for the send command\n"
		      "Usage:\n"
		      "raw_tx send -m fixed -n 9 -t 10 (For fixed mode)\n"
		      "raw_tx send -m continuous -t 10 (For continuous mode)\n",
		      cmd_send_raw_tx_pkt,
		      5, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(raw_tx,
		   &raw_tx_cmds,
		   "raw_tx_cmds (To configure and send raw TX packets)",
		   NULL);
