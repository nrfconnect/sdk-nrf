/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/time.h>
#include "model_utils.h"
#include "time_internal.h"

#define UNCERTAINTY_STEP_MS 10
#define UNCERTAINTY_MS_MAX 2550U

void bt_mesh_time_decode_time_params(struct net_buf_simple *buf,
				     struct bt_mesh_time_status *status)
{
	/* Mesh Model Specification 5.2.1.3: If the TAI Seconds field is
	 * 0, all other fields shall be omitted
	 */
	if (buf->len != BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS) {
		memset(status, 0, sizeof(struct bt_mesh_time_status));
		return;
	}

	status->tai.sec = bt_mesh_time_buf_pull_tai_sec(buf);
	status->tai.subsec = net_buf_simple_pull_u8(buf);
	uint8_t raw_uncertainty = net_buf_simple_pull_u8(buf);

	status->uncertainty = raw_uncertainty * UNCERTAINTY_STEP_MS;

	uint16_t temp = net_buf_simple_pull_le16(buf);

	status->is_authority = (bool)(temp & 1);
	status->tai_utc_delta = (temp >> 1) - UTC_CHANGE_ZERO_POINT;
	status->time_zone_offset =
		net_buf_simple_pull_u8(buf) - ZONE_CHANGE_ZERO_POINT;
}

void bt_mesh_time_encode_time_params(struct net_buf_simple *buf,
				     const struct bt_mesh_time_status *status)
{
	uint8_t raw_uncertainty =
		(status->uncertainty < UNCERTAINTY_MS_MAX ?
			 (status->uncertainty / UNCERTAINTY_STEP_MS) :
			 UINT8_MAX);

	bt_mesh_time_buf_put_tai_sec(buf, status->tai.sec);
	net_buf_simple_add_u8(buf, status->tai.subsec);
	net_buf_simple_add_u8(buf, raw_uncertainty);
	net_buf_simple_add_le16(
		buf, (status->is_authority |
		      ((uint16_t)(status->tai_utc_delta + UTC_CHANGE_ZERO_POINT)
		       << 1)));
	net_buf_simple_add_u8(
		buf, (uint8_t)(status->time_zone_offset + ZONE_CHANGE_ZERO_POINT));
}
