/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Linked list management functions for the nRF71 driver.
 */

#include <common/mem_mgmt.h>
#include <common/llist_mgmt.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct nrf_wifi_llist_node {
	sys_dnode_t head;
	void *data;
};

struct nrf_wifi_llist {
	sys_dlist_t head;
	unsigned int len;
};

void *nrf_wifi_llist_node_alloc(void)
{
	struct nrf_wifi_llist_node *llist_node;

	llist_node = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(*llist_node));
	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

void *nrf_wifi_ctrl_llist_node_alloc(void)
{
	struct nrf_wifi_llist_node *llist_node;

	llist_node = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*llist_node));
	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

void nrf_wifi_llist_node_free(void *llist_node)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, llist_node);
}

void nrf_wifi_ctrl_llist_node_free(void *llist_node)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, llist_node);
}

void *nrf_wifi_llist_node_data_get(void *llist_node)
{
	struct nrf_wifi_llist_node *node = llist_node;

	return node->data;
}

void nrf_wifi_llist_node_data_set(void *llist_node, void *data)
{
	struct nrf_wifi_llist_node *node = llist_node;

	node->data = data;
}

void *nrf_wifi_llist_alloc(void)
{
	struct nrf_wifi_llist *llist;

	llist = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(*llist));
	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list", __func__);
	}

	return llist;
}

void *nrf_wifi_ctrl_llist_alloc(void)
{
	struct nrf_wifi_llist *llist;

	llist = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*llist));
	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list", __func__);
	}

	return llist;
}

void nrf_wifi_llist_free(void *llist)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, llist);
}

void nrf_wifi_ctrl_llist_free(void *llist)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, llist);
}

void nrf_wifi_llist_init(void *llist)
{
	struct nrf_wifi_llist *list = llist;

	sys_dlist_init(&list->head);
}

void nrf_wifi_llist_add_node_tail(void *llist, void *llist_node)
{
	struct nrf_wifi_llist *list = llist;
	struct nrf_wifi_llist_node *node = llist_node;

	sys_dlist_append(&list->head, &node->head);

	list->len += 1;
}

void nrf_wifi_llist_add_node_head(void *llist, void *llist_node)
{
	struct nrf_wifi_llist *list = llist;
	struct nrf_wifi_llist_node *node = llist_node;

	sys_dlist_prepend(&list->head, &node->head);

	list->len += 1;
}

void *nrf_wifi_llist_get_node_head(void *llist)
{
	struct nrf_wifi_llist *list = llist;
	struct nrf_wifi_llist_node *head_node;

	if (!list->len) {
		return NULL;
	}

	head_node = (struct nrf_wifi_llist_node *)sys_dlist_peek_head(&list->head);

	return head_node;
}

void *nrf_wifi_llist_get_node_nxt(void *llist, void *llist_node)
{
	struct nrf_wifi_llist *list = llist;
	struct nrf_wifi_llist_node *node = llist_node;
	struct nrf_wifi_llist_node *nxt_node;

	nxt_node = (struct nrf_wifi_llist_node *)sys_dlist_peek_next(&list->head,
								     &node->head);

	return nxt_node;
}

void nrf_wifi_llist_del_node(void *llist, void *llist_node)
{
	struct nrf_wifi_llist *list = llist;
	struct nrf_wifi_llist_node *node = llist_node;

	sys_dlist_remove(&node->head);

	list->len -= 1;
}

unsigned int nrf_wifi_llist_len(void *llist)
{
	struct nrf_wifi_llist *list = llist;

	return list->len;
}

void *nrf_wifi_llist_create(void)
{
	void *list;

	list = nrf_wifi_llist_alloc();
	if (!list) {
		LOG_ERR("%s: Unable to allocate list", __func__);
		return NULL;
	}

	nrf_wifi_llist_init(list);

	return list;
}

void *nrf_wifi_ctrl_llist_create(void)
{
	void *list;

	list = nrf_wifi_ctrl_llist_alloc();
	if (!list) {
		LOG_ERR("%s: Unable to allocate list", __func__);
		return NULL;
	}

	nrf_wifi_llist_init(list);

	return list;
}

enum nrf_wifi_status nrf_wifi_llist_add_tail_data(void *llist, void *data)
{
	void *list_node;

	list_node = nrf_wifi_llist_node_alloc();
	if (!list_node) {
		LOG_ERR("%s: Unable to allocate list node", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_llist_node_data_set(list_node, data);
	nrf_wifi_llist_add_node_tail(llist, list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_ctrl_llist_add_tail_data(void *llist, void *data)
{
	void *list_node;

	list_node = nrf_wifi_ctrl_llist_node_alloc();
	if (!list_node) {
		LOG_ERR("%s: Unable to allocate list node", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_llist_node_data_set(list_node, data);
	nrf_wifi_llist_add_node_tail(llist, list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_llist_add_head_data(void *llist, void *data)
{
	void *list_node;

	list_node = nrf_wifi_llist_node_alloc();
	if (!list_node) {
		LOG_ERR("%s: Unable to allocate list node", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_llist_node_data_set(list_node, data);
	nrf_wifi_llist_add_node_head(llist, list_node);

	return NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_llist_del_by_data(void *llist, void *data)
{
	void *list_node;
	void *list_node_next;
	void *stored_data;

	list_node = nrf_wifi_llist_get_node_head(llist);

	while (list_node) {
		stored_data = nrf_wifi_llist_node_data_get(list_node);
		list_node_next = nrf_wifi_llist_get_node_nxt(llist, list_node);

		if (stored_data == data) {
			nrf_wifi_llist_del_node(llist, list_node);
			nrf_wifi_llist_node_free(list_node);
		}

		list_node = list_node_next;
	}
}

void *nrf_wifi_llist_pop_head(void *llist)
{
	void *list_node;
	void *data;

	list_node = nrf_wifi_llist_get_node_head(llist);
	if (!list_node) {
		return NULL;
	}

	data = nrf_wifi_llist_node_data_get(list_node);
	nrf_wifi_llist_del_node(llist, list_node);
	nrf_wifi_llist_node_free(list_node);

	return data;
}

void *nrf_wifi_ctrl_llist_pop_head(void *llist)
{
	void *list_node;
	void *data;

	list_node = nrf_wifi_llist_get_node_head(llist);
	if (!list_node) {
		return NULL;
	}

	data = nrf_wifi_llist_node_data_get(list_node);
	nrf_wifi_llist_del_node(llist, list_node);
	nrf_wifi_ctrl_llist_node_free(list_node);

	return data;
}

void *nrf_wifi_llist_peek_head(void *llist)
{
	void *list_node;

	list_node = nrf_wifi_llist_get_node_head(llist);
	if (!list_node) {
		return NULL;
	}

	return nrf_wifi_llist_node_data_get(list_node);
}

enum nrf_wifi_status nrf_wifi_llist_traverse(void *llist,
					     void *callbk_data,
					     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
										 void *data))
{
	void *list_node;
	void *data;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	list_node = nrf_wifi_llist_get_node_head(llist);

	while (list_node) {
		data = nrf_wifi_llist_node_data_get(list_node);

		status = callbk_func(callbk_data, data);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			return status;
		}

		list_node = nrf_wifi_llist_get_node_nxt(llist, list_node);
	}

	return status;
}
