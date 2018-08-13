/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup nrf_ble_link_ctx_manager BLE Link Context Manager. API
 * @{
 * @brief API for the BLE Link Context Manager.
 */

#ifndef _BLE_LINK_CTX_MANAGER_H
#define _BLE_LINK_CTX_MANAGER_H

#include <zephyr.h>
#include <misc/__assert.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Macro for defining a BLE Link Context Manager instance.
 *
 * @param  _name          Name of the instance.
 * @param  _max_clients   Maximum number of clients connected at a time.
 * @param  _ctx_size      Context size in bytes for a single link.
 */
#define BLE_LINK_CTX_MANAGER_DEF(_name, _max_clients, _ctx_size)                             \
	K_MEM_SLAB_DEFINE(_name##_mem_slab,                                                  \
			  ROUND_UP(_ctx_size, CONFIG_NRF_BT_LINK_CTX_MANAGER_MEM_BUF_ALIGN), \
			  (_max_clients),                                                    \
			  CONFIG_NRF_BT_LINK_CTX_MANAGER_MEM_BUF_ALIGN);                     \
	K_MUTEX_DEFINE(_name##_mutex);                                                       \
	static struct ble_link_ctx_manager CONCAT(_name, _link_manager) =                    \
	{                                                                                    \
		.mem_slab = &CONCAT(_name, _mem_slab),                                       \
		.mutex = &_name##_mutex                                                      \
	}

/** @brief Connection context data. */
struct ble_link_conn_ctx {
	/** Pointer to connection context data. */
	void *data;

	 /** Pointer to Connection Object. */
	struct bt_conn *conn;
};

/** @brief BLE Link Context manager structure.
 */
struct ble_link_ctx_manager {
	/** Connection context. */
	struct ble_link_conn_ctx conn_ctx[CONFIG_BT_MAX_CONN];

	/** Context data Mutex. */
	struct k_mutex * const mutex;

	/** Memory Slab Instance. */
	struct k_mem_slab * const mem_slab;
};

/**
 * @brief Function for getting block size.
 *
 * @param ctx_manager Pointer to BLE Link Context manager structure.
 *
 * @return Block size in bytes.
 */
static inline size_t ble_link_ctx_manager_get_block_size(struct ble_link_ctx_manager *ctx_manager)
{
	return ctx_manager->mem_slab->block_size;
}

/**
 * @brief Function for getting the connection context number.
 *
 * @param ctx_manager Pointer to BLE Link Context manager structure.
 *
 * @return Number of context.
 */
static inline size_t ble_link_ctx_manager_get_ctx_num(struct ble_link_ctx_manager *ctx_manager)
{
	return ARRAY_SIZE(ctx_manager->conn_ctx);
}

/**
 * @brief Function for allocating memory for connection context data.
 *        The function can set the pointer to the allocated memory.
 *
 * @param ctx_manager Pointer to BLE Link Context manager structure.
 * @param conn        Pointer to Connection object.
 *
 * @warning This function should be used in conjunction with
 *	    the @ref ble_link_ctx_manager_release to ensure proper operation
 *
 * @return In the case of success pointer to connection context data,
 *	   otherwise NULL.
 */
void *ble_link_ctx_manager_alloc(struct ble_link_ctx_manager *ctx_manager,
				 struct bt_conn *conn);

/**
 * @brief Function for free allocated memory for connection.
 *
 * @param ctx_manager Pointer to BLE Link Context manager structure.
 * @param conn        Pointer to Connection object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ble_link_ctx_manager_free(struct ble_link_ctx_manager *ctx_manager,
			      struct bt_conn *conn);

/**
 * @brief Function for free all allocated context data
 *	  for all exiting connections.
 *
 * @param ctx_manager Pointer to BLE Link Context manager structure.
 */
void ble_link_ctx_manager_free_all(struct ble_link_ctx_manager *ctx_manager);

/**
 * @brief Function for getting the link's context data from the link memory pool.
 *
 * This function finds the link's context data in the memory pool.
 * The link to find is identified by the Connection object.
 *
 * @param ctx_manager         Pointer to BLE Link Context manager structure.
 * @param conn                Pointer to Connection object.
 *
 * @warning This function should be used in conjunction with the @ref ble_link_ctx_manager_release
 *	    to ensure proper operation.
 *
 * @return In the case of success pointer to connection context data,
 *	   otherwise NULL.
 */
void *ble_link_ctx_manager_get(struct ble_link_ctx_manager *ctx_manager,
			       struct bt_conn *conn);

/**
 * @brief Function for getting the link context from the link memory pool.
 *
 * This function finds the link context and Connection Object associated with it,
 * in the memory pool. The link to find is identified
 * by its index in connection context array.
 *
 * @param ctx_manager         Pointer to BLE Link Context manager structure.
 * @param conn_ctx            Pointer to Connection context.
 * @param id                  Connection context index.
 *
 * @warning This function should be used in conjunction with the @ref ble_link_ctx_manager_release
 *	    to ensure proper operation.
 *
 * @return In the case of success pointer to ble_link_conn_ctx struct,
 *	   otherwise NULL.
 */
const struct ble_link_conn_ctx *ble_link_ctx_manager_context_get(struct ble_link_ctx_manager *ctx_manager,
								 u8_t id);

/**
 * @brief Function for releasing the link context from the link memory pool.
 *
 * This function finds and release the link context in the memory pool.
 * The link to find is identified by the Context data.
 *
 * @warning This function should be used in conjunction with the @ref ble_link_ctx_manager_get
 *	    or @ref ble_link_ctx_manager_get_by_id to ensure proper operation.
 *
 * @param ctx_manager         Pointer to BLE Link Context manager structure.
 * @param ctx_data            Pointer to data with context for the connection.
 */
void ble_link_ctx_manager_release(struct ble_link_ctx_manager *ctx_manager,
				  void *ctx_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_LINK_CTX_MANAGER_H */
