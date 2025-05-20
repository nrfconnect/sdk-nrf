/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <drivers/flash/flash_rpc.h>

#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_ipc.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>


LOG_MODULE_DECLARE(FLASH_RPC, CONFIG_FLASH_RPC_LOG_LEVEL);

#define CBOR_BUF_FLASH_MSG_SIZE (sizeof(void *) + sizeof(size_t) + sizeof(off_t))

NRF_RPC_IPC_TRANSPORT(flash_rpc_api_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "flash_rpc_api_ept");
NRF_RPC_GROUP_DEFINE(flash_rpc_api, "flash_rpc_api", &flash_rpc_api_tr, NULL, NULL, NULL);

static const struct device *const flash_controller =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

static bool decode_flash_msg(struct nrf_rpc_cbor_ctx *ctx, off_t *offset, void **ptr, size_t *len)
{
	uint32_t ptr_addr;

	if (!zcbor_uint32_decode(ctx->zs, (uint32_t *)offset)) {
		return false;
	}
	if (!zcbor_uint32_decode(ctx->zs, &ptr_addr)) {
		return false;
	}
	*ptr = (void *)ptr_addr;
	if (!zcbor_uint32_decode(ctx->zs, (uint32_t *)len)) {
		return false;
	}

	return true;
}

static void rsp_error_code_send(const struct nrf_rpc_group *group, int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, sizeof(int));

	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

static void flash_rpc_init_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ARG_UNUSED(handler_data);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!device_is_ready(flash_controller)) {
		LOG_DBG("Flash RPC init failed");
		rsp_error_code_send(group, -ENODEV);
		return;
	}

	rsp_error_code_send(group, 0);
}

static void flash_read_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
		       void *handler_data)
{
	int err;
	size_t len;
	off_t offset;
	void *buffer = NULL;

	if (!decode_flash_msg(ctx, &offset, &buffer, &len)) {
		LOG_ERR("Unable to decode flash_rpc message");
		err = -EBADMSG;
		goto error_exit;
	}

	LOG_DBG("buffer_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, buffer, (uint32_t)offset, len);
	err = flash_read(flash_controller, offset, buffer, len);

error_exit:
	nrf_rpc_cbor_decoding_done(group, ctx);
	rsp_error_code_send(group, err);
}

static void flash_write_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data)
{
	int err;
	off_t offset;
	size_t len;
	const void *data = NULL;

	if (!decode_flash_msg(ctx, &offset, (void *)&data, &len)) {
		err = -EBADMSG;
		goto error_exit;
	}

	LOG_DBG("data_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, data, (uint32_t)offset, len);
	err = flash_write(flash_controller, offset, data, len);

error_exit:
	nrf_rpc_cbor_decoding_done(group, ctx);
	rsp_error_code_send(group, err);
}

static void flash_erase_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
		       void *handler_data)
{
	int err;
	size_t size;
	off_t offset;
	void *buffer = NULL;

	if (!decode_flash_msg(ctx, &offset, &buffer, &size)) {
		err = -EBADMSG;
		goto error_exit;
	}

	LOG_DBG("Offset: 0x%"PRIu32", size: %"PRIx32, (uint32_t)offset, size);
	err = flash_erase(flash_controller, offset, size);

error_exit:
	nrf_rpc_cbor_decoding_done(group, ctx);
	rsp_error_code_send(group, err);
}

NRF_RPC_CBOR_CMD_DECODER(flash_rpc_api, flash_init, RPC_COMMAND_FLASH_INIT,
			 flash_rpc_init_handler, NULL);

NRF_RPC_CBOR_CMD_DECODER(flash_rpc_api, flash_read,
			 RPC_COMMAND_FLASH_READ, flash_read_handler,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(flash_rpc_api, flash_write,
			 RPC_COMMAND_FLASH_WRITE, flash_write_handler,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(flash_rpc_api, flash_erase,
			 RPC_COMMAND_FLASH_ERASE, flash_erase_handler,
			 NULL);
