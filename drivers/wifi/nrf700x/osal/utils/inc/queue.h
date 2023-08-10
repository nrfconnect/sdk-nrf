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

void *wifi_nrf_utils_q_alloc(struct wifi_nrf_osal_priv *opriv);

void wifi_nrf_utils_q_free(struct wifi_nrf_osal_priv *opriv,
			   void *q);

enum wifi_nrf_status wifi_nrf_utils_q_enqueue(struct wifi_nrf_osal_priv *opriv,
					      void *q,
					      void *q_node);

enum wifi_nrf_status wifi_nrf_utils_q_enqueue_head(struct wifi_nrf_osal_priv *opriv,
						   void *q,
						   void *q_node);

void *wifi_nrf_utils_q_dequeue(struct wifi_nrf_osal_priv *opriv,
			       void *q);

void *wifi_nrf_utils_q_peek(struct wifi_nrf_osal_priv *opriv,
			    void *q);

unsigned int wifi_nrf_utils_q_len(struct wifi_nrf_osal_priv *opriv,
				  void *q);
#endif /* __QUEUE_H__ */
