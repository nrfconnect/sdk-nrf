/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/conn_ctx.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_conn_ctx, CONFIG_BT_CONN_CTX_LOG_LEVEL);

static void bt_conn_ctx_mem_free(struct k_mem_slab *mem_slab, void **data)
{
	k_mem_slab_free(mem_slab, *data);
	*data = NULL;
}

void *bt_conn_ctx_alloc(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_lib != NULL);

	int err = -ENOMEM;
	struct bt_conn_ctx *ctx;

	k_mutex_lock(ctx_lib->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		ctx = &ctx_lib->ctx[i];

		if (!ctx->conn && !ctx->data) {
			err = k_mem_slab_alloc(ctx_lib->mem_slab,
					       &ctx->data,
					       K_NO_WAIT);
			if (!err) {
				ctx->conn = conn;

				LOG_DBG("The memory for the connection context "
					"has been allocated, conn %p, index: %u",
					(void *)conn, i);

				return ctx->data;

			} else {
				break;
			}

		}
	}

	LOG_WRN("Memory can not be allocated");
	k_mutex_unlock(ctx_lib->mutex);

	return NULL;
}

int bt_conn_ctx_free(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_lib != NULL);

	k_mutex_lock(ctx_lib->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn_ctx *ctx = &ctx_lib->ctx[i];

		if (ctx->conn == conn) {
			bt_conn_ctx_mem_free(ctx_lib->mem_slab, &ctx->data);
			ctx->conn = NULL;
			ctx->data = NULL;

			LOG_DBG("The context memory for the connection "
				"has been released, conn %p index %u",
				(void *)conn, i);

			k_mutex_unlock(ctx_lib->mutex);

			return 0;
		}
	}

	LOG_WRN("There is no allocated memory for this connection");
	k_mutex_unlock(ctx_lib->mutex);

	return  -EINVAL;
}

void bt_conn_ctx_free_all(struct bt_conn_ctx_lib *ctx_lib)
{
	__ASSERT_NO_MSG(ctx_lib != NULL);

	k_mutex_lock(ctx_lib->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn_ctx *ctx = &ctx_lib->ctx[i];

		if (ctx->data != NULL) {
			k_mem_slab_free(ctx_lib->mem_slab, (void *)ctx->data);
			ctx->conn = NULL;
			ctx->data = NULL;
		}
	}

	k_mutex_unlock(ctx_lib->mutex);

	LOG_DBG("All allocated memory has been released");
}

void *bt_conn_ctx_get(struct bt_conn_ctx_lib *ctx_lib, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(ctx_lib != NULL);

	k_mutex_lock(ctx_lib->mutex, K_FOREVER);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn_ctx *ctx = &ctx_lib->ctx[i];

		if (ctx->conn == conn) {
			LOG_DBG("Memory block found for the connection");

			return ctx->data;
		}
	}

	LOG_WRN("No memory block for connection");

	k_mutex_unlock(ctx_lib->mutex);

	return NULL;
}

const struct bt_conn_ctx *bt_conn_ctx_get_by_id(struct bt_conn_ctx_lib *ctx_lib, uint8_t id)
{
	__ASSERT_NO_MSG(ctx_lib != NULL);
	__ASSERT_NO_MSG(id < bt_conn_ctx_count(ctx_lib));

	k_mutex_lock(ctx_lib->mutex, K_FOREVER);

	struct bt_conn_ctx *ctx = &ctx_lib->ctx[id];

	if (ctx_lib->ctx[id].conn != NULL) {
		return ctx;
	}

	k_mutex_unlock(ctx_lib->mutex);

	return NULL;
}

void bt_conn_ctx_release(struct bt_conn_ctx_lib *ctx_lib, void *ctx_data)
{
	__ASSERT_NO_MSG(ctx_lib != NULL);
	__ASSERT_NO_MSG(ctx_data != NULL);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn_ctx *ctx = &ctx_lib->ctx[i];

		if (ctx->data == ctx_data) {
			k_mutex_unlock(ctx_lib->mutex);

			return;
		}
	}

	__ASSERT_NO_MSG(false);
}
