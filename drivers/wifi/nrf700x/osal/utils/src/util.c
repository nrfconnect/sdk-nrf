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

	len = strlen(str);

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
