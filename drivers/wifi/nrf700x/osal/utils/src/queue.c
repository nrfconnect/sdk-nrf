/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing queue specific definitions
 * for the Wi-Fi driver.
 */

#include "list.h"
#include "queue.h"

void *wifi_nrf_utils_q_alloc(struct wifi_nrf_osal_priv *opriv)
{
	return wifi_nrf_utils_list_alloc(opriv);
}


void wifi_nrf_utils_q_free(struct wifi_nrf_osal_priv *opriv,
			   void *q)
{
	wifi_nrf_utils_list_free(opriv,
				 q);
}


enum wifi_nrf_status wifi_nrf_utils_q_enqueue(struct wifi_nrf_osal_priv *opriv,
					      void *q,
					      void *data)
{
	return wifi_nrf_utils_list_add_tail(opriv,
					    q,
					    data);
}

enum wifi_nrf_status wifi_nrf_utils_q_enqueue_head(struct wifi_nrf_osal_priv *opriv,
						   void *q,
						   void *data)
{
	return wifi_nrf_utils_list_add_head(opriv,
					    q,
					    data);
}


void *wifi_nrf_utils_q_dequeue(struct wifi_nrf_osal_priv *opriv,
			       void *q)
{
	return wifi_nrf_utils_list_del_head(opriv,
					    q);
}


void *wifi_nrf_utils_q_peek(struct wifi_nrf_osal_priv *opriv,
			    void *q)
{
	return wifi_nrf_utils_list_peek(opriv,
					q);
}


unsigned int wifi_nrf_utils_q_len(struct wifi_nrf_osal_priv *opriv,
				  void *q)
{
	return wifi_nrf_utils_list_len(opriv,
				       q);
}
