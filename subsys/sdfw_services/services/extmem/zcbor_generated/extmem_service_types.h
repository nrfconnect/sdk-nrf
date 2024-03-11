/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef EXTMEM_SERVICE_TYPES_H__
#define EXTMEM_SERVICE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct extmem_read_done_req {
	uint32_t extmem_read_done_req_request_id;
	uint32_t extmem_read_done_req_error;
	uint32_t extmem_read_done_req_addr;
};

struct extmem_write_setup_req {
	uint32_t extmem_write_setup_req_request_id;
	uint32_t extmem_write_setup_req_error;
	uint32_t extmem_write_setup_req_addr;
};

struct extmem_write_done_req {
	uint32_t extmem_write_done_req_request_id;
	uint32_t extmem_write_done_req_error;
};

struct extmem_erase_done_req {
	uint32_t extmem_erase_done_req_request_id;
	uint32_t extmem_erase_done_req_error;
};

struct extmem_get_capabilities_req {
	uint32_t extmem_get_capabilities_req_request_id;
	uint32_t extmem_get_capabilities_req_error;
	uint32_t extmem_get_capabilities_req_base_addr;
	uint32_t extmem_get_capabilities_req_capacity;
	uint32_t extmem_get_capabilities_req_erase_size;
	uint32_t extmem_get_capabilities_req_write_size;
	uint32_t extmem_get_capabilities_req_chunk_size;
	bool extmem_get_capabilities_req_memory_mapped;
};

struct extmem_req {
	union {
		struct extmem_read_done_req extmem_req_msg_extmem_read_done_req_m;
		struct extmem_write_setup_req extmem_req_msg_extmem_write_setup_req_m;
		struct extmem_write_done_req extmem_req_msg_extmem_write_done_req_m;
		struct extmem_erase_done_req extmem_req_msg_extmem_erase_done_req_m;
		struct extmem_get_capabilities_req extmem_req_msg_extmem_get_capabilities_req_m;
	};
	enum {
		extmem_req_msg_extmem_read_done_req_m_c,
		extmem_req_msg_extmem_write_setup_req_m_c,
		extmem_req_msg_extmem_write_done_req_m_c,
		extmem_req_msg_extmem_erase_done_req_m_c,
		extmem_req_msg_extmem_get_capabilities_req_m_c,
	} extmem_req_msg_choice;
};

struct extmem_rsp {
	enum {
		extmem_rsp_msg_extmem_read_done_rsp_m_c = 1,
		extmem_rsp_msg_extmem_write_setup_rsp_m_c = 3,
		extmem_rsp_msg_extmem_write_done_rsp_m_c = 5,
		extmem_rsp_msg_extmem_erase_done_rsp_m_c = 7,
		extmem_rsp_msg_extmem_get_capabilities_rsp_m_c = 9,
	} extmem_rsp_msg_choice;
};

struct extmem_read_pending_notify {
	uint32_t extmem_read_pending_notify_request_id;
	uint32_t extmem_read_pending_notify_offset;
	uint32_t extmem_read_pending_notify_size;
};

struct extmem_write_pending_notify {
	uint32_t extmem_write_pending_notify_request_id;
	uint32_t extmem_write_pending_notify_offset;
	uint32_t extmem_write_pending_notify_size;
};

struct extmem_erase_pending_notify {
	uint32_t extmem_erase_pending_notify_request_id;
	uint32_t extmem_erase_pending_notify_offset;
	uint32_t extmem_erase_pending_notify_size;
};

struct extmem_get_capabilities_notify_pending {
	uint32_t extmem_get_capabilities_notify_pending_request_id;
};

struct extmem_nfy {
	union {
		struct extmem_read_pending_notify extmem_nfy_msg_extmem_read_pending_notify_m;
		struct extmem_write_pending_notify extmem_nfy_msg_extmem_write_pending_notify_m;
		struct extmem_erase_pending_notify extmem_nfy_msg_extmem_erase_pending_notify_m;
		struct extmem_get_capabilities_notify_pending
			extmem_nfy_msg_extmem_get_capabilities_notify_pending_m;
	};
	enum {
		extmem_nfy_msg_extmem_read_pending_notify_m_c,
		extmem_nfy_msg_extmem_write_pending_notify_m_c,
		extmem_nfy_msg_extmem_erase_pending_notify_m_c,
		extmem_nfy_msg_extmem_get_capabilities_notify_pending_m_c,
	} extmem_nfy_msg_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* EXTMEM_SERVICE_TYPES_H__ */
