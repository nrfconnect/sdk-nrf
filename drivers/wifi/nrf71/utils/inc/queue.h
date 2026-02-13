/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing queue specific declarations
 * for the Wi-Fi driver.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "osal_ops.h"

void *nrf_wifi_utils_q_alloc(void);

void nrf_wifi_utils_q_free(void *q);

void *nrf_wifi_utils_ctrl_q_alloc(void);

void nrf_wifi_utils_ctrl_q_free(void *q);

enum nrf_wifi_status nrf_wifi_utils_q_enqueue(void *q,
					      void *q_node);

enum nrf_wifi_status nrf_wifi_utils_ctrl_q_enqueue(void *q,
					       void *q_node);

enum nrf_wifi_status nrf_wifi_utils_q_enqueue_head(void *q,
						   void *q_node);

void *nrf_wifi_utils_q_dequeue(void *q);

void *nrf_wifi_utils_ctrl_q_dequeue(void *q);

void *nrf_wifi_utils_q_peek(void *q);

unsigned int nrf_wifi_utils_q_len(void *q);
#endif /* __QUEUE_H__ */
