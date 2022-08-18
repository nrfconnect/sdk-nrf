/* bttester.c - Bluetooth Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/uart_pipe.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define STACKSIZE 2048
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread cmd_thread;

#define CMD_QUEUED 2
struct btp_buf {
	intptr_t _reserved;
	union {
		uint8_t data[BTP_MTU];
		struct btp_hdr hdr;
	};
};

static struct btp_buf cmd_buf[CMD_QUEUED];

static K_FIFO_DEFINE(cmds_queue);
static K_FIFO_DEFINE(avail_queue);

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t buf[1];
	struct core_read_supported_commands_rp *rp = (void *) buf;

	(void)memset(buf, 0, sizeof(buf));

	tester_set_bit(buf, CORE_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf, CORE_READ_SUPPORTED_SERVICES);
	tester_set_bit(buf, CORE_REGISTER_SERVICE);
	tester_set_bit(buf, CORE_UNREGISTER_SERVICE);

	tester_send(BTP_SERVICE_ID_CORE, CORE_READ_SUPPORTED_COMMANDS,
		    BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void supported_services(uint8_t *data, uint16_t len)
{
	uint8_t buf[1];
	struct core_read_supported_services_rp *rp = (void *) buf;

	(void)memset(buf, 0, sizeof(buf));

	tester_set_bit(buf, BTP_SERVICE_ID_CORE);
	tester_set_bit(buf, BTP_SERVICE_ID_GAP);
#if defined(CONFIG_BT_MESH)
	tester_set_bit(buf, BTP_SERVICE_ID_MESH);
	tester_set_bit(buf, BTP_SERVICE_ID_MMDL);
#endif /* CONFIG_BT_MESH */

	tester_send(BTP_SERVICE_ID_CORE, CORE_READ_SUPPORTED_SERVICES,
		    BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void register_service(uint8_t *data, uint16_t len)
{
	struct core_register_service_cmd *cmd = (void *) data;
	uint8_t status;

	switch (cmd->id) {
	case BTP_SERVICE_ID_GAP:
		status = tester_init_gap();
		/* Rsp with success status will be handled by bt enable cb */
		if (status == BTP_STATUS_FAILED) {
			goto rsp;
		}
		return;
#if defined(CONFIG_BT_MESH)
	case BTP_SERVICE_ID_MESH:
		status = tester_init_mesh();
		break;
	case BTP_SERVICE_ID_MMDL:
		status = tester_init_mmdl();
		break;
#endif /* CONFIG_BT_MESH */
	default:
		LOG_WRN("unknown id: 0x%02x", cmd->id);
		status = BTP_STATUS_FAILED;
		break;
	}

rsp:
	tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE, BTP_INDEX_NONE,
		   status);
}

static void unregister_service(uint8_t *data, uint16_t len)
{
	struct core_unregister_service_cmd *cmd = (void *) data;
	uint8_t status;

	switch (cmd->id) {
	case BTP_SERVICE_ID_GAP:
		status = tester_unregister_gap();
		break;
#if defined(CONFIG_BT_MESH)
	case BTP_SERVICE_ID_MESH:
		status = tester_unregister_mesh();
		break;
	case BTP_SERVICE_ID_MMDL:
		status = tester_unregister_mmdl();
		break;
#endif /* CONFIG_BT_MESH */
	default:
		LOG_WRN("unknown id: 0x%x", cmd->id);
		status = BTP_STATUS_FAILED;
		break;
	}

	tester_rsp(BTP_SERVICE_ID_CORE, CORE_UNREGISTER_SERVICE, BTP_INDEX_NONE,
		   status);
}

static void handle_core(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len)
{
	if (index != BTP_INDEX_NONE) {
		LOG_WRN("index != BTP_INDEX_NONE: 0x%x", index);
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, index, BTP_STATUS_FAILED);
		return;
	}

	switch (opcode) {
	case CORE_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case CORE_READ_SUPPORTED_SERVICES:
		supported_services(data, len);
		return;
	case CORE_REGISTER_SERVICE:
		register_service(data, len);
		return;
	case CORE_UNREGISTER_SERVICE:
		unregister_service(data, len);
		return;
	default:
		LOG_WRN("unknown opcode: 0x%x", opcode);
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, BTP_INDEX_NONE,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

static void cmd_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		struct btp_buf *cmd;
		uint16_t len;

		cmd = k_fifo_get(&cmds_queue, K_FOREVER);

		len = sys_le16_to_cpu(cmd->hdr.len);

		/* TODO
		 * verify if service is registered before calling handler
		 */

		switch (cmd->hdr.service) {
		case BTP_SERVICE_ID_CORE:
			handle_core(cmd->hdr.opcode, cmd->hdr.index,
				    cmd->hdr.data, len);
			break;
		case BTP_SERVICE_ID_GAP:
			tester_handle_gap(cmd->hdr.opcode, cmd->hdr.index,
					  cmd->hdr.data, len);
			break;
#if defined(CONFIG_BT_MESH)
		case BTP_SERVICE_ID_MESH:
			tester_handle_mesh(cmd->hdr.opcode, cmd->hdr.index,
					   cmd->hdr.data, len);
			break;
		case BTP_SERVICE_ID_MMDL:
			tester_handle_mmdl(cmd->hdr.opcode, cmd->hdr.index,
					   cmd->hdr.data, len);
			break;
#endif /* CONFIG_BT_MESH */
		default:
			LOG_WRN("unknown service: 0x%x", cmd->hdr.service);
			tester_rsp(cmd->hdr.service, cmd->hdr.opcode,
				   cmd->hdr.index, BTP_STATUS_FAILED);
			break;
		}

		k_fifo_put(&avail_queue, cmd);
	}
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct btp_hdr *cmd = (void *) buf;
	struct btp_buf *new_buf;
	uint16_t len;

	if (*off < sizeof(*cmd)) {
		return buf;
	}

	len = sys_le16_to_cpu(cmd->len);
	if (len > BTP_MTU - sizeof(*cmd)) {
		LOG_ERR("BT tester: invalid packet length");
		*off = 0;
		return buf;
	}

	if (*off < sizeof(*cmd) + len) {
		return buf;
	}

	new_buf =  k_fifo_get(&avail_queue, K_NO_WAIT);
	if (!new_buf) {
		LOG_ERR("BT tester: RX overflow");
		*off = 0;
		return buf;
	}

	k_fifo_put(&cmds_queue, CONTAINER_OF(buf, struct btp_buf, data));

	*off = 0;
	return new_buf->data;
}

void tester_init(void)
{
	int i;
	struct btp_buf *buf;

	LOG_DBG("Initializing tester");

	for (i = 0; i < CMD_QUEUED; i++) {
		k_fifo_put(&avail_queue, &cmd_buf[i]);
	}

	k_thread_create(&cmd_thread, stack, STACKSIZE, cmd_handler,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	buf = k_fifo_get(&avail_queue, K_NO_WAIT);
	uart_pipe_register(buf->data, BTP_MTU, recv_cb);

	tester_send(BTP_SERVICE_ID_CORE, CORE_EV_IUT_READY, BTP_INDEX_NONE,
		    NULL, 0);
}

void tester_send(uint8_t service, uint8_t opcode, uint8_t index, uint8_t *data,
		 size_t len)
{
	struct btp_hdr msg;

	msg.service = service;
	msg.opcode = opcode;
	msg.index = index;
	msg.len = len;

	uart_pipe_send((uint8_t *)&msg, sizeof(msg));
	if (data && len) {
		uart_pipe_send(data, len);
	}
}

void tester_rsp(uint8_t service, uint8_t opcode, uint8_t index, uint8_t status)
{
	struct btp_status s;

	if (status == BTP_STATUS_SUCCESS) {
		tester_send(service, opcode, index, NULL, 0);
		return;
	}

	s.code = status;
	tester_send(service, BTP_STATUS, index, (uint8_t *) &s, sizeof(s));
}
