/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing queue specific definitions
 * for the Wi-Fi driver.
 */

#include "list.h"
#include "queue.h"

void *nrf_wifi_utils_q_alloc(void)
{
	return nrf_wifi_utils_list_alloc();
}

void *nrf_wifi_utils_ctrl_q_alloc(void)
{
	return nrf_wifi_utils_ctrl_list_alloc();
}

void nrf_wifi_utils_q_free(void *q)
{
	nrf_wifi_utils_list_free(q);
}

void nrf_wifi_utils_ctrl_q_free(void *q)
{
	nrf_wifi_utils_ctrl_list_free(q);
}

enum nrf_wifi_status nrf_wifi_utils_q_enqueue(void *q,
					      void *data)
{
	return nrf_wifi_utils_list_add_tail(q,
					    data);
}

enum nrf_wifi_status nrf_wifi_utils_ctrl_q_enqueue(void *q,
						   void *data)
{
	return nrf_wifi_utils_ctrl_list_add_tail(q, data);
}

enum nrf_wifi_status nrf_wifi_utils_q_enqueue_head(void *q,
						   void *data)
{
	return nrf_wifi_utils_list_add_head(q,
					    data);
}


void *nrf_wifi_utils_q_dequeue(void *q)
{
	return nrf_wifi_utils_list_del_head(q);
}


void *nrf_wifi_utils_ctrl_q_dequeue(void *q)
{
	return nrf_wifi_utils_ctrl_list_del_head(q);
}


void *nrf_wifi_utils_q_peek(void *q)
{
	return nrf_wifi_utils_list_peek(q);
}


unsigned int nrf_wifi_utils_q_len(void *q)
{
	return nrf_wifi_utils_list_len(q);
}
