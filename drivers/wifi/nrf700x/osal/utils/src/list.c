/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing linked list specific definitions
 * for the Wi-Fi driver.
 */


#include "list.h"

void *wifi_nrf_utils_list_alloc(struct wifi_nrf_osal_priv *opriv)
{
	void *list = NULL;

	list = wifi_nrf_osal_llist_alloc(opriv);

	if (!list) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate list\n",
				      __func__);
		goto out;
	}

	wifi_nrf_osal_llist_init(opriv,
				 list);

out:
	return list;

}


void wifi_nrf_utils_list_free(struct wifi_nrf_osal_priv *opriv,
			      void *list)
{
	wifi_nrf_osal_llist_free(opriv,
				 list);
}


enum wifi_nrf_status wifi_nrf_utils_list_add_tail(struct wifi_nrf_osal_priv *opriv,
						  void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = wifi_nrf_osal_llist_node_alloc(opriv);

	if (!list_node) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate list node\n",
				      __func__);
		return WIFI_NRF_STATUS_FAIL;
	}

	wifi_nrf_osal_llist_node_data_set(opriv,
					  list_node,
					  data);

	wifi_nrf_osal_llist_add_node_tail(opriv,
					  list,
					  list_node);

	return WIFI_NRF_STATUS_SUCCESS;
}

enum wifi_nrf_status wifi_nrf_utils_list_add_head(struct wifi_nrf_osal_priv *opriv,
						  void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = wifi_nrf_osal_llist_node_alloc(opriv);

	if (!list_node) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate list node\n",
				      __func__);
		return WIFI_NRF_STATUS_FAIL;
	}

	wifi_nrf_osal_llist_node_data_set(opriv,
					  list_node,
					  data);

	wifi_nrf_osal_llist_add_node_head(opriv,
					  list,
					  list_node);

	return WIFI_NRF_STATUS_SUCCESS;
}

void wifi_nrf_utils_list_del_node(struct wifi_nrf_osal_priv *opriv,
				  void *list,
				  void *data)
{
	void *stored_data;
	void *list_node = NULL;
	void *list_node_next = NULL;

	list_node = wifi_nrf_osal_llist_get_node_head(opriv,
						      list);

	while (list_node) {
		stored_data = wifi_nrf_osal_llist_node_data_get(opriv,
								list_node);

		list_node_next = wifi_nrf_osal_llist_get_node_nxt(opriv,
								  list,
								  list_node);

		if (stored_data == data) {
			wifi_nrf_osal_llist_del_node(opriv,
						     list,
						     list_node);

			wifi_nrf_osal_llist_node_free(opriv,
						      list_node);
		}

		list_node = list_node_next;
	}
}

void *wifi_nrf_utils_list_del_head(struct wifi_nrf_osal_priv *opriv,
				   void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = wifi_nrf_osal_llist_get_node_head(opriv,
						      list);

	if (!list_node) {
		goto out;
	}

	data = wifi_nrf_osal_llist_node_data_get(opriv,
						 list_node);

	wifi_nrf_osal_llist_del_node(opriv,
				     list,
				     list_node);
	wifi_nrf_osal_llist_node_free(opriv,
				      list_node);

out:
	return data;
}


void *wifi_nrf_utils_list_peek(struct wifi_nrf_osal_priv *opriv,
			       void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = wifi_nrf_osal_llist_get_node_head(opriv,
						      list);

	if (!list_node) {
		goto out;
	}

	data = wifi_nrf_osal_llist_node_data_get(opriv,
						 list_node);

out:
	return data;
}


unsigned int wifi_nrf_utils_list_len(struct wifi_nrf_osal_priv *opriv,
				     void *list)
{
	return wifi_nrf_osal_llist_len(opriv,
				       list);
}


enum wifi_nrf_status
wifi_nrf_utils_list_traverse(struct wifi_nrf_osal_priv *opriv,
			     void *list,
			     void *callbk_data,
			     enum wifi_nrf_status (*callbk_func)(void *callbk_data,
								 void *data))
{
	void *list_node = NULL;
	void *data = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	list_node = wifi_nrf_osal_llist_get_node_head(opriv,
						      list);

	while (list_node) {
		data = wifi_nrf_osal_llist_node_data_get(opriv,
							 list_node);

		status = callbk_func(callbk_data,
				     data);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			goto out;
		}

		list_node = wifi_nrf_osal_llist_get_node_nxt(opriv,
							     list,
							     list_node);
	}
out:
	return status;
}
