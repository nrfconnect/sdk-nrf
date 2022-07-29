/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing uitlity function declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_UTIL_H__
#define __ZEPHYR_UTIL_H__
int hex_str_to_val(unsigned char *hex_arr,
		   unsigned int hex_arr_sz,
		   unsigned char *str);
#endif /* __ZEPHYR_UTIL_H__ */
