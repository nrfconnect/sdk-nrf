/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing linked list specific declarations
 * for the Wi-Fi driver.
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>
#include "osal_api.h"

void *wifi_nrf_utils_list_alloc(struct wifi_nrf_osal_priv *opriv);

void wifi_nrf_utils_list_free(struct wifi_nrf_osal_priv *opriv,
			      void *list);

enum wifi_nrf_status wifi_nrf_utils_list_add_tail(struct wifi_nrf_osal_priv *opriv,
						  void *list,
						  void *data);

enum wifi_nrf_status wifi_nrf_utils_list_add_head(struct wifi_nrf_osal_priv *opriv,
						  void *list,
						  void *data);

void wifi_nrf_utils_list_del_node(struct wifi_nrf_osal_priv *opriv,
				  void *list,
				  void *data);

void *wifi_nrf_utils_list_del_head(struct wifi_nrf_osal_priv *opriv,
				   void *list);

void *wifi_nrf_utils_list_peek(struct wifi_nrf_osal_priv *opriv,
			       void *list);

unsigned int wifi_nrf_utils_list_len(struct wifi_nrf_osal_priv *opriv,
				     void *list);

enum wifi_nrf_status
wifi_nrf_utils_list_traverse(struct wifi_nrf_osal_priv *opriv,
			     void *list,
			     void *callbk_data,
			     enum wifi_nrf_status (*callbk_func)(void *callbk_data,
								 void *data));
#endif /* __LIST_H__ */
