/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/services/ble_link_ctx_manager.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ble_link_ctx_manager,
		    CONFIG_BT_LINK_CTX_MANAGER_LOG_LEVEL);

static inline void ble_link_ctx_mem_free(struct k_mem_slab *mem_slab,
					 void **data)
{
	k_mem_slab_free(mem_slab,
			data);
	*data = NULL;
}

void *ble_link_ctx_manager_alloc(struct ble_link_ctx_manager *ctx_manager,
				 struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_manager != NULL);

	int err = -ENOMEM;
	struct ble_link_conn_ctx *conn_ctx;

	k_mutex_lock(ctx_manager->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		conn_ctx = &ctx_manager->conn_ctx[i];

		if (!conn_ctx->conn &&
		    !conn_ctx->data) {
			err = k_mem_slab_alloc(ctx_manager->mem_slab,
					       &conn_ctx->data,
					       K_NO_WAIT);
			if (!err) {
				conn_ctx->conn = conn;

				LOG_DBG("The memory for the connection context "
					"has been allocated, conn %p, index: %u",
					conn, i);

				return conn_ctx->data;

			} else {
				break;
			}

		}
	}

	LOG_WRN("Memory can not be allocated");
	k_mutex_unlock(ctx_manager->mutex);

	return NULL;
}

int ble_link_ctx_manager_free(struct ble_link_ctx_manager *ctx_manager,
			      struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_manager != NULL);

	k_mutex_lock(ctx_manager->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct ble_link_conn_ctx *conn_ctx =
				&ctx_manager->conn_ctx[i];

		if (conn_ctx->conn == conn) {
			ble_link_ctx_mem_free(ctx_manager->mem_slab,
					      &conn_ctx->data);
			conn_ctx->conn = NULL;
			conn_ctx->data = NULL;

			LOG_DBG("The context memory for the connection "
				"has been released, conn %p index %u",
				conn, i);

			k_mutex_unlock(ctx_manager->mutex);

			return 0;
		}
	}

	LOG_WRN("There is no allocated memory for this connection");
	k_mutex_unlock(ctx_manager->mutex);

	return  -EINVAL;
}

void ble_link_ctx_manager_free_all(struct ble_link_ctx_manager *ctx_manager)
{
	__ASSERT_NO_MSG(ctx_manager != NULL);

	k_mutex_lock(ctx_manager->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct ble_link_conn_ctx *conn_ctx =
				&ctx_manager->conn_ctx[i];

		if (conn_ctx->data != NULL) {
			k_mem_slab_free(ctx_manager->mem_slab,
					&conn_ctx->data);
			conn_ctx->conn = NULL;
			conn_ctx->data = NULL;
		}
	}

	k_mutex_unlock(ctx_manager->mutex);

	LOG_DBG("All allocated memory has been released");
}

void *ble_link_ctx_manager_get(struct ble_link_ctx_manager *ctx_manager,
			       struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_manager != NULL);

	k_mutex_lock(ctx_manager->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct ble_link_conn_ctx *conn_ctx =
				&ctx_manager->conn_ctx[i];

		if (conn_ctx->conn == conn) {
			LOG_DBG("Memory block found for the connection");

			return conn_ctx->data;
		}
	}

	LOG_WRN("No memory block for connection");

	k_mutex_unlock(ctx_manager->mutex);

	return NULL;
}

const struct ble_link_conn_ctx *ble_link_ctx_manager_context_get(struct ble_link_ctx_manager *ctx_manager,
								 u8_t id)
{
	__ASSERT_NO_MSG(ctx_manager != NULL);
	__ASSERT_NO_MSG(id < ble_link_ctx_manager_get_ctx_num(ctx_manager));

	k_mutex_lock(ctx_manager->mutex, K_FOREVER);

	struct ble_link_conn_ctx *ctx =
			&ctx_manager->conn_ctx[id];

	if (ctx_manager->conn_ctx[id].conn != NULL) {
		return ctx;
	}

	k_mutex_unlock(ctx_manager->mutex);

	return NULL;
}

void ble_link_ctx_manager_release(struct ble_link_ctx_manager *ctx_manager,
				  void *ctx_data)
{
	__ASSERT_NO_MSG(ctx_manager != NULL);
	__ASSERT_NO_MSG(ctx_data != NULL);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct ble_link_conn_ctx *conn_ctx =
				&ctx_manager->conn_ctx[i];

		if (conn_ctx->data == ctx_data) {
			k_mutex_unlock(ctx_manager->mutex);

			return;
		}
	}

	__ASSERT_NO_MSG(false);
}
