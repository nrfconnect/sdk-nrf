/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing utility function definitions for the
 * Wi-Fi driver.
 */

#include <string.h>
#include <util.h>
#include "host_rpu_data_if.h"


int nrf_wifi_utils_hex_str_to_val(struct wifi_nrf_osal_priv *opriv,
				  unsigned char *hex_arr,
				  unsigned int hex_arr_sz,
				  unsigned char *str)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned char ch = 0;
	unsigned char val = 0;
	unsigned int len = 0;
	int ret = -1;

	len = wifi_nrf_osal_strlen(opriv, str);

	if (len / 2 > hex_arr_sz) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: String length (%d) greater than array size (%d)\n",
				      __func__,
				      len,
				      hex_arr_sz);
		goto out;
	}

	if (len % 2) {
		wifi_nrf_osal_log_err(opriv,
				      "%s:String length = %d, is not a multiple of 2\n",
				      __func__,
				      len);
		goto out;
	}

	for (i = 0; i < len; i++) {
		/* Convert each character to lower case */
		ch = ((str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i]);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			wifi_nrf_osal_log_err(opriv,
					      "%s: Invalid hex character in string %d\n",
					      __func__,
					      ch);
			goto out;
		}

		if (ch >= '0' && ch <= '9') {
			ch = ch - '0';
		} else {
			ch = ch - 'a' + 10;
		}

		val += ch;

		if (!(i % 2)) {
			val <<= 4;
		} else {
			hex_arr[j] = val;
			j++;
			val = 0;
		}
	}

	ret = j;
out:
	return ret;
}


bool nrf_wifi_utils_is_mac_addr_valid(const char *mac_addr)
{
	unsigned char zero_addr[NRF_WIFI_ETH_ADDR_LEN] = {0};

	return (mac_addr && (memcmp(mac_addr,
			zero_addr,
			sizeof(zero_addr)) != 0) &&
		!(mac_addr[0] & 0x1));
}


int nrf_wifi_utils_chan_to_freq(struct wifi_nrf_osal_priv *opriv,
				enum nrf_wifi_band band,
				unsigned short chan)
{
	int freq = -1;
	unsigned short valid_5g_chans[] = {32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 96, 100, 104,
		108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 159, 161, 163,
		165, 167, 169, 171, 173, 175, 177};
	unsigned char i = 0;

	switch (band) {
	case NRF_WIFI_BAND_2GHZ:
		if ((chan >= 1) && (chan <= 13)) {
			freq = (((chan - 1) * 5) + 2412);
		} else if (chan == 14) {
			freq = 2484;
		} else {
			wifi_nrf_osal_log_err(opriv,
					      "%s: Invalid channel value %d\n",
					      __func__,
					      chan);
			goto out;
		}
		break;
	case NRF_WIFI_BAND_5GHZ:
		for (i = 0; i < ARRAY_SIZE(valid_5g_chans); i++) {
			if (chan == valid_5g_chans[i]) {
				freq = (chan * 5) + 5000;
				break;
			}
		}

		break;
	default:
		wifi_nrf_osal_log_err(opriv,
				      "%s: Invalid band value %d\n",
				      __func__,
				      band);
		goto out;
	}
out:
	return freq;

}
