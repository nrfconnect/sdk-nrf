/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing utility function declarations for the
 * Wi-Fi driver.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stddef.h>
#include "osal_api.h"

int nrf_wifi_utils_hex_str_to_val(struct wifi_nrf_osal_priv *opriv,
				  unsigned char *hex_arr,
				  unsigned int hex_arr_sz,
				  unsigned char *str);
#endif /* __UTIL_H__ */
