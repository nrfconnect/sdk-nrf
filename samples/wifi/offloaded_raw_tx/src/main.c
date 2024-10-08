/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Offloaded raw tx sample
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/drivers/wifi/nrfwifi/off_raw_tx/off_raw_tx_api.h>

#ifdef CONFIG_GENERATE_MAC_ADDRESS
#include <zephyr/random/random.h>

#define UNICAST_MASK GENMASK(7, 1)
#define LOCAL_BIT BIT(1)
#endif /* CONFIG_GENERATE_MAC_ADDRESS */

uint8_t bcn_extra_fields[] = {
0x03, 0x01, 0x01, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x07, 0x06, 0x55, 0x53, 0x04, 0x01, 0x0b,
0x1e, 0x23, 0x02, 0x1c, 0x00, 0x2a, 0x01, 0x04, 0x32, 0x04, 0x0c, 0x12, 0x18, 0x60, 0x0b, 0x05,
0x00, 0x00, 0xd4, 0x00, 0x00, 0x46, 0x05, 0x32, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x1a, 0xad, 0x09,
0x17, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x16, 0x01, 0x08, 0x11, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x7f, 0x09, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0xdd, 0x18, 0x00, 0x50, 0xf2,
0x02, 0x01, 0x01, 0x8c, 0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00, 0x00, 0x42, 0x43, 0x5e,
0x00, 0x62, 0x32, 0x2f, 0x00
};


struct bcn_frame {
	uint8_t frame[NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MAX];
	uint16_t len;
} bcn;

unsigned char bssid[6] = {0xf4, 0xce, 0x36, 0x00, 0x10, 0xaa};

unsigned char bcn_supp_rates[] = {0x02, 0x04, 0x0b, 0x16};

/* Example function to build a beacon frame */
static int build_wifi_beacon(unsigned short beacon_interval,
			     unsigned short cap_info,
			     const char *ssid,
			     unsigned char *supp_rates,
			     unsigned int supp_rates_len,
			     unsigned char *extra_fields,
			     unsigned int extra_fields_len)
{
	unsigned int ssid_len = strlen(ssid);
	unsigned int pos = 0;
	unsigned char *bcn_frm = bcn.frame;
	unsigned int len = 0;

	len = sizeof(beacon_interval) + sizeof(cap_info) + ssid_len + extra_fields_len;

	if (len < NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MIN) {
		printf("Beacon frame exceeds maximum size\n");
		return -1;
	}

	if (len > NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MAX) {
		printf("Beacon frame exceeds maximum size\n");
		return 0;
	}

	/* Frame Control */
	bcn_frm[pos++] = 0x80; /* Beacon frame */
	bcn_frm[pos++] = 0x00;

	/* Duration */
	bcn_frm[pos++] = 0x00;
	bcn_frm[pos++] = 0x00;

	/* Destination Address (Broadcast) */
	memset(&bcn_frm[pos], 0xff, 6);
	pos += 6;

	if (nrf70_off_raw_tx_mac_addr_get(bssid)) {
		printf("Failed to get MAC address\n");
		return -1;
	}

	/* Source Address */
	memcpy(&bcn_frm[pos], bssid, 6);
	pos += 6;

	/* BSSID */
	memcpy(&bcn_frm[pos], bssid, 6);
	pos += 6;

	/* Sequence Control */
	bcn_frm[pos++] = 0x00;
	bcn_frm[pos++] = 0x00;

	/* Timestamp */
	memset(&bcn_frm[pos], 0x00, 8);
	pos += 8;

	/* Beacon Interval */
	bcn_frm[pos++] = beacon_interval & 0xff;
	bcn_frm[pos++] = (beacon_interval >> 8) & 0xff;

	/* Capability Information */
	bcn_frm[pos++] = cap_info & 0xff;
	bcn_frm[pos++] = (cap_info >> 8) & 0xff;

	/* SSID Parameter Set */
	bcn_frm[pos++] = 0x00; /* SSID Element ID */
	bcn_frm[pos++] = ssid_len; /* SSID Length */
	memcpy(&bcn_frm[pos], ssid, ssid_len);
	pos += ssid_len;

	/* Supported Rates */
	bcn_frm[pos++] = 0x01; /* Supported Rates Element ID */
	bcn_frm[pos++] = supp_rates_len; /* Supported Rates Length */
	memcpy(&bcn_frm[pos], supp_rates, supp_rates_len);
	pos += supp_rates_len;

	/* Extra fields */
	memcpy(&bcn_frm[pos], extra_fields, extra_fields_len);
	pos += extra_fields_len;

	return pos;
}


int main(void)
{
	struct nrf_wifi_off_raw_tx_conf conf;
	struct nrf_wifi_off_raw_tx_stats stats;
	int len = -1;
	uint8_t *mac_addr = NULL;
	unsigned char *country_code = NULL;
	char rate[RATE_MAX][10] = {"1M", "2M", "5.5M", "11M", "6M", "9M", "12M", "18M",
				   "24M", "36M", "48M", "54M", "MC0", "MC1", "MC2", "MC3",
				   "MC4", "MC5", "MC6", "MC7"};
	char tput_mode[TPUT_MODE_MAX][10] = {"Legacy", "HT", "VHT", "HE SU", "HE ER SU", "HE TB"};

	printf("----- Initializing nRF70 -----\n");

#ifdef CONFIG_GENERATE_MAC_ADDRESS
	mac_addr = malloc(6);
	if (!mac_addr) {
		printf("Failed to allocate memory for mac_addr\n");
		return -ENOMEM;
	}
	sys_rand_get(mac_addr, 6);
	mac_addr[0] = (mac_addr[0] & UNICAST_MASK) | LOCAL_BIT;
#endif /* CONFIG_GENERATE_MAC_ADDRESS */

	country_code = (unsigned char *)malloc(3 * sizeof(unsigned char));
	if (!country_code) {
		printf("Failed to allocate memory for country_code\n");
		return -ENOMEM;
	}

	strncpy((char *)country_code, "US", NRF_WIFI_COUNTRY_CODE_LEN + 1);
	nrf70_off_raw_tx_init(mac_addr, country_code);

	/* Build a beacon frame */
	len = build_wifi_beacon(CONFIG_BEACON_INTERVAL,
				0x431,
				"nRF70_off_raw_tx_1",
				bcn_supp_rates,
				sizeof(bcn_supp_rates),
				bcn_extra_fields,
				sizeof(bcn_extra_fields));

	memset(&conf, 0, sizeof(conf));
	conf.pkt = bcn.frame;
	conf.pkt_len = len;
	conf.period_us = CONFIG_BEACON_INTERVAL * USEC_PER_MSEC;
	conf.tx_pwr = 15;
	conf.chan = 1;
	conf.short_preamble = false;
	conf.num_retries = 10;
	conf.tput_mode = TPUT_MODE_LEGACY;
	conf.rate = RATE_54M;
	conf.he_gi = 1;
	conf.he_ltf = 1;

	printf("----- Starting to transmit beacons with the following configuration -----\n");
	printf("\tSSID: nRF70_off_raw_tx_1\n");
	printf("\tPeriod: %d\n", conf.period_us);
	printf("\tTX Power: %d\n", conf.tx_pwr);
	printf("\tChannel: %d\n", conf.chan);
	printf("\tShort Preamble: %d\n", conf.short_preamble);
	printf("\tNumber of Retries: %d\n", conf.num_retries);
	printf("\tThroughput Mode: %s\n", tput_mode[conf.tput_mode]);
	printf("\tRate: %s\n", rate[conf.rate]);
	printf("\tHE GI: %d\n", conf.he_gi);
	printf("\tHE LTF: %d\n", conf.he_ltf);
	nrf70_off_raw_tx_start(&conf);

	k_sleep(K_SECONDS(30));

	memset(&stats, 0, sizeof(stats));
	nrf70_off_raw_tx_stats(&stats);
	printf("-----  Statistics -----\n");
	printf("\tPacket sent: %u\n", stats.off_raw_tx_pkt_sent);

	/* Build a beacon frame */
	len = build_wifi_beacon(CONFIG_BEACON_INTERVAL,
				0x431,
				"nRF70_off_raw_tx_2",
				bcn_supp_rates,
				sizeof(bcn_supp_rates),
				bcn_extra_fields,
				sizeof(bcn_extra_fields));
	conf.pkt = bcn.frame;
	conf.pkt_len = len;
	conf.tx_pwr = 11;
	conf.chan = 36;
	conf.short_preamble = false;
	conf.num_retries = 10;
	conf.rate = RATE_12M;

	printf("----- Updating configuration to -----\n");
	printf("\tSSID: nRF70_off_raw_tx_2\n");
	printf("\tPeriod: %d\n", conf.period_us);
	printf("\tTX Power: %d\n", conf.tx_pwr);
	printf("\tChannel: %d\n", conf.chan);
	printf("\tShort Preamble: %d\n", conf.short_preamble);
	printf("\tNumber of Retries: %d\n", conf.num_retries);
	printf("\tThroughput Mode: %s\n", tput_mode[conf.tput_mode]);
	printf("\tRate: %s\n", rate[conf.rate]);
	printf("\tHE GI: %d\n", conf.he_gi);
	printf("\tHE LTF: %d\n", conf.he_ltf);
	nrf70_off_raw_tx_conf_update(&conf);

	k_sleep(K_SECONDS(30));

	nrf70_off_raw_tx_stats(&stats);
	printf("-----  Statistics -----\n");
	printf("\tPacket sent: %u\n", stats.off_raw_tx_pkt_sent);

	printf("----- Stopping transmission -----\n");
	nrf70_off_raw_tx_stop();

	printf("----- Deinitializing nRF70 -----\n");
	nrf70_off_raw_tx_deinit();

	if (mac_addr) {
		free(mac_addr);
	}

	return 0;
}
