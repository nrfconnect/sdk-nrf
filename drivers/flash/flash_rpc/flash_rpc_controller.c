/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <drivers/flash/flash_rpc.h>

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_ipc.h>

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

static void rsp_error_code_handle_raw(const struct nrf_rpc_group *group, const uint8_t *packet,
				      size_t len, void *handler_data)
{
	int *result = (int *)handler_data;

	if (sizeof(int) == len) {
		*result = *packet;
	} else {
		LOG_ERR("Raw: Unable to decode result with length: %d", len);
		*result = -EINVAL;
	}
	LOG_DBG("Raw rsp len:%d data:%d", len, *result);
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
	uint8_t *buf;
	const struct rpc_flash_config *dev_config = dev->config;

	ARG_UNUSED(dev_config);

#ifndef CONFIG_FLASH_RPC_SYS_INIT
	err = nrf_rpc_init(err_handler);
	if (err) {
		LOG_ERR("Initializing nRF RPC failed: %d", err);
		return -EINVAL;
	}
#endif
	nrf_rpc_alloc_tx_buf(&flash_rpc_api, &buf, 0);
	err = nrf_rpc_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_INIT, buf, 0,
			       rsp_error_code_handle_raw, &result);
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
	uint8_t *buf;

	if (len == 0) {
		return 0;
	}

	if (buffer == NULL) {
		return -EINVAL;
	}

	struct flash_rpc_msg msg = {
		.offset = offset,
		.data = buffer,
		.len = len,
	};

	buf = (uint8_t *)&msg;

	LOG_DBG("buffer_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, msg.data,
		(uint32_t)msg.offset, msg.len);

	nrf_rpc_alloc_tx_buf(&flash_rpc_api, &buf, sizeof(msg));
	err = nrf_rpc_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_READ, buf, sizeof(struct flash_rpc_msg),
	rsp_error_code_handle_raw, &result);

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
	uint8_t *buf;

	if (len == 0) {
		return 0;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	struct flash_rpc_msg msg = {
		.offset = offset,
		.data = (void *)data,
		.len = len,
	};

	buf = (uint8_t *)&msg;

	nrf_rpc_alloc_tx_buf(&flash_rpc_api, &buf, sizeof(msg));
	LOG_DBG("buffer_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, msg.data,
		(uint32_t)msg.offset, msg.len);
	err = nrf_rpc_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_WRITE, buf, sizeof(struct flash_rpc_msg),
	rsp_error_code_handle_raw, &result);

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
	uint8_t *buf;

	struct flash_rpc_msg msg = {
		.offset = offset,
		.data = NULL,
		.len = size,
	};

	buf = (uint8_t *)&msg;

	nrf_rpc_alloc_tx_buf(&flash_rpc_api, &buf, sizeof(msg));
	LOG_DBG("offset: 0x%"PRIu32", size: %"PRIx32, (uint32_t)msg.offset, msg.len);
	err = nrf_rpc_cmd(&flash_rpc_api, RPC_COMMAND_FLASH_ERASE, buf,
			  sizeof(struct flash_rpc_msg), rsp_error_code_handle_raw, &result);

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
