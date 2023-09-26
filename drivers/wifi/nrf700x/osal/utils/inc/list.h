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

void *nrf_wifi_utils_list_alloc(struct nrf_wifi_osal_priv *opriv);

void nrf_wifi_utils_list_free(struct nrf_wifi_osal_priv *opriv,
			      void *list);

enum nrf_wifi_status nrf_wifi_utils_list_add_tail(struct nrf_wifi_osal_priv *opriv,
						  void *list,
						  void *data);

enum nrf_wifi_status nrf_wifi_utils_list_add_head(struct nrf_wifi_osal_priv *opriv,
						  void *list,
						  void *data);

void nrf_wifi_utils_list_del_node(struct nrf_wifi_osal_priv *opriv,
				  void *list,
				  void *data);

void *nrf_wifi_utils_list_del_head(struct nrf_wifi_osal_priv *opriv,
				   void *list);

void *nrf_wifi_utils_list_peek(struct nrf_wifi_osal_priv *opriv,
			       void *list);

unsigned int nrf_wifi_utils_list_len(struct nrf_wifi_osal_priv *opriv,
				     void *list);

enum nrf_wifi_status
nrf_wifi_utils_list_traverse(struct nrf_wifi_osal_priv *opriv,
			     void *list,
			     void *callbk_data,
			     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
								 void *data));
#endif /* __LIST_H__ */
