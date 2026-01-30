/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing utility function declarations for the
 * Wi-Fi driver.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include "osal_api.h"
#include "nrf71_wifi_ctrl.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

/* Convert power from mBm to dBm */
#define MBM_TO_DBM(gain) ((gain) / 100)

int nrf_wifi_utils_hex_str_to_val(unsigned char *hex_arr,
				  unsigned int hex_arr_sz,
				  unsigned char *str);

bool nrf_wifi_utils_is_mac_addr_valid(const char *mac_addr);

int nrf_wifi_utils_chan_to_freq(enum nrf_wifi_band band,
				unsigned short chan);
#endif /* __UTIL_H__ */
