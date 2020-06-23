/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/time.h>
#include "model_utils.h"
#include "time_internal.h"

#define UNCERTAINTY_STEP_MS 10
#define SUBSEC_MAX 256U
#define UNCERTAINTY_MS_MAX 2550U

static uint8_t ms_to_subsec_conv(uint16_t ms)
{
	return ((ms * SUBSEC_MAX) / MSEC_PER_SEC);
}

static uint16_t subsec_to_ms_conv(uint8_t subsec)
{
	return ((subsec * MSEC_PER_SEC) / SUBSEC_MAX);
}

void bt_mesh_time_decode_time_params(struct net_buf_simple *buf,
				     struct bt_mesh_time_status *status)
{
	uint64_t sec = bt_mesh_time_buf_pull_tai_sec(buf);
	uint8_t subsec = net_buf_simple_pull_u8(buf);

	status->tai = (sec * MSEC_PER_SEC) + subsec_to_ms_conv(subsec);
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
	uint64_t sec = status->tai / MSEC_PER_SEC;
	uint8_t subsec = ms_to_subsec_conv(status->tai % MSEC_PER_SEC);

	uint8_t raw_uncertainty =
		(status->uncertainty < UNCERTAINTY_MS_MAX ?
			 (status->uncertainty / UNCERTAINTY_STEP_MS) :
			 UINT8_MAX);

	bt_mesh_time_buf_put_tai_sec(buf, sec);
	net_buf_simple_add_u8(buf, subsec);
	net_buf_simple_add_u8(buf, raw_uncertainty);
	net_buf_simple_add_le16(
		buf, (status->is_authority |
		      ((uint16_t)(status->tai_utc_delta + UTC_CHANGE_ZERO_POINT)
		       << 1)));
	net_buf_simple_add_u8(
		buf, (uint8_t)(status->time_zone_offset + ZONE_CHANGE_ZERO_POINT));
}
