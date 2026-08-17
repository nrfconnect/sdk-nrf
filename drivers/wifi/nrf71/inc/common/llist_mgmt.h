/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing linked list management function declarations for the nRF71 driver.
 */

#ifndef __LLIST_MGMT_H__
#define __LLIST_MGMT_H__

#include <common/status.h>

/**
 * @brief Allocate a linked list node from the data pool.
 *
 * @return Pointer to the linked list node on success, NULL on failure.
 */
void *nrf_wifi_llist_node_alloc(void);

/**
 * @brief Allocate a linked list node from the control pool.
 *
 * @return Pointer to the linked list node on success, NULL on failure.
 */
void *nrf_wifi_ctrl_llist_node_alloc(void);

/**
 * @brief Free a linked list node allocated from the data pool.
 *
 * @param node Pointer to a linked list node allocated by @ref nrf_wifi_llist_node_alloc.
 */
void nrf_wifi_llist_node_free(void *node);

/**
 * @brief Free a linked list node allocated from the control pool.
 *
 * @param node Pointer to a linked list node allocated by @ref nrf_wifi_ctrl_llist_node_alloc.
 */
void nrf_wifi_ctrl_llist_node_free(void *node);

/**
 * @brief Get data stored in a linked list node.
 *
 * @param node Pointer to a linked list node.
 *
 * @return Pointer to the data stored in the node, or NULL if @p node is NULL.
 */
void *nrf_wifi_llist_node_data_get(void *node);

/**
 * @brief Set data in a linked list node.
 *
 * @param node Pointer to a linked list node.
 * @param data Pointer to the data to store in the node.
 */
void nrf_wifi_llist_node_data_set(void *node, void *data);

/**
 * @brief Allocate a linked list from the data pool.
 *
 * @return Pointer to the linked list on success, NULL on failure.
 */
void *nrf_wifi_llist_alloc(void);

/**
 * @brief Allocate a linked list from the control pool.
 *
 * @return Pointer to the linked list on success, NULL on failure.
 */
void *nrf_wifi_ctrl_llist_alloc(void);

/**
 * @brief Free a linked list allocated from the data pool.
 *
 * @param llist Pointer to a linked list allocated by @ref nrf_wifi_llist_alloc.
 */
void nrf_wifi_llist_free(void *llist);

/**
 * @brief Free a linked list allocated from the control pool.
 *
 * @param llist Pointer to a linked list allocated by @ref nrf_wifi_ctrl_llist_alloc.
 */
void nrf_wifi_ctrl_llist_free(void *llist);

/**
 * @brief Initialize a linked list.
 *
 * @param llist Pointer to a linked list allocated by @ref nrf_wifi_llist_alloc or
 *              @ref nrf_wifi_ctrl_llist_alloc.
 */
void nrf_wifi_llist_init(void *llist);

/**
 * @brief Add a node to the tail of a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 */
void nrf_wifi_llist_add_node_tail(void *llist, void *llist_node);

/**
 * @brief Add a node to the head of a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 */
void nrf_wifi_llist_add_node_head(void *llist, void *llist_node);

/**
 * @brief Get the head node of a linked list without removing it.
 *
 * @param llist Pointer to a linked list.
 *
 * @return Pointer to the head node on success, NULL if the list is empty.
 */
void *nrf_wifi_llist_get_node_head(void *llist);

/**
 * @brief Get the node after @p llist_node in a linked list.
 *
 * The returned node is not removed from the list.
 *
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 *
 * @return Pointer to the next node on success, NULL if there is no next node.
 */
void *nrf_wifi_llist_get_node_nxt(void *llist, void *llist_node);

/**
 * @brief Remove a node from a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to the node to remove.
 */
void nrf_wifi_llist_del_node(void *llist, void *llist_node);

/**
 * @brief Get the number of nodes in a linked list.
 *
 * @param llist Pointer to a linked list.
 *
 * @return Number of nodes in the list.
 */
unsigned int nrf_wifi_llist_len(void *llist);

/**
 * @brief Allocate and initialize a linked list from the data pool.
 *
 * @return Pointer to the linked list on success, NULL on failure.
 */
void *nrf_wifi_llist_create(void);

/**
 * @brief Allocate and initialize a linked list from the control pool.
 *
 * @return Pointer to the linked list on success, NULL on failure.
 */
void *nrf_wifi_ctrl_llist_create(void);

/**
 * @brief Allocate a node and append @p data to the tail of a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param data Pointer to the data to store in the new node.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On success.
 * @retval NRF_WIFI_STATUS_FAIL On failure.
 */
enum nrf_wifi_status nrf_wifi_llist_add_tail_data(void *llist, void *data);

/**
 * @brief Allocate a control-pool node and append @p data to the tail of a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param data Pointer to the data to store in the new node.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On success.
 * @retval NRF_WIFI_STATUS_FAIL On failure.
 */
enum nrf_wifi_status nrf_wifi_ctrl_llist_add_tail_data(void *llist, void *data);

/**
 * @brief Allocate a node and prepend @p data to the head of a linked list.
 *
 * @param llist Pointer to a linked list.
 * @param data Pointer to the data to store in the new node.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On success.
 * @retval NRF_WIFI_STATUS_FAIL On failure.
 */
enum nrf_wifi_status nrf_wifi_llist_add_head_data(void *llist, void *data);

/**
 * @brief Remove all nodes whose stored data pointer matches @p data.
 *
 * Matching nodes are removed from the list and freed.
 *
 * @param llist Pointer to a linked list.
 * @param data Pointer to the data to match.
 */
void nrf_wifi_llist_del_by_data(void *llist, void *data);

/**
 * @brief Remove and return the data stored in the head node.
 *
 * The head node is removed from the list and freed.
 *
 * @param llist Pointer to a linked list.
 *
 * @return Pointer to the data from the head node, or NULL if the list is empty.
 */
void *nrf_wifi_llist_pop_head(void *llist);

/**
 * @brief Remove and return the data stored in the head node using control-pool free.
 *
 * The head node is removed from the list and freed using
 * @ref nrf_wifi_ctrl_llist_node_free.
 *
 * @param llist Pointer to a linked list.
 *
 * @return Pointer to the data from the head node, or NULL if the list is empty.
 */
void *nrf_wifi_ctrl_llist_pop_head(void *llist);

/**
 * @brief Return the data stored in the head node without removing it.
 *
 * @param llist Pointer to a linked list.
 *
 * @return Pointer to the data from the head node, or NULL if the list is empty.
 */
void *nrf_wifi_llist_peek_head(void *llist);

/**
 * @brief Traverse a linked list and invoke a callback for each node.
 *
 * Traversal stops when the callback returns a value other than
 * @ref NRF_WIFI_STATUS_SUCCESS or when all nodes have been visited.
 *
 * @param llist Pointer to a linked list.
 * @param callbk_data Opaque pointer passed to the callback on each invocation.
 * @param callbk_func Callback invoked for each node's stored data pointer.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS If all callbacks succeeded.
 * @retval NRF_WIFI_STATUS_FAIL If a callback failed or the list is empty.
 */
enum nrf_wifi_status nrf_wifi_llist_traverse(void *llist,
					     void *callbk_data,
					     enum nrf_wifi_status (*callbk_func)(void *callbk_data,
										 void *data));

#endif /* __LLIST_MGMT_H__ */
