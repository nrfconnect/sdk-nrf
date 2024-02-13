/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <drivers/flash/flash_rpc.h>

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_ipc.h>

LOG_MODULE_DECLARE(FLASH_RPC, CONFIG_FLASH_RPC_LOG_LEVEL);

#define CBOR_BUF_FLASH_MSG_SIZE (sizeof(void *) + sizeof(size_t) + sizeof(off_t))

NRF_RPC_IPC_TRANSPORT(flash_rpc_api_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "flash_rpc_api_ept");
NRF_RPC_GROUP_DEFINE(flash_rpc_api, "flash_rpc_api", &flash_rpc_api_tr, NULL, NULL, NULL);

static const struct device *const flash_controller =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

static void rsp_error_code_send_raw(const struct nrf_rpc_group *group, int err_code)
{
	uint8_t *buf;
	size_t buf_len = sizeof(int);

	nrf_rpc_alloc_tx_buf(group, &buf, sizeof(int));
	nrf_rpc_rsp_no_err(group, buf, buf_len);
}

static void flash_init_handler(const struct nrf_rpc_group *group, const uint8_t *packet,
				   size_t len, void *handler_data)
{
	ARG_UNUSED(handler_data);

	if (!device_is_ready(flash_controller)) {
		LOG_DBG("Flash RPC init failed");
		rsp_error_code_send_raw(group, -ENODEV);
		return;
	}

	rsp_error_code_send_raw(group, 0);
}

static void flash_read_handler(const struct nrf_rpc_group *group, const uint8_t *packet,
				   size_t len, void *handler_data)
{
	ARG_UNUSED(handler_data);
	int err;
	struct flash_rpc_msg *msg = NULL;

	if (sizeof(struct flash_rpc_msg) != len) {
		LOG_ERR("Unable to decode flash_rpc message");
		err = -EBADMSG;
		goto error_exit;
	}

	msg = (struct flash_rpc_msg *)packet;

	LOG_DBG("buffer_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, msg->data,
		(uint32_t)msg->offset, msg->len);
	err = flash_read(flash_controller, msg->offset, msg->data, msg->len);

error_exit:
	rsp_error_code_send_raw(group, err);
}

static void flash_write_handler(const struct nrf_rpc_group *group, const uint8_t *packet,
				   size_t len, void *handler_data)
{
	ARG_UNUSED(handler_data);
	int err;
	struct flash_rpc_msg *msg = NULL;

	if (sizeof(struct flash_rpc_msg) != len) {
		LOG_ERR("Unable to decode flash_rpc message");
		err = -EBADMSG;
		goto error_exit;
	}

	msg = (struct flash_rpc_msg *)packet;

	LOG_DBG("data_ptr: %p offset: 0x%"PRIu32", size: %"PRIx32, msg->data,
		(uint32_t)msg->offset, msg->len);
	err = flash_write(flash_controller, msg->offset, msg->data, msg->len);

error_exit:
	rsp_error_code_send_raw(group, err);
}

static void flash_erase_handler(const struct nrf_rpc_group *group, const uint8_t *packet,
				   size_t len, void *handler_data)
{
	ARG_UNUSED(handler_data);
	int err;
	struct flash_rpc_msg *msg = NULL;

	if (sizeof(struct flash_rpc_msg) != len) {
		LOG_ERR("Unable to decode flash_rpc message");
		err = -EBADMSG;
		goto error_exit;
	}

	msg = (struct flash_rpc_msg *)packet;
	LOG_DBG("offset: 0x%"PRIu32", size: %"PRIx32, (uint32_t)msg->offset, msg->len);
	err = flash_erase(flash_controller, msg->offset, msg->len);

error_exit:
	rsp_error_code_send_raw(group, err);
}

NRF_RPC_CMD_DECODER(flash_rpc_api, flash_init, RPC_COMMAND_FLASH_INIT, flash_init_handler, NULL);
NRF_RPC_CMD_DECODER(flash_rpc_api, flash_read, RPC_COMMAND_FLASH_READ, flash_read_handler, NULL);
NRF_RPC_CMD_DECODER(flash_rpc_api, flash_write, RPC_COMMAND_FLASH_WRITE, flash_write_handler, NULL);
NRF_RPC_CMD_DECODER(flash_rpc_api, flash_erase, RPC_COMMAND_FLASH_ERASE, flash_erase_handler, NULL);
