/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/common/svc_common.h>
#include <bluetooth/services/hids.h>

#define SYS_LOG_DOMAIN	STRINGIFY(nrf_ble_hids)
#define SYS_LOG_LEVEL	CONFIG_NRF_BT_HIDS_SYS_LOG_LEVEL
#include <logging/sys_log.h>

#define BOOT_MOUSE_INPUT_REPORT_MIN_SIZE 3

/* HID Protocol modes. */
enum {
	HIDS_PROTOCOL_MODE_BOOT   = 0x00,
	HIDS_PROTOCOL_MODE_REPORT = 0x01
};

/* HID Control Point settings. */
enum {
	HIDS_CONTROL_POINT_SUSPEND      = 0x00,
	HIDS_CONTROL_POINT_EXIT_SUSPEND = 0x01
};

/* HIDS report type values. */
enum {
	HIDS_INPUT   = 0x01,
	HIDS_OUTPUT  = 0x02,
	HIDS_FEATURE = 0x03,
};

static ssize_t hids_protocol_mode_write(struct bt_conn *conn,
					struct bt_gatt_attr const *attr,
					void const *buf, u16_t len,
					u16_t offset, u8_t flags)
{
	SYS_LOG_DBG("Writing to Protocol Mode characteristic.");

	u8_t const *new_pm = (u8_t const *) buf;
	u8_t *cur_pm = attr->user_data;

	struct hids_evt evt = { 0 };
	hids_evt_handler_t evt_handler =
		CONTAINER_OF(cur_pm, struct hids, protocol_mode)->evt_handler;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_pm) {
	case HIDS_PROTOCOL_MODE_BOOT:
		evt.type = HIDS_EVT_BOOT_MODE_ENTERED;
		evt_handler(&evt);
		break;
	case HIDS_PROTOCOL_MODE_REPORT:
		evt.type = HIDS_EVT_REPORT_MODE_ENTERED;
		evt_handler(&evt);
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(cur_pm + offset, new_pm, len);
	return len;
}


static ssize_t hids_protocol_mode_read(struct bt_conn *conn,
				       struct bt_gatt_attr const *attr,
				       void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Protocol Mode characteristic.");

	u8_t *protocol_mode = (u8_t *) attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 protocol_mode, sizeof(*protocol_mode));
}


static ssize_t hids_report_map_read(struct bt_conn *conn,
				    struct bt_gatt_attr const *attr,
				    void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Report Map characteristic.");

	struct hids_rep_map *rep_map =
		(struct hids_rep_map *) attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 rep_map->data, rep_map->size);
}


static ssize_t hids_inp_rep_read(struct bt_conn *conn,
				 struct bt_gatt_attr const *attr,
				 void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Input Report characteristic.");

	struct hids_rep *rep = (struct hids_rep *) attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 rep->data, rep->size);
}


static ssize_t hids_inp_rep_ref_read(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr,
				     void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Input Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = HIDS_INPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 report_ref,
				 sizeof(report_ref));
}


static void hids_input_report_ccc_changed(struct bt_gatt_attr const *attr,
					  u16_t value)
{
	SYS_LOG_DBG("Input Report CCCD has changed.");

	if (value == BT_GATT_CCC_NOTIFY) {
		SYS_LOG_DBG("Notification has been turned on");
	}
}


static ssize_t hids_boot_mouse_inp_report_read(struct bt_conn *conn,
					       struct bt_gatt_attr const *attr,
					       void *buf, u16_t len,
					       u16_t offset)
{
	SYS_LOG_DBG("Reading from Boot Mouse Input Report characteristic.");

	struct hids_boot_mouse_inp_rep *rep =
		(struct hids_boot_mouse_inp_rep *) attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 rep->buff, sizeof(rep->buff));
}


static void hids_boot_mouse_inp_rep_ccc_changed(struct bt_gatt_attr const *attr,
						u16_t value)
{
	SYS_LOG_DBG("Boot Mouse Input Report CCCD has changed.");

	if (value == BT_GATT_CCC_NOTIFY) {
		SYS_LOG_DBG("Notification for Boot Mouse has been turned on");
	}
}


static ssize_t hids_info_read(struct bt_conn *conn,
			      struct bt_gatt_attr const *attr,
			      void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from HID information characteristic.");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 HIDS_INFORMATION_CHAR_LEN);
}


static ssize_t hids_ctrl_point_write(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr,
				     void const *buf, u16_t len, u16_t offset,
				     u8_t flags)
{
	SYS_LOG_DBG("Writing to Control Point characteristic.");

	u8_t *cur_cp = attr->user_data;
	u8_t const *new_cp = (u8_t const *) buf;

	struct hids_evt evt = { 0 };
	hids_evt_handler_t evt_handler =
		CONTAINER_OF(cur_cp, struct hids, ctrl_point)->evt_handler;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_cp) {
	case HIDS_CONTROL_POINT_SUSPEND:
		evt.type = HIDS_EVT_HOST_SUSP;
		evt_handler(&evt);
		break;
	case HIDS_CONTROL_POINT_EXIT_SUSPEND:
		evt.type = HIDS_EVT_HOST_EXIT_SUSP;
		evt_handler(&evt);
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(cur_cp + offset, new_cp, len);
	return len;
}


static u8_t hid_information_encode(u8_t *buffer,
				   struct hids_info const *hid_info)
{
	u8_t len = 0;

	sys_put_le16(hid_info->bcd_hid, buffer);
	len += sizeof(u16_t);

	buffer[len++] = hid_info->b_country_code;
	buffer[len++] = hid_info->flags;

	__ASSERT(len == HIDS_INFORMATION_CHAR_LEN,
		 "HIDS Information encoding failed");
	return len;
}


static void hids_input_reports_register(struct hids *hids_obj,
					struct hids_init const *const hids_init)
{
	memcpy(&hids_obj->inp_rep_group, &hids_init->inp_rep_group_init,
		sizeof(struct hids_inp_rep_group));

	for (u8_t i = 0; i < hids_obj->inp_rep_group.cnt; i++) {
		struct hids_inp_rep *hids_inp_rep =
			&hids_obj->inp_rep_group.reports[i];

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);

		hids_inp_rep->att_ind = hids_obj->svc.attr_count;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT,
						       BT_GATT_PERM_READ,
						       hids_inp_rep_read,
						       NULL,
						       &hids_inp_rep->buff));

		CCC_REGISTER(&hids_obj->svc, hids_inp_rep->ccc,
			     hids_input_report_ccc_changed);
		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF,
						       BT_GATT_PERM_READ,
						       hids_inp_rep_ref_read,
						       NULL,
						       &hids_inp_rep->id));
	}
}


int hids_init(struct hids *hids_obj,
	      struct hids_init const *const hids_init_obj)
{
	SYS_LOG_DBG("Initializing HIDS.");

	hids_obj->evt_handler = hids_init_obj->evt_handler;

	/* Register primary service. */
	PRIMARY_SVC_REGISTER(&hids_obj->svc, BT_UUID_HIDS);

	/* Register Protocol Mode characteristic. */
	hids_obj->protocol_mode = HIDS_PROTOCOL_MODE_REPORT;

	CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_PROTOCOL_MODE,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP);
	DESCRIPTOR_REGISTER(&hids_obj->svc,
			    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_PROTOCOL_MODE,
					       BT_GATT_PERM_READ |
					       BT_GATT_PERM_WRITE,
					       hids_protocol_mode_read,
					       hids_protocol_mode_write,
					       &hids_obj->protocol_mode));

	/* Register Input Report characteristics. */
	hids_input_reports_register(hids_obj, hids_init_obj);

	/* Register Report Map characteristic and its descriptor. */
	memcpy(&hids_obj->rep_map, &hids_init_obj->rep_map,
	       sizeof(hids_obj->rep_map));

	CHRC_REGISTER(&hids_obj->svc,
		      BT_UUID_HIDS_REPORT_MAP,
		      BT_GATT_CHRC_READ);
	DESCRIPTOR_REGISTER(&hids_obj->svc,
			    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_MAP,
					       BT_GATT_PERM_READ,
					       hids_report_map_read,
					       NULL,
					       &hids_obj->rep_map));

	/* Register HID Boot Mouse Input Report characteristic, its descriptor
	 * and CCC.
	 */
	if (hids_init_obj->is_mouse) {
		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);

		hids_obj->boot_mouse_inp_rep.att_ind = hids_obj->svc.attr_count;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(
					BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
					BT_GATT_PERM_READ,
					hids_boot_mouse_inp_report_read,
					NULL,
					&hids_obj->boot_mouse_inp_rep));

		CCC_REGISTER(&hids_obj->svc, hids_obj->boot_mouse_inp_rep.ccc,
			     hids_boot_mouse_inp_rep_ccc_changed);
	}

	/* Register HID Information characteristic and its descriptor. */
	hid_information_encode(hids_obj->info, &hids_init_obj->info);

	CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ);
	DESCRIPTOR_REGISTER(&hids_obj->svc,
			    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_INFO,
					       BT_GATT_PERM_READ,
					       hids_info_read,
					       NULL,
					       hids_obj->info));

	/* Register HID Control Point characteristic and its descriptor. */
	CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_CTRL_POINT,
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP);
	DESCRIPTOR_REGISTER(&hids_obj->svc,
			    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_CTRL_POINT,
					       BT_GATT_PERM_WRITE,
					       NULL, hids_ctrl_point_write,
					       &hids_obj->ctrl_point));

	/* Register HIDS attributes in GATT database. */
	return bt_gatt_service_register(&hids_obj->svc);
}


int hids_inp_rep_send(struct hids *hids_obj, u8_t rep_index, u8_t *rep,
		      u8_t len)
{
	struct hids_inp_rep *hids_inp_rep =
		&hids_obj->inp_rep_group.reports[rep_index];

	if (hids_inp_rep->buff.size != len) {
		return -EINVAL;
	}
	memcpy(hids_inp_rep->buff.data, rep, len);

	return bt_gatt_notify(NULL,
			      &hids_obj->svc.attrs[hids_inp_rep->att_ind],
			      hids_inp_rep->buff.data,
			      hids_inp_rep->buff.size);
}


int hids_boot_mouse_inp_rep_send(struct hids *hids_obj, u8_t buttons,
				 s8_t x_delta, s8_t y_delta)
{
	u8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct hids_boot_mouse_inp_rep *boot_mouse_inp_rep =
		&hids_obj->boot_mouse_inp_rep;

	memset(boot_mouse_inp_rep->buff, 0, BOOT_MOUSE_INPUT_REPORT_MIN_SIZE);

	boot_mouse_inp_rep->buff[0] = buttons;
	boot_mouse_inp_rep->buff[1] = (u8_t) x_delta;
	boot_mouse_inp_rep->buff[2] = (u8_t) y_delta;

	/*  @TODO Optional data encoding. */

	return bt_gatt_notify(NULL,
			      &hids_obj->svc.attrs[rep_ind],
			      boot_mouse_inp_rep->buff,
			      sizeof(boot_mouse_inp_rep->buff));
}

