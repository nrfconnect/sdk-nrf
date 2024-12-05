/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef DEVICE_INFO_SERVICE_TYPES_H__
#define DEVICE_INFO_SERVICE_TYPES_H__

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

struct entity_r {
	enum {
		entity_UUID_c = 0,
		entity_TYPE_c = 1,
		entity_TESTIMPRINT_c = 2,
		entity_PARTNO_c = 3,
		entity_HWREVISION_c = 4,
		entity_PRODUCTIONREVISION_c = 5,
	} entity_choice;
};

struct stat_r {
	enum {
		stat_SUCCESS_c = 0,
		stat_INTERNAL_ERROR_c = 16781313,
		stat_UNPROGRAMMED_c = 16781314,
	} stat_choice;
};

struct read_req {
	struct entity_r read_req_target;
};

struct write_req {
	struct entity_r write_req_target;
	uint32_t write_req_data_uint[8];
	size_t write_req_data_uint_count;
};

struct read_resp {
	struct entity_r read_resp_target;
	struct stat_r read_resp_status;
	uint32_t read_resp_data_uint[8];
	size_t read_resp_data_uint_count;
};

struct write_resp {
	struct entity_r write_resp_target;
	struct stat_r write_resp_status;
};

struct device_info_req {
	union {
		struct read_req req_msg_read_req_m;
		struct write_req req_msg_write_req_m;
	};
	enum {
		req_msg_read_req_m_c,
		req_msg_write_req_m_c,
	} req_msg_choice;
};

struct device_info_resp {
	union {
		struct read_resp resp_msg_read_resp_m;
		struct write_resp resp_msg_write_resp_m;
	};
	enum {
		resp_msg_read_resp_m_c,
		resp_msg_write_resp_m_c,
	} resp_msg_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_INFO_SERVICE_TYPES_H__ */
