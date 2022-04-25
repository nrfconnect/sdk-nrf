/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_SHELL_ZDO_TYPES_H__
#define ZIGBEE_SHELL_ZDO_TYPES_H__

#include <zboss_api.h>


/* Forward declaration of context manager entry structure. */
struct ctx_entry;

/* Additional context for commands which request tables. */
struct zdo_req_seq {
	/* Starting index for the requested elements. */
	zb_uint8_t  start_index;
	/* Destination address. */
	zb_uint16_t dst_addr;
};

/* Structure to store an information required for sending zdo requests. */
struct zdo_req_info {
	zb_bufid_t buffer_id;
	zb_uint8_t ctx_timeout;
	zb_uint8_t (*req_fn)(zb_uint8_t param, zb_callback_t cb_fn);
	void (*timeout_cb_fn)(zb_bufid_t bufid);
	void (*req_cb_fn)(zb_bufid_t bufid);
};

/* Structure used to store ZDO data in the context manager entry. */
struct zdo_data {
	bool is_broadcast;
	bool (*app_cb_fn)(struct ctx_entry *ctx_entry, uint8_t param);
	struct zdo_req_seq req_seq;
	struct zdo_req_info zdo_req;
};

#endif /* ZIGBEE_SHELL_ZDO_TYPES_H__ */
