/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing linked list specific definitions
 * for the Wi-Fi driver.
 */

#include "list.h"

void *nrf_wifi_utils_list_alloc(void)
{
	void *list = NULL;

	list = nrf_wifi_osal_llist_alloc();

	if (!list) {
		nrf_wifi_osal_log_err("%s: Unable to allocate list",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_llist_init(list);

out:
	return list;

}

void *nrf_wifi_utils_ctrl_list_alloc(void)
{
	void *list = NULL;

	list = nrf_wifi_osal_ctrl_llist_alloc();

	if (!list) {
		nrf_wifi_osal_log_err("%s: Unable to allocate list",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_llist_init(list);

out:
	return list;

}

void nrf_wifi_utils_list_free(void *list)
{
	nrf_wifi_osal_llist_free(list);
}

void nrf_wifi_utils_ctrl_list_free(void *list)
{
	nrf_wifi_osal_ctrl_llist_free(list);
}

enum nrf_wifi_status nrf_wifi_utils_list_add_tail(void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = nrf_wifi_osal_llist_node_alloc();

	if (!list_node) {
		nrf_wifi_osal_log_err("%s: Unable to allocate list node",
				      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_llist_node_data_set(list_node,
					  data);

	nrf_wifi_osal_llist_add_node_tail(list,
					  list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_utils_ctrl_list_add_tail(void *list,
	void *data)
{
	void *list_node = NULL;

	list_node = nrf_wifi_osal_ctrl_llist_node_alloc();

	if (!list_node) {
		nrf_wifi_osal_log_err("%s: Unable to allocate list node",
			      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_llist_node_data_set(list_node, data);

	nrf_wifi_osal_llist_add_node_tail(list, list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_utils_list_add_head(void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = nrf_wifi_osal_llist_node_alloc();

	if (!list_node) {
		nrf_wifi_osal_log_err("%s: Unable to allocate list node",
				      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_llist_node_data_set(list_node,
					  data);

	nrf_wifi_osal_llist_add_node_head(list,
					  list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_utils_list_del_node(void *list,
				  void *data)
{
	void *stored_data;
	void *list_node = NULL;
	void *list_node_next = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(list);

	while (list_node) {
		stored_data = nrf_wifi_osal_llist_node_data_get(list_node);

		list_node_next = nrf_wifi_osal_llist_get_node_nxt(list,
								  list_node);

		if (stored_data == data) {
			nrf_wifi_osal_llist_del_node(list,
						     list_node);

			nrf_wifi_osal_llist_node_free(list_node);
		}

		list_node = list_node_next;
	}
}

void *nrf_wifi_utils_list_del_head(void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(list);

	if (!list_node) {
		goto out;
	}

	data = nrf_wifi_osal_llist_node_data_get(list_node);

	nrf_wifi_osal_llist_del_node(list,
				     list_node);
	nrf_wifi_osal_llist_node_free(list_node);

out:
	return data;
}

void *nrf_wifi_utils_ctrl_list_del_head(void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(list);
	if (!list_node) {
		goto out;
	}

	data = nrf_wifi_osal_llist_node_data_get(list_node);

	nrf_wifi_osal_llist_del_node(list,
				     list_node);
	nrf_wifi_osal_ctrl_llist_node_free(list_node);

out:
	return data;
}


void *nrf_wifi_utils_list_peek(void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(list);

	if (!list_node) {
		goto out;
	}

	data = nrf_wifi_osal_llist_node_data_get(list_node);

out:
	return data;
}


unsigned int nrf_wifi_utils_list_len(void *list)
{
	return nrf_wifi_osal_llist_len(list);
}


enum nrf_wifi_status
nrf_wifi_utils_list_traverse(void *list,
			     void *callbk_data,
			     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
								 void *data))
{
	void *list_node = NULL;
	void *data = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	list_node = nrf_wifi_osal_llist_get_node_head(list);

	while (list_node) {
		data = nrf_wifi_osal_llist_node_data_get(list_node);

		status = callbk_func(callbk_data,
				     data);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			goto out;
		}

		list_node = nrf_wifi_osal_llist_get_node_nxt(list,
							     list_node);
	}
out:
	return status;
}
