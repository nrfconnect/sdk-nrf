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

void *nrf_wifi_utils_q_alloc(struct nrf_wifi_osal_priv *opriv)
{
	return nrf_wifi_utils_list_alloc(opriv);
}


void nrf_wifi_utils_q_free(struct nrf_wifi_osal_priv *opriv,
			   void *q)
{
	nrf_wifi_utils_list_free(opriv,
				 q);
}


enum nrf_wifi_status nrf_wifi_utils_q_enqueue(struct nrf_wifi_osal_priv *opriv,
					      void *q,
					      void *data)
{
	return nrf_wifi_utils_list_add_tail(opriv,
					    q,
					    data);
}

enum nrf_wifi_status nrf_wifi_utils_q_enqueue_head(struct nrf_wifi_osal_priv *opriv,
						   void *q,
						   void *data)
{
	return nrf_wifi_utils_list_add_head(opriv,
					    q,
					    data);
}


void *nrf_wifi_utils_q_dequeue(struct nrf_wifi_osal_priv *opriv,
			       void *q)
{
	return nrf_wifi_utils_list_del_head(opriv,
					    q);
}


void *nrf_wifi_utils_q_peek(struct nrf_wifi_osal_priv *opriv,
			    void *q)
{
	return nrf_wifi_utils_list_peek(opriv,
					q);
}


unsigned int nrf_wifi_utils_q_len(struct nrf_wifi_osal_priv *opriv,
				  void *q)
{
	return nrf_wifi_utils_list_len(opriv,
				       q);
}
