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

void *nrf_wifi_utils_list_alloc(struct nrf_wifi_osal_priv *opriv)
{
	void *list = NULL;

	list = nrf_wifi_osal_llist_alloc(opriv);

	if (!list) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate list\n",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_llist_init(opriv,
				 list);

out:
	return list;

}


void nrf_wifi_utils_list_free(struct nrf_wifi_osal_priv *opriv,
			      void *list)
{
	nrf_wifi_osal_llist_free(opriv,
				 list);
}


enum nrf_wifi_status nrf_wifi_utils_list_add_tail(struct nrf_wifi_osal_priv *opriv,
						  void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = nrf_wifi_osal_llist_node_alloc(opriv);

	if (!list_node) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate list node\n",
				      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_llist_node_data_set(opriv,
					  list_node,
					  data);

	nrf_wifi_osal_llist_add_node_tail(opriv,
					  list,
					  list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_utils_list_add_head(struct nrf_wifi_osal_priv *opriv,
						  void *list,
						  void *data)
{
	void *list_node = NULL;

	list_node = nrf_wifi_osal_llist_node_alloc(opriv);

	if (!list_node) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate list node\n",
				      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_llist_node_data_set(opriv,
					  list_node,
					  data);

	nrf_wifi_osal_llist_add_node_head(opriv,
					  list,
					  list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_utils_list_del_node(struct nrf_wifi_osal_priv *opriv,
				  void *list,
				  void *data)
{
	void *stored_data;
	void *list_node = NULL;
	void *list_node_next = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(opriv,
						      list);

	while (list_node) {
		stored_data = nrf_wifi_osal_llist_node_data_get(opriv,
								list_node);

		list_node_next = nrf_wifi_osal_llist_get_node_nxt(opriv,
								  list,
								  list_node);

		if (stored_data == data) {
			nrf_wifi_osal_llist_del_node(opriv,
						     list,
						     list_node);

			nrf_wifi_osal_llist_node_free(opriv,
						      list_node);
		}

		list_node = list_node_next;
	}
}

void *nrf_wifi_utils_list_del_head(struct nrf_wifi_osal_priv *opriv,
				   void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(opriv,
						      list);

	if (!list_node) {
		goto out;
	}

	data = nrf_wifi_osal_llist_node_data_get(opriv,
						 list_node);

	nrf_wifi_osal_llist_del_node(opriv,
				     list,
				     list_node);
	nrf_wifi_osal_llist_node_free(opriv,
				      list_node);

out:
	return data;
}


void *nrf_wifi_utils_list_peek(struct nrf_wifi_osal_priv *opriv,
			       void *list)
{
	void *list_node = NULL;
	void *data = NULL;

	list_node = nrf_wifi_osal_llist_get_node_head(opriv,
						      list);

	if (!list_node) {
		goto out;
	}

	data = nrf_wifi_osal_llist_node_data_get(opriv,
						 list_node);

out:
	return data;
}


unsigned int nrf_wifi_utils_list_len(struct nrf_wifi_osal_priv *opriv,
				     void *list)
{
	return nrf_wifi_osal_llist_len(opriv,
				       list);
}


enum nrf_wifi_status
nrf_wifi_utils_list_traverse(struct nrf_wifi_osal_priv *opriv,
			     void *list,
			     void *callbk_data,
			     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
								 void *data))
{
	void *list_node = NULL;
	void *data = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	list_node = nrf_wifi_osal_llist_get_node_head(opriv,
						      list);

	while (list_node) {
		data = nrf_wifi_osal_llist_node_data_get(opriv,
							 list_node);

		status = callbk_func(callbk_data,
				     data);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			goto out;
		}

		list_node = nrf_wifi_osal_llist_get_node_nxt(opriv,
							     list,
							     list_node);
	}
out:
	return status;
}
