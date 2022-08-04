/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/services/hids.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_sample, LOG_LEVEL_INF);

#include "hids_helper.h"

#define BASE_USB_HID_SPEC_VERSION	0x0101

#define REPORT_ID_CONSUMER_CTRL		1
#define REPORT_IDX_CONSUMER_CTRL	0
#define REPORT_SIZE_CONSUMER_CTRL	2  /* bytes */

/* HIDS instance. */
BT_HIDS_DEF(hids_obj,
	    REPORT_SIZE_CONSUMER_CTRL);


static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		int error = bt_hids_connected(&hids_obj, conn);

		if (error) {
			LOG_ERR("Failed to notify HIDS about connection %d", error);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err = bt_hids_disconnected(&hids_obj, conn);

	if (err) {
		LOG_ERR("Failed to notify HIDS about disconnection %d", err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
};

int hids_helper_init(void)
{
	int err;
	struct bt_hids_init_param hids_init_param = { 0 };
	struct bt_hids_inp_rep *rep;

	static const uint8_t report_map[] = {
		0x05, 0x0C,				/* Usage page (Consumer Control) */
		0x09, 0x01,				/* Usage (Consumer Control) */
		0xA1, 0x01,				/* Collection (Application) */
		0x85, REPORT_ID_CONSUMER_CTRL,		/* Report ID */
		0x15, 0x00,				/* Logical minimum (0) */
		0x26, 0xFF, 0x03,			/* Logical maximum (0x3FF) */
		0x19, 0x00,				/* Usage minimum (0) */
		0x2A, 0xFF, 0x03,			/* Usage maximum (0x3FF) */
		0x75, 0x10,				/* Report Size (16) */
		0x95, 0x01,				/* Report Count */
		0x81, 0x00,				/* Input (Data,Array,Absolute) */
		0xC0,					/* End Collection (Application) */
	};

	hids_init_param.rep_map.data = report_map;
	hids_init_param.rep_map.size = sizeof(report_map);

	hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags = (BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE);

	rep = &hids_init_param.inp_rep_group_init.reports[0];
	rep->id = REPORT_ID_CONSUMER_CTRL;
	rep->size = REPORT_SIZE_CONSUMER_CTRL;

	hids_init_param.inp_rep_group_init.cnt = 1;

	err = bt_hids_init(&hids_obj, &hids_init_param);

	return err;
}

int hids_helper_volume_ctrl(enum hids_helper_volume_change volume_change)
{
	static const uint16_t usage_id_vol_up = 0x00E9;
	static const uint16_t usage_id_vol_down = 0x00EA;
	static const uint16_t usage_id_empty;

	uint8_t buf[REPORT_SIZE_CONSUMER_CTRL];

	switch (volume_change) {
	case HIDS_HELPER_VOLUME_CHANGE_DOWN:
		sys_put_le16(usage_id_vol_down, buf);
		break;

	case HIDS_HELPER_VOLUME_CHANGE_NONE:
		sys_put_le16(usage_id_empty, buf);
		break;

	case HIDS_HELPER_VOLUME_CHANGE_UP:
		sys_put_le16(usage_id_vol_up, buf);
		break;

	default:
		return -EINVAL;
	}

	return bt_hids_inp_rep_send(&hids_obj, NULL, REPORT_IDX_CONSUMER_CTRL, buf, ARRAY_SIZE(buf),
				    NULL);
}
