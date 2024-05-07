/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/extmem_remote.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include "extmem_service_decode.h"
#include "extmem_service_encode.h"
#include "extmem_service_types.h"

#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(extmem_srvc, CONFIG_SSF_EXTMEM_SERVICE_LOG_LEVEL);

#define FLASH_DEVICE DEVICE_DT_GET(DT_CHOSEN(extmem_device))

#define SSF_CLIENT_SERVICE_DEFINE_GLOBAL(_name, _srvc_name, _req_encode, _rsp_decode)              \
	const struct ssf_client_srvc _name = {                                                     \
		.id = (CONFIG_SSF_##_srvc_name##_SERVICE_ID),                                      \
		.version = (CONFIG_SSF_##_srvc_name##_SERVICE_VERSION),                            \
		.req_encode = (request_encoder)_req_encode,                                        \
		.rsp_decode = (response_decoder)_rsp_decode,                                       \
		.req_buf_size = (CONFIG_SSF_##_srvc_name##_SERVICE_BUFFER_SIZE)                    \
	}

SSF_CLIENT_SERVICE_DEFINE_GLOBAL(extmem_srvc, EXTMEM, cbor_encode_extmem_req,
				 cbor_decode_extmem_rsp);
SSF_CLIENT_NOTIF_LISTENER_DEFINE(extmem_evt_listener, EXTMEM, cbor_decode_extmem_nfy,
				 notify_evt_handler);

enum server_action {
	ACTION_CAPABILITIES,
	ACTION_READ,
	ACTION_WRITE,
	ACTION_ERASE,
};

struct server_state {
	struct k_work action_worker;
	enum server_action action;
	union {
		struct {
			int request_id;
		} capabilities;
		struct {
			off_t offset;
			size_t len;
			int request_id;
		} read;
		struct {
			off_t offset;
			size_t len;
			int request_id;
		} write;
		struct {
			off_t offset;
			size_t len;
			int request_id;
		} erase;
	} action_data;
};

static uint8_t
	flash_op_buffer[CONFIG_SSF_EXTMEM_READ_BUFFER_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);

static struct server_state server;

static size_t cache_size_adjust(size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (line_size == 0) {
		return size;
	}

	return ROUND_UP(size, line_size);
}

static void handle_extmem_read_fn(struct server_state *state,
				  struct extmem_read_pending_notify *data)
{
	state->action = ACTION_READ;
	state->action_data.read.len = data->extmem_read_pending_notify_size;
	state->action_data.read.offset = data->extmem_read_pending_notify_offset;
	state->action_data.read.request_id = data->extmem_read_pending_notify_request_id;
	k_work_submit(&state->action_worker);
}

static void handle_extmem_write_fn(struct server_state *state,
				   struct extmem_write_pending_notify *data)
{
	state->action = ACTION_WRITE;
	state->action_data.write.len = data->extmem_write_pending_notify_size;
	state->action_data.write.offset = data->extmem_write_pending_notify_offset;
	state->action_data.write.request_id = data->extmem_write_pending_notify_request_id;
	k_work_submit(&state->action_worker);
}

static void handle_extmem_erase_fn(struct server_state *state,
				   struct extmem_erase_pending_notify *data)
{
	state->action = ACTION_ERASE;
	state->action_data.erase.len = data->extmem_erase_pending_notify_size;
	state->action_data.erase.offset = data->extmem_erase_pending_notify_offset;
	state->action_data.erase.request_id = data->extmem_erase_pending_notify_request_id;
	k_work_submit(&state->action_worker);
}

static void handle_extmem_capabilities_fn(struct server_state *state,
					  struct extmem_get_capabilities_notify_pending *data)
{
	state->action = ACTION_CAPABILITIES;
	state->action_data.capabilities.request_id =
		data->extmem_get_capabilities_notify_pending_request_id;
	k_work_submit(&state->action_worker);
}

static void read_worker_fn(struct server_state *state, struct extmem_req *req)
{
	const struct device *fdev = FLASH_DEVICE;
	int result = EXTMEM_RESULT_REMOTE_ERROR;

	int err = flash_read(fdev, state->action_data.read.offset, flash_op_buffer,
			     state->action_data.read.len);
	if (err == 0) {
		result = EXTMEM_RESULT_SUCCESS;
	}

	size_t flush_size = cache_size_adjust(state->action_data.read.len);

	sys_cache_data_flush_range(flash_op_buffer, flush_size);

	LOG_HEXDUMP_DBG(flash_op_buffer, state->action_data.read.len, "Read data");

	req->extmem_req_msg_choice = extmem_req_msg_extmem_read_done_req_m_c;
	req->extmem_req_msg_extmem_read_done_req_m.extmem_read_done_req_addr =
		(uintptr_t)flash_op_buffer;
	req->extmem_req_msg_extmem_read_done_req_m.extmem_read_done_req_error = result;
	req->extmem_req_msg_extmem_read_done_req_m.extmem_read_done_req_request_id =
		state->action_data.read.request_id;
}

static void write_worker_fn(struct server_state *state, struct extmem_req *req)
{
	const struct device *fdev = FLASH_DEVICE;
	int result = EXTMEM_RESULT_REMOTE_ERROR;

	struct extmem_rsp rsp;

	req->extmem_req_msg_choice = extmem_req_msg_extmem_write_setup_req_m_c;
	req->extmem_req_msg_extmem_write_setup_req_m.extmem_write_setup_req_addr =
		(uintptr_t)flash_op_buffer;
	req->extmem_req_msg_extmem_write_setup_req_m.extmem_write_setup_req_error =
		EXTMEM_RESULT_SUCCESS;
	req->extmem_req_msg_extmem_write_setup_req_m.extmem_write_setup_req_request_id =
		state->action_data.write.request_id;

	int ret = ssf_client_send_request(&extmem_srvc, req, &rsp, NULL);

	if (ret == 0) {
		size_t invd_size = cache_size_adjust(state->action_data.read.len);

		sys_cache_data_invd_range(flash_op_buffer, invd_size);

		ret = flash_write(fdev, state->action_data.write.offset, flash_op_buffer,
				  state->action_data.write.len);
	}

	if (ret == 0) {
		result = EXTMEM_RESULT_SUCCESS;
	}

	LOG_HEXDUMP_DBG(flash_op_buffer, state->action_data.write.len, "Write data");

	req->extmem_req_msg_choice = extmem_req_msg_extmem_write_done_req_m_c;
	req->extmem_req_msg_extmem_write_done_req_m.extmem_write_done_req_error = result;
	req->extmem_req_msg_extmem_write_done_req_m.extmem_write_done_req_request_id =
		state->action_data.write.request_id;
}

static void erase_worker_fn(struct server_state *state, struct extmem_req *req)
{
	const struct device *fdev = FLASH_DEVICE;
	int result = EXTMEM_RESULT_REMOTE_ERROR;

	int err = flash_erase(fdev, state->action_data.erase.offset, state->action_data.erase.len);

	if (err == 0) {
		result = EXTMEM_RESULT_SUCCESS;
	}

	LOG_HEXDUMP_DBG(flash_op_buffer, state->action_data.erase.len, "erase data");

	req->extmem_req_msg_choice = extmem_req_msg_extmem_erase_done_req_m_c;
	req->extmem_req_msg_extmem_erase_done_req_m.extmem_erase_done_req_error = result;
	req->extmem_req_msg_extmem_erase_done_req_m.extmem_erase_done_req_request_id =
		state->action_data.erase.request_id;
}

static void capabilities_worker_fn(struct server_state *state, struct extmem_req *req)
{
	const struct device *fdev = FLASH_DEVICE;
	const struct flash_driver_api *api = fdev->api;

	const struct flash_pages_layout *layout;
	size_t capacity = 0;
	size_t count = 0;
	size_t erase_size = 0;

	api->page_layout(fdev, &layout, &count);

	for (; count > 0; count--, layout += 1) {
		capacity += layout->pages_count * layout->pages_size;
		erase_size = MAX(erase_size, layout->pages_size);
	}

	req->extmem_req_msg_choice = extmem_req_msg_extmem_get_capabilities_req_m_c;
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_base_addr =
		CONFIG_SSF_EXTMEM_BASE_ADDRESS;
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_capacity =
		capacity;
	req->extmem_req_msg_extmem_get_capabilities_req_m
		.extmem_get_capabilities_req_memory_mapped = false;
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_erase_size =
		erase_size;
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_write_size =
		flash_get_write_block_size(fdev);
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_chunk_size =
		sizeof(flash_op_buffer);
	req->extmem_req_msg_extmem_get_capabilities_req_m.extmem_get_capabilities_req_request_id =
		state->action_data.capabilities.request_id;
}

static void action_handler(struct k_work *work)
{
	struct server_state *state = CONTAINER_OF(work, struct server_state, action_worker);

	struct extmem_req req;
	struct extmem_rsp rsp;

	switch (state->action) {
	case ACTION_READ:
		read_worker_fn(state, &req);
		break;

	case ACTION_CAPABILITIES:
		capabilities_worker_fn(state, &req);
		break;

	case ACTION_WRITE:
		write_worker_fn(state, &req);
		break;

	case ACTION_ERASE:
		erase_worker_fn(state, &req);
		break;

	default:
		__ASSERT_NO_MSG(false);
	}

	(void)ssf_client_send_request(&extmem_srvc, &req, &rsp, NULL);
}

static int notify_evt_handler(struct ssf_notification *notif, void *handler_user_data)
{
	ARG_UNUSED(handler_user_data);

	struct extmem_nfy notification;

	int err = ssf_client_notif_decode(notif, &notification);

	if (err != 0) {
		LOG_ERR("Decoding notification failed: %d", err);
		return err;
	}

	switch (notification.extmem_nfy_msg_choice) {
	case extmem_nfy_msg_extmem_read_pending_notify_m_c:
		LOG_DBG("Read notification");
		struct extmem_read_pending_notify *extmem_read_data =
			&notification.extmem_nfy_msg_extmem_read_pending_notify_m;

		handle_extmem_read_fn(&server, extmem_read_data);
		break;

	case extmem_nfy_msg_extmem_write_pending_notify_m_c:
		LOG_DBG("Write notification");
		struct extmem_write_pending_notify *extmem_write_data =
			&notification.extmem_nfy_msg_extmem_write_pending_notify_m;

		handle_extmem_write_fn(&server, extmem_write_data);
		break;

	case extmem_nfy_msg_extmem_erase_pending_notify_m_c:
		LOG_DBG("Erase notification");
		struct extmem_erase_pending_notify *extmem_erase_data =
			&notification.extmem_nfy_msg_extmem_erase_pending_notify_m;

		handle_extmem_erase_fn(&server, extmem_erase_data);
		break;

	case extmem_nfy_msg_extmem_get_capabilities_notify_pending_m_c:
		LOG_DBG("Capabilities notification");
		struct extmem_get_capabilities_notify_pending *extmem_capabilities_data =
			&notification.extmem_nfy_msg_extmem_get_capabilities_notify_pending_m;

		handle_extmem_capabilities_fn(&server, extmem_capabilities_data);
		break;

	default:
		break;
	}
	ssf_client_notif_decode_done(notif);
	return err;
}

int extmem_remote_init(void)
{
	int err = 0;

	k_work_init(&server.action_worker, action_handler);

	err = ssf_client_notif_register(&extmem_evt_listener, NULL);
	if (err != 0) {
		LOG_ERR("Failed to register for notifications: %d", err);
		return -EIO;
	}

	return err;
}
