/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <drivers/flash/flash_rpc.h>

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#ifndef CONFIG_FLASH_RPC_SYS_INIT
LOG_MODULE_REGISTER(FLASH_RPC, CONFIG_FLASH_RPC_LOG_LEVEL);
#else
LOG_MODULE_DECLARE(FLASH_RPC, CONFIG_FLASH_RPC_LOG_LEVEL);
#endif

#define CBOR_BUF_FLASH_MSG_SIZE (sizeof(void *) + sizeof(size_t) + sizeof(off_t) + 32)

NRF_RPC_IPC_TRANSPORT(flash_rpc_api_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "flash_rpc_api_ept");
NRF_RPC_GROUP_DEFINE(flash_rpc_api, "flash_rpc_api", &flash_rpc_api_tr, NULL, NULL, NULL);

#if DT_NODE_HAS_STATUS(DT_INST(0, nordic_rpc_flash_controller), okay)
#define DT_DRV_COMPAT nordic_rpc_flash_controller
#else
#error "Node is not available"
#endif

#define FLASH_RPC_CONTROLLER_NODE DT_INST(0, nordic_rpc_flash_controller)
#define FLASH_RPC_NODE DT_INST_CHILD(0, flash_rpc_0)
#define FLASH_RPC_FLASH_SIZE DT_REG_SIZE(FLASH_RPC_NODE)
#define FLASH_RPC_ERASE_UNIT DT_PROP(FLASH_RPC_NODE, erase_block_size)
#define FLASH_RPC_PROG_UNIT DT_PROP(FLASH_RPC_NODE, write_block_size)
#define FLASH_RPC_FLASH_SIZE DT_REG_SIZE(FLASH_RPC_NODE)

#define FLASH_RPC_PAGE_COUNT (FLASH_RPC_FLASH_SIZE/FLASH_RPC_ERASE_UNIT)

static const struct flash_parameters flash_rpc_parameters = {
	.write_block_size = FLASH_RPC_PROG_UNIT,
	.erase_value = 0xff,
};

static void flash_rpc_get_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	int *result = (int *)handler_data;

	if (!zcbor_int32_decode(ctx->zs, result)) {
		LOG_ERR("Unable to decode result");
		*result = -EINVAL;
	}
}

static bool encode_flash_msg(struct nrf_rpc_cbor_ctx *ctx, off_t *offset, void *ptr, size_t *len)
{
	NRF_RPC_CBOR_ALLOC(&flash_rpc_api, *ctx, CBOR_BUF_FLASH_MSG_SIZE);

	if (!zcbor_uint32_put(ctx->zs, (uint32_t)*offset)) {
		return false;
	}
	if (!zcbor_uint32_put(ctx->zs, (uint32_t)ptr)) {
		return false;
	}
	if (!zcbor_uint32_put(ctx->zs, (uint32_t)*len)) {
		return false;
	}

	return true;
}

#ifndef CONFIG_FLASH_RPC_SYS_INIT
static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
	       report->code);
	k_oops();
}
#endif

int flash_rpc_init(const struct device *dev)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;
	const struct rpc_flash_config *dev_config = dev->config;

	ARG_UNUSED(dev_config);

#ifndef CONFIG_FLASH_RPC_SYS_INIT
	err = nrf_rpc_init(err_handler);
	if (err) {
		LOG_ERR("Initializing nRF RPC failed: %d", err);
		return -EINVAL;
	}
#endif

	NRF_RPC_CBOR_ALLOC(&flash_rpc_api, ctx, CBOR_BUF_FLASH_MSG_SIZE);

	err = nrf_rpc_cbor_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_INIT, &ctx,
			       flash_rpc_get_rsp, &result);
	if (err != 0) {
		LOG_ERR("Could not send RPC command: %d", err);
		return err;
	}

	return result;
}

int flash_rpc_read(const struct device *dev, off_t offset, void *buffer, size_t len)
{
	ARG_UNUSED(dev);
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (len == 0) {
		return 0;
	}

	if (buffer == NULL) {
		return -EINVAL;
	}

	if (!encode_flash_msg(&ctx, &offset, buffer, &len)) {
		LOG_ERR("Could not encode flash_rpc message");
		return -EMSGSIZE;
	}

	LOG_DBG("buffer_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, buffer, (uint32_t)offset, len);

	err = nrf_rpc_cbor_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_READ, &ctx,
				flash_rpc_get_rsp, &result);
	if (err) {
		LOG_ERR("Failed to send RPC read command: %d", err);
		return -EIO;
	}

	return result;
}

int flash_rpc_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	ARG_UNUSED(dev);
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (len == 0) {
		return 0;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	if (!encode_flash_msg(&ctx, &offset, (void *)data, &len)) {
		return -EMSGSIZE;
	}

	err = nrf_rpc_cbor_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_WRITE, &ctx,
			flash_rpc_get_rsp, &result);

	LOG_DBG("data_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, data, (uint32_t)offset, len);
	if (err) {
		LOG_ERR("Failed to send RPC write command: %d", err);
		return -EIO;
	}

	return result;
}

int flash_rpc_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	int err;
	int result;

	struct nrf_rpc_cbor_ctx ctx;

	if (!encode_flash_msg(&ctx, &offset, NULL, &size)) {
		return -EMSGSIZE;
	}

	err = nrf_rpc_cbor_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_ERASE, &ctx,
				flash_rpc_get_rsp, &result);
	if (err) {
		LOG_ERR("Failed to send command: %d", err);
		return -EIO;
	}

	return result;
}

static const struct flash_parameters *flash_rpc_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rpc_parameters;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_rpc_pages_layout = {
	.pages_count = FLASH_RPC_PAGE_COUNT,
	.pages_size = FLASH_RPC_ERASE_UNIT,
};

static void flash_rpc_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &flash_rpc_pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_driver_rpc_api = {
	.read = flash_rpc_read,
	.write = flash_rpc_write,
	.erase = flash_rpc_erase,
	.get_parameters = flash_rpc_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rpc_page_layout,
#endif
};

DEVICE_DT_INST_DEFINE(0, flash_rpc_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_RPC_DRIVER_INIT_PRIORITY, &flash_driver_rpc_api);
