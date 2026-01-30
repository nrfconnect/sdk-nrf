/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing linked list specific declarations
 * for the Wi-Fi driver.
 */

#ifndef __LIST_H__
#define __LIST_H__

#include "osal_api.h"

void *nrf_wifi_utils_list_alloc(void);

void nrf_wifi_utils_list_free(void *list);

void *nrf_wifi_utils_ctrl_list_alloc(void);

void nrf_wifi_utils_ctrl_list_free(void *list);

enum nrf_wifi_status nrf_wifi_utils_list_add_tail(void *list,
						  void *data);

enum nrf_wifi_status nrf_wifi_utils_ctrl_list_add_tail(void *list,
						  void *data);

enum nrf_wifi_status nrf_wifi_utils_list_add_head(void *list,
						  void *data);

void nrf_wifi_utils_list_del_node(void *list,
				  void *data);

void *nrf_wifi_utils_list_del_head(void *list);

void *nrf_wifi_utils_ctrl_list_del_head(void *list);

void *nrf_wifi_utils_list_peek(void *list);

unsigned int nrf_wifi_utils_list_len(void *list);

enum nrf_wifi_status
nrf_wifi_utils_list_traverse(void *list,
			     void *callbk_data,
			     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
								 void *data));
#endif /* __LIST_H__ */
