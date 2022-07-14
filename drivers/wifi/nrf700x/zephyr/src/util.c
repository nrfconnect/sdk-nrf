/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing utility function definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

int hex_str_to_val(unsigned char *hex_arr, unsigned int hex_arr_sz, unsigned char *str)
{
	int i = 0;
	int j = 0;
	unsigned char ch = 0;
	unsigned char val = 0;
	int len = 0;

	len = strlen(str);

	if (len / 2 > hex_arr_sz) {
		LOG_ERR("%s: String length (%d) greater than array size (%d)\n", __func__, len,
		       hex_arr_sz);
		return -1;
	}

	if (len % 2) {
		LOG_ERR("%s:String length = %d, is not the multiple of 2\n", __func__, len);
		return -1;
	}

	memset(hex_arr, 0, hex_arr_sz);

	for (i = 0; i < len; i++) {
		/* Convert to lower case */
		ch = ((str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i]);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			LOG_ERR("%s: Invalid hex character in string %d\n", __func__, ch);
			return -1;
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

	return j;
}
