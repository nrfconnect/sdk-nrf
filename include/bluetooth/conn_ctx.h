/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_conn_ctx Bluetooth connection context library API
 * @{
 * @brief API for the Bluetooth connection context library.
 */

#ifndef BT_CONN_CTX_H_
#define BT_CONN_CTX_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Macro for defining a Bluetooth connection context library instance.
 *
 * @param  _name	Name of the instance.
 * @param  _max_clients	Maximum number of clients connected at a time.
 * @param  _ctx_sz	Context size in bytes for a single connection.
 */
#define BT_CONN_CTX_DEF(_name, _max_clients, _ctx_sz)                          \
	K_MEM_SLAB_DEFINE(_name##_mem_slab,                                    \
			  ROUND_UP(_ctx_sz, CONFIG_BT_CONN_CTX_MEM_BUF_ALIGN), \
			  (_max_clients),                                      \
			  CONFIG_BT_CONN_CTX_MEM_BUF_ALIGN);                   \
	K_MUTEX_DEFINE(_name##_mutex);                                         \
	static struct bt_conn_ctx_lib _CONCAT(_name, _ctx_lib) =                \
	{                                                                      \
		.mem_slab = &_CONCAT(_name, _mem_slab),                         \
		.mutex = &_name##_mutex                                        \
	}

/** @brief Context data for a connection. */
struct bt_conn_ctx {
	/** Any kind of data associated with a specific connection. */
	void *data;

	 /** The connection that the data is associated with. */
	struct bt_conn *conn;
};

/** @brief Bluetooth connection context library structure. */
struct bt_conn_ctx_lib {
	/** Connection contexts. */
	struct bt_conn_ctx ctx[CONFIG_BT_MAX_CONN];

	/** Context data mutex that ensures that only one connection context is
	  * manipulated at a time. */
	struct k_mutex * const mutex;

	/** Memory slab instance where the memory is allocated. */
	struct k_mem_slab * const mem_slab;
};

/**
 * @brief Get the block size.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 *
 * @return Block size in bytes.
 */
static inline size_t bt_conn_ctx_block_size_get(struct bt_conn_ctx_lib *ctx_lib)
{
	return ctx_lib->mem_slab->block_size;
}

/**
 * @brief Get the number of connection contexts that are managed by the library.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 *
 * @return Number of connection contexts that are managed by the library.
 */
static inline size_t bt_conn_ctx_count(struct bt_conn_ctx_lib *ctx_lib)
{
	return ARRAY_SIZE(ctx_lib->ctx);
}

/**
 * @brief Allocate memory for the connection context data.
 *
 * This function can set the pointer to the allocated memory.
 *
 * This function should be used in conjunction with
 * @ref bt_conn_ctx_release to ensure proper operation.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 * @param conn		Bluetooth connection.
 *
 * @return Pointer to the connection context data if the operation
 *         was successful. Otherwise NULL.
 */
void *bt_conn_ctx_alloc(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn);

/**
 * @brief Free the allocated memory for a connection.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 * @param conn		Bluetooth connection.
 *
 * @retval 0            If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int bt_conn_ctx_free(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn);

/**
 * @brief Free all allocated context data.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 */
void bt_conn_ctx_free_all(struct bt_conn_ctx_lib *ctx_lib);

/**
 * @brief Get the context data of a connection from the memory pool.
 *
 * This function finds a connection's context data in the memory pool.
 * The link to find is identified by the connection object.
 *
 * This function should be used in conjunction with
 * @ref bt_conn_ctx_release to ensure proper operation.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 * @param conn		Bluetooth connection.
 *
 * @return Pointer to the connection context data if the operation
 *         was successful. Otherwise NULL.
 */
void *bt_conn_ctx_get(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn);

/**
 * @brief Get a specific connection context from the memory pool.
 *
 * This function finds the connection context and the associated connection
 * object in the memory pool. The link to find is identified
 * by its index in the connection context array.
 *
 * This function should be used in conjunction with
 * @ref bt_conn_ctx_release to ensure proper operation.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 * @param id		Connection context index.
 *
 * @return Pointer to the @ref bt_conn_ctx struct if the operation
 *         was successful. Otherwise NULL.
 */
const struct bt_conn_ctx *bt_conn_ctx_get_by_id(struct bt_conn_ctx_lib *ctx_lib, uint8_t id);

/**
 * @brief Release a connection context from the memory pool.
 *
 * This function finds and releases a connection context in the memory pool.
 * The link to find is identified by its context data.
 *
 * This function should be used in conjunction with @ref bt_conn_ctx_alloc,
 * @ref bt_conn_ctx_get, or @ref bt_conn_ctx_get_by_id to ensure proper
 * operation.
 *
 * @param ctx_lib	Bluetooth connection context library instance.
 * @param data		Context data for the connection.
 */
void bt_conn_ctx_release(struct bt_conn_ctx_lib *ctx_lib, void *data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CONN_CTX_H_ */
