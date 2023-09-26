/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing queue specific declarations
 * for the Wi-Fi driver.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include "osal_ops.h"

void *nrf_wifi_utils_q_alloc(struct nrf_wifi_osal_priv *opriv);

void nrf_wifi_utils_q_free(struct nrf_wifi_osal_priv *opriv,
			   void *q);

enum nrf_wifi_status nrf_wifi_utils_q_enqueue(struct nrf_wifi_osal_priv *opriv,
					      void *q,
					      void *q_node);

enum nrf_wifi_status nrf_wifi_utils_q_enqueue_head(struct nrf_wifi_osal_priv *opriv,
						   void *q,
						   void *q_node);

void *nrf_wifi_utils_q_dequeue(struct nrf_wifi_osal_priv *opriv,
			       void *q);

void *nrf_wifi_utils_q_peek(struct nrf_wifi_osal_priv *opriv,
			    void *q);

unsigned int nrf_wifi_utils_q_len(struct nrf_wifi_osal_priv *opriv,
				  void *q);
#endif /* __QUEUE_H__ */
