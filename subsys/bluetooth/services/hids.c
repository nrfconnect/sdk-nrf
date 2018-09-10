/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <assert.h>
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

int hids_notify_connected(struct hids *hids_obj, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(hids_obj != NULL);

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_alloc(hids_obj->ctx_manager,
									    conn);

	if (!conn_data) {
		SYS_LOG_WRN("There is no free memory to "
			    "allocate the connection context");
		return -ENOMEM;
	}

	memset(conn_data,
	       0,
	       ble_link_ctx_manager_get_block_size(hids_obj->ctx_manager));

	conn_data->pm_ctx_value = HIDS_PROTOCOL_MODE_REPORT;

	/* Assign input report context. */
	conn_data->inp_rep_ctx = (u8_t *)conn_data + sizeof(struct hids_conn_data);

	/* Assign output report context. */
	conn_data->outp_rep_ctx = conn_data->inp_rep_ctx;

	for (size_t i = 0; i < hids_obj->inp_rep_group.cnt; i++) {
		conn_data->outp_rep_ctx +=
				hids_obj->inp_rep_group.reports[i].size;
	}

	/* Assign feature report context. */
	conn_data->feat_rep_ctx = conn_data->outp_rep_ctx;

	for (size_t i = 0; i < hids_obj->outp_rep_group.cnt; i++) {
		conn_data->feat_rep_ctx +=
				hids_obj->outp_rep_group.reports[i].size;
	}

	ble_link_ctx_manager_release(hids_obj->ctx_manager,
				     (void *)conn_data);

	return 0;
}

int hids_notify_disconnected(struct hids *hids_obj, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(hids_obj != NULL);

	int err = ble_link_ctx_manager_free(hids_obj->ctx_manager, conn);

	if (err) {
		SYS_LOG_WRN("The memory was not allocated for the context of this connection.");

		return err;
	}

	return 0;
}

static bool hids_is_notification_enabled(struct bt_conn *conn,
					 struct bt_gatt_ccc_cfg *ccd)
{
	const bt_addr_le_t *conn_addr = bt_conn_get_dst(conn);

	for (size_t i = 0; i < CONFIG_NRF_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		bt_addr_le_t *ccd_addr = &ccd[i].peer;

		if ((!memcmp(conn_addr, ccd_addr, sizeof(bt_addr_le_t))) &&
		    (ccd[i].value == BT_GATT_CCC_NOTIFY)) {
			return true;
		}
	}

	return false;
}

static ssize_t hids_protocol_mode_write(struct bt_conn *conn,
					struct bt_gatt_attr const *attr,
					void const *buf, u16_t len,
					u16_t offset, u8_t flags)
{
	SYS_LOG_DBG("Writing to Protocol Mode characteristic.");

	struct protocol_mode *pm = (struct protocol_mode *) attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(pm, struct hids, pm);
	u8_t const *new_pm = (u8_t const *) buf;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	u8_t *cur_pm = &conn_data->pm_ctx_value;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_pm) {
	case HIDS_PROTOCOL_MODE_BOOT:
		if (pm->evt_handler) {
			pm->evt_handler(HIDS_PM_EVT_BOOT_MODE_ENTERED, conn);
		}
		break;
	case HIDS_PROTOCOL_MODE_REPORT:
		if (pm->evt_handler) {
			pm->evt_handler(HIDS_PM_EVT_REPORT_MODE_ENTERED, conn);
		}
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(cur_pm + offset, new_pm, len);

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return len;
}


static ssize_t hids_protocol_mode_read(struct bt_conn *conn,
				       struct bt_gatt_attr const *attr,
				       void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Protocol Mode characteristic.");

	struct protocol_mode *pm = (struct protocol_mode *) attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(pm, struct hids, pm);
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	u8_t *protocol_mode = &conn_data->pm_ctx_value;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset,
				    protocol_mode, sizeof(*protocol_mode));

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
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

	struct hids_inp_rep *rep =
			(struct hids_inp_rep *) attr->user_data;
	u8_t idx = rep->idx;
	struct hids *hids_ctx = CONTAINER_OF((rep - idx),
					     struct hids,
					     inp_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->inp_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset,
				    rep_data, rep->size);

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static ssize_t hids_inp_rep_ref_read(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr,
				     void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Input Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = HIDS_INPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}


static ssize_t hids_outp_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr,
				  void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Output Report characteristic.");

	struct hids_outp_feat_rep *rep =
			(struct hids_outp_feat_rep *) attr->user_data;
	u8_t idx = rep->idx;
	struct hids *hids_ctx = CONTAINER_OF((rep - idx),
					     struct hids,
					     outp_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static ssize_t hids_outp_rep_write(struct bt_conn *conn,
				   struct bt_gatt_attr const *attr,
				   void const *buf, u16_t len,
				   u16_t offset, u8_t flags)
{
	SYS_LOG_DBG("Writing to Output Report characteristic.");

	struct hids_outp_feat_rep *rep =
			(struct hids_outp_feat_rep *) attr->user_data;
	u8_t idx = rep->idx;
	struct hids *hids_ctx = CONTAINER_OF((rep - idx),
					     struct hids,
					     outp_rep_group.reports);
	u8_t *rep_data;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct hids_rep report = {
			.data = rep_data,
			.size = rep->size,
		};
		rep->handler(&report, conn);
	}

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return len;
}


static ssize_t hids_outp_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Output Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = HIDS_OUTPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}


static ssize_t hids_feat_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr,
				  void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Feature Report characteristic.");

	struct hids_outp_feat_rep *rep =
		(struct hids_outp_feat_rep *) attr->user_data;
	u8_t idx = rep->idx;
	struct hids *hids_ctx = CONTAINER_OF((rep - idx),
					     struct hids,
					     feat_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;


	ret_len =  bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				     rep->size);

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static ssize_t hids_feat_rep_write(struct bt_conn *conn,
				   struct bt_gatt_attr const *attr,
				   void const *buf, u16_t len,
				   u16_t offset, u8_t flags)
{
	SYS_LOG_DBG("Writing to Feature Report characteristic.");

	struct hids_outp_feat_rep *rep =
		(struct hids_outp_feat_rep *) attr->user_data;
	u8_t idx = rep->idx;
	struct hids *hids_ctx = CONTAINER_OF((rep - idx),
					     struct hids,
					     feat_rep_group.reports);
	u8_t *rep_data;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct hids_rep report = {
			.data = rep_data,
			.size = rep->size,
		};
		rep->handler(&report, conn);
	}

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return len;
}


static ssize_t hids_feat_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, u16_t len, u16_t offset)
{
	SYS_LOG_DBG("Reading from Feature Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = HIDS_FEATURE;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}


static void hids_input_report_ccc_changed(struct bt_gatt_attr const *attr,
					  u16_t value)
{
	SYS_LOG_DBG("Input Report CCCD has changed.");

	struct hids_inp_rep *inp_rep =
		CONTAINER_OF(((struct _bt_gatt_ccc *) attr->user_data)->cfg,
			     struct hids_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		SYS_LOG_DBG("Notification has been turned on");
		if (inp_rep->handler != NULL) {
			inp_rep->handler(HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		SYS_LOG_DBG("Notification has been turned off");
		if (inp_rep->handler != NULL) {
			inp_rep->handler(HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}


static ssize_t hids_boot_mouse_inp_report_read(struct bt_conn *conn,
					       struct bt_gatt_attr const *attr,
					       void *buf, u16_t len,
					       u16_t offset)
{
	SYS_LOG_DBG("Reading from Boot Mouse Input Report characteristic.");

	struct hids_boot_mouse_inp_rep *rep =
			(struct hids_boot_mouse_inp_rep *)attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(rep, struct hids, boot_mouse_inp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_mouse_inp_rep_ctx;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset,
				   rep_data,
				   sizeof(conn_data->hids_boot_mouse_inp_rep_ctx));
	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static void hids_boot_mouse_inp_rep_ccc_changed(struct bt_gatt_attr const *attr,
						u16_t value)
{
	SYS_LOG_DBG("Boot Mouse Input Report CCCD has changed.");

	struct hids_boot_mouse_inp_rep *boot_mouse_rep =
		CONTAINER_OF(((struct _bt_gatt_ccc *) attr->user_data)->cfg,
			     struct hids_boot_mouse_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		SYS_LOG_DBG("Notification for Boot Mouse has been turned on");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		SYS_LOG_DBG("Notification for Boot Mouse has been turned off");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}


static ssize_t hids_boot_kb_inp_report_read(struct bt_conn *conn,
					    struct bt_gatt_attr const *attr,
					    void *buf, u16_t len,
					    u16_t offset)
{
	SYS_LOG_DBG("Reading from Boot Keyboard Input Report characteristic.");

	struct hids_boot_kb_inp_rep *rep =
			(struct hids_boot_kb_inp_rep *)attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(rep, struct hids, boot_kb_inp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

	ret_len =  bt_gatt_attr_read(conn, attr, buf, len, offset,
				     rep_data,
				     sizeof(conn_data->hids_boot_kb_inp_rep_ctx));
	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static void hids_boot_kb_inp_rep_ccc_changed(struct bt_gatt_attr const *attr,
					     u16_t value)
{
	SYS_LOG_DBG("Boot Keyboard Input Report CCCD has changed.");

	struct hids_boot_kb_inp_rep *boot_kb_inp_rep =
		CONTAINER_OF(((struct _bt_gatt_ccc *) attr->user_data)->cfg,
			     struct hids_boot_kb_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		SYS_LOG_DBG("Notification for Boot Keyboard has been turned "
			    "on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		SYS_LOG_DBG("Notification for Boot Keyboard has been turned "
			    "on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}


static ssize_t hids_boot_kb_outp_report_read(struct bt_conn *conn,
					     struct bt_gatt_attr const *attr,
					     void *buf, u16_t len,
					     u16_t offset)
{
	SYS_LOG_DBG("Reading from Boot Keyboard Output Report characteristic.");

	struct hids_boot_kb_outp_rep *rep =
			(struct hids_boot_kb_outp_rep *)attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(rep, struct hids, boot_kb_outp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_outp_rep_ctx;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset,
				    rep_data,
				    sizeof(conn_data->hids_boot_kb_outp_rep_ctx));
	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return ret_len;
}


static ssize_t hids_boot_kb_outp_report_write(struct bt_conn *conn,
					      struct bt_gatt_attr const *attr,
					      void const *buf, u16_t len,
					      u16_t offset, u8_t flags)
{
	SYS_LOG_DBG("Writing to Boot Keyboard Output Report characteristic.");

	struct hids_boot_kb_outp_rep *rep =
		(struct hids_boot_kb_outp_rep *) attr->user_data;
	struct hids *hids_ctx = CONTAINER_OF(rep, struct hids, boot_kb_outp_rep);
	u8_t *rep_data;

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_ctx->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_outp_rep_ctx;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct hids_rep report = {
			.data = rep_data,
			.size = sizeof(conn_data->hids_boot_kb_outp_rep_ctx),
		};

		rep->handler(&report, conn);
	}

	ble_link_ctx_manager_release(hids_ctx->ctx_manager,
				     (void *)conn_data);

	return len;
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

	struct control_point *cp = (struct control_point *) attr->user_data;

	u8_t cur_cp = cp->value;
	u8_t const *new_cp = (u8_t const *) buf;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_cp) {
	case HIDS_CONTROL_POINT_SUSPEND:
		if (cp->evt_handler) {
			cp->evt_handler(HIDS_CP_EVT_HOST_SUSP);
		}
		break;
	case HIDS_CONTROL_POINT_EXIT_SUSPEND:
		if (cp->evt_handler) {
			cp->evt_handler(HIDS_CP_EVT_HOST_EXIT_SUSP);
		}
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(&cur_cp + offset, new_cp, len);
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
	__ASSERT_NO_MSG(hids_init->inp_rep_group_init.cnt <=
			CONFIG_NRF_BT_HIDS_INPUT_REP_MAX);
	u8_t offset = 0;

	memcpy(&hids_obj->inp_rep_group, &hids_init->inp_rep_group_init,
		sizeof(hids_obj->inp_rep_group));

	for (size_t i = 0; i < hids_obj->inp_rep_group.cnt; i++) {
		struct hids_inp_rep *hids_inp_rep =
			&hids_obj->inp_rep_group.reports[i];

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);

		hids_inp_rep->att_ind = hids_obj->svc.attr_count;
		hids_inp_rep->offset = offset;
		hids_inp_rep->idx = i;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT,
						       BT_GATT_PERM_READ,
						       hids_inp_rep_read,
						       NULL,
						       hids_inp_rep));

		CCC_REGISTER(&hids_obj->svc, hids_inp_rep->ccc,
			     hids_input_report_ccc_changed);
		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF,
						       BT_GATT_PERM_READ,
						       hids_inp_rep_ref_read,
						       NULL,
						       &hids_inp_rep->id));
		offset += hids_inp_rep->size;
	}
}


static void hids_outp_reports_register(struct hids *hids_obj,
				       struct hids_init const *const hids_init)
{
	__ASSERT_NO_MSG(hids_init->outp_rep_group_init.cnt <=
			CONFIG_NRF_BT_HIDS_OUTPUT_REP_MAX);

	u8_t offset = 0;

	memcpy(&hids_obj->outp_rep_group, &hids_init->outp_rep_group_init,
		sizeof(hids_obj->outp_rep_group));

	for (size_t i = 0; i < hids_obj->outp_rep_group.cnt; i++) {
		struct hids_outp_feat_rep *hids_outp_rep =
			&hids_obj->outp_rep_group.reports[i];

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			      BT_GATT_CHRC_WRITE_WITHOUT_RESP);

		hids_outp_rep->att_ind = hids_obj->svc.attr_count;
		hids_outp_rep->offset = offset;
		hids_outp_rep->idx = i;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT,
						       (BT_GATT_PERM_READ |
						       BT_GATT_PERM_WRITE),
						       hids_outp_rep_read,
						       hids_outp_rep_write,
						       hids_outp_rep));
		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF,
						       BT_GATT_PERM_READ,
						       hids_outp_rep_ref_read,
						       NULL,
						       &hids_outp_rep->id));

		offset += hids_outp_rep->size;
	}
}


static void hids_feat_reports_register(struct hids *hids_obj,
				       struct hids_init const *const hids_init)
{
	__ASSERT_NO_MSG(hids_init->feat_rep_group_init.cnt <=
			CONFIG_NRF_BT_HIDS_FEATURE_REP_MAX);

	memcpy(&hids_obj->feat_rep_group, &hids_init->feat_rep_group_init,
		sizeof(hids_obj->feat_rep_group));

	u8_t offset = 0;

	for (size_t i = 0; i < hids_obj->feat_rep_group.cnt; i++) {
		struct hids_outp_feat_rep *hids_feat_rep =
			&hids_obj->feat_rep_group.reports[i];

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE);

		hids_feat_rep->att_ind = hids_obj->svc.attr_count;
		hids_feat_rep->offset = offset;
		hids_feat_rep->idx = i;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT,
						       (BT_GATT_PERM_READ |
						       BT_GATT_PERM_WRITE),
						       hids_feat_rep_read,
						       hids_feat_rep_write,
						       hids_feat_rep));
		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF,
						       BT_GATT_PERM_READ,
						       hids_feat_rep_ref_read,
						       NULL,
						       &hids_feat_rep->id));

		offset += hids_feat_rep->size;
	}
}


int hids_init(struct hids *hids_obj,
	      struct hids_init const *const hids_init_obj)
{
	SYS_LOG_DBG("Initializing HIDS.");

	hids_obj->pm.evt_handler = hids_init_obj->pm_evt_handler;
	hids_obj->cp.evt_handler = hids_init_obj->cp_evt_handler;

	/* Register primary service. */
	PRIMARY_SVC_REGISTER(&hids_obj->svc, BT_UUID_HIDS);

	/* Register Protocol Mode characteristic. */
	CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_PROTOCOL_MODE,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP);
	DESCRIPTOR_REGISTER(&hids_obj->svc,
			    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_PROTOCOL_MODE,
					       BT_GATT_PERM_READ |
					       BT_GATT_PERM_WRITE,
					       hids_protocol_mode_read,
					       hids_protocol_mode_write,
					       &hids_obj->pm));

	/* Register Input Report characteristics. */
	hids_input_reports_register(hids_obj, hids_init_obj);

	/* Register Output Report characteristics. */
	hids_outp_reports_register(hids_obj, hids_init_obj);

	/* Register Feature Report characteristics. */
	hids_feat_reports_register(hids_obj, hids_init_obj);

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
		hids_obj->is_mouse = hids_init_obj->is_mouse;

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);

		hids_obj->boot_mouse_inp_rep.att_ind = hids_obj->svc.attr_count;
		hids_obj->boot_mouse_inp_rep.handler =
			hids_init_obj->boot_mouse_notif_handler;

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

	/* Register HID Boot Keyboard Input/Output Report characteristic, its
	 * descriptor and CCC.
	 */
	if (hids_init_obj->is_kb) {
		hids_obj->is_kb = hids_init_obj->is_kb;

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_BOOT_KB_IN_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);

		hids_obj->boot_kb_inp_rep.att_ind = hids_obj->svc.attr_count;
		hids_obj->boot_kb_inp_rep.handler =
			hids_init_obj->boot_kb_notif_handler;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(
					BT_UUID_HIDS_BOOT_KB_IN_REPORT,
					BT_GATT_PERM_READ,
					hids_boot_kb_inp_report_read,
					NULL,
					&hids_obj->boot_kb_inp_rep));

		CCC_REGISTER(&hids_obj->svc, hids_obj->boot_kb_inp_rep.ccc,
			     hids_boot_kb_inp_rep_ccc_changed);

		CHRC_REGISTER(&hids_obj->svc, BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
			      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			      BT_GATT_CHRC_WRITE_WITHOUT_RESP);

		hids_obj->boot_kb_outp_rep.att_ind = hids_obj->svc.attr_count;
		hids_obj->boot_kb_outp_rep.handler =
			hids_init_obj->boot_kb_outp_rep_handler;

		DESCRIPTOR_REGISTER(&hids_obj->svc,
				    BT_GATT_DESCRIPTOR(
					BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
					(BT_GATT_PERM_READ |
					BT_GATT_PERM_WRITE),
					hids_boot_kb_outp_report_read,
					hids_boot_kb_outp_report_write,
					&hids_obj->boot_kb_outp_rep));
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
					       &hids_obj->cp));

	/* Register HIDS attributes in GATT database. */
	return bt_gatt_service_register(&hids_obj->svc);
}


int hids_uninit(struct hids *hids_obj)
{
	int err;

	/* Unregister HIDS attributes in GATT database. */
	err = bt_gatt_service_unregister(&hids_obj->svc);
	if (err) {
		return err;
	}

	struct bt_gatt_attr *attr_start = hids_obj->svc.attrs;
	struct bt_gatt_attr *attr = attr_start;
	struct ble_link_ctx_manager *ctx_manager
				= hids_obj->ctx_manager;

	/* Unregister primary service. */
	primary_svc_unregister(attr++);

	/* Unregister Protocol Mode characteristic. */
	chrc_unregister(attr++);
	descriptor_unregister(attr++);

	/* Unregister Input Report characteristics. */
	for (size_t i = 0; i < hids_obj->inp_rep_group.cnt; i++) {
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
		ccc_unregister(attr++);
		descriptor_unregister(attr++);
	}

	/* Unregister Output Report characteristics. */
	for (size_t i = 0; i < hids_obj->outp_rep_group.cnt; i++) {
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
		descriptor_unregister(attr++);
	}

	/* Unregister Feature Report characteristics. */
	for (size_t i = 0; i < hids_obj->feat_rep_group.cnt; i++) {
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
		descriptor_unregister(attr++);
	}

	/* Unregister Report Map characteristic and its descriptor. */
	chrc_unregister(attr++);
	descriptor_unregister(attr++);

	/* Unregister HID Boot Mouse Input Report characteristic, its descriptor
	 * and CCC.
	 */
	if (hids_obj->is_mouse) {
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
		ccc_unregister(attr++);
	}

	/* Unregister HID Boot Keyboard Input/Output Report characteristic, its
	 * descriptor and CCC.
	 */
	if (hids_obj->is_kb) {
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
		ccc_unregister(attr++);
		chrc_unregister(attr++);
		descriptor_unregister(attr++);
	}

	/* Unregister HID Information characteristic and its descriptor. */
	chrc_unregister(attr++);
	descriptor_unregister(attr++);

	/* Unregister HID Control Point characteristic and its descriptor. */
	chrc_unregister(attr++);
	descriptor_unregister(attr++);

	__ASSERT((attr - attr_start) == hids_obj->svc.attr_count,
		 "Failed to unregister full list of attributes.");

	/* Free all allocated memory. */
	ble_link_ctx_manager_free_all(hids_obj->ctx_manager);

	/* Reset HIDS instance. */
	memset(hids_obj, 0, sizeof(*hids_obj));
	hids_obj->svc.attrs = attr_start;
	hids_obj->ctx_manager = ctx_manager;

	return 0;
}


static int inp_rep_notify_all(struct hids *hids_obj,
			      struct hids_inp_rep *hids_inp_rep,
			      u8_t const *rep,
			      u8_t len)
{
	struct hids_conn_data *conn_data;
	u8_t *rep_data = NULL;

	for (size_t i = 0; i < ble_link_ctx_manager_get_ctx_num(hids_obj->ctx_manager); i++) {
		const struct ble_link_conn_ctx *conn_ctx =
				ble_link_ctx_manager_context_get(hids_obj->ctx_manager, i);

		if (conn_ctx) {
			bool notification_enabled =
					hids_is_notification_enabled(conn_ctx->conn,
								     hids_inp_rep->ccc);
			if (notification_enabled) {
				conn_data = (struct hids_conn_data *)conn_ctx->data;
				rep_data = conn_data->inp_rep_ctx + hids_inp_rep->offset;
				memcpy(rep_data, rep, len);
			}

			ble_link_ctx_manager_release(hids_obj->ctx_manager,
						     (void *)conn_ctx->data);
		}
	}

	if (rep_data != NULL) {
		return bt_gatt_notify(NULL,
			&hids_obj->svc.attrs[hids_inp_rep->att_ind],
			rep_data,
			hids_inp_rep->size);
	} else {
		return -ENODATA;
	}
}


int hids_inp_rep_send(struct hids *hids_obj, struct bt_conn *conn,
		      u8_t rep_index, u8_t const *rep,
		      u8_t len)
{
	struct hids_inp_rep *hids_inp_rep =
		&hids_obj->inp_rep_group.reports[rep_index];
	u8_t *rep_data;

	if (hids_inp_rep->size != len) {
		return -EINVAL;
	}

	if (!conn) {
		return inp_rep_notify_all(hids_obj,
					  hids_inp_rep,
					  rep,
					  len);
	}

	if (!hids_is_notification_enabled(conn, hids_inp_rep->ccc)) {
		return -EACCES;
	}

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_obj->ctx_manager,
									  conn);

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->inp_rep_ctx + hids_inp_rep->offset;

	memcpy(rep_data, rep, len);

	int err = bt_gatt_notify(conn,
				 &hids_obj->svc.attrs[hids_inp_rep->att_ind],
				 rep_data,
				 hids_inp_rep->size);

	ble_link_ctx_manager_release(hids_obj->ctx_manager,
				     (void *)conn_data);

	return err;
}


static int boot_mouse_inp_report_notify_all(struct hids *hids_obj,
					    const u8_t *buttons,
					    struct hids_boot_mouse_inp_rep *boot_mouse_inp_rep,
					    s8_t x_delta, s8_t y_delta)
{
	struct hids_conn_data *conn_data;
	u8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	u8_t *rep_data = NULL;

	for (size_t i = 0; i < ble_link_ctx_manager_get_ctx_num(hids_obj->ctx_manager); i++) {
		const struct ble_link_conn_ctx *conn_ctx =
				ble_link_ctx_manager_context_get(hids_obj->ctx_manager, i);

		if (conn_ctx) {
			bool notification_enabled =
					hids_is_notification_enabled(conn_ctx->conn,
								     boot_mouse_inp_rep->ccc);

			if (notification_enabled) {
				conn_data = (struct hids_conn_data *)conn_ctx->data;
				rep_data = conn_data->hids_boot_mouse_inp_rep_ctx;

				if (buttons) {
					/* If buttons data is not given
					 * use old values.
					 */
					rep_data[0] = *buttons;
				}
				rep_data[1] = (u8_t) x_delta;
				rep_data[2] = (u8_t) y_delta;
			}

			ble_link_ctx_manager_release(hids_obj->ctx_manager,
						     (void *)conn_ctx->data);
		}
	}

	if (rep_data != NULL) {
		return bt_gatt_notify(NULL,
			&hids_obj->svc.attrs[rep_ind],
			rep_data,
			sizeof(conn_data->hids_boot_mouse_inp_rep_ctx));
	} else {
		return -ENODATA;
	}
}


int hids_boot_mouse_inp_rep_send(struct hids *hids_obj,
				 struct bt_conn *conn,
				 const u8_t *buttons,
				 s8_t x_delta, s8_t y_delta)
{
	u8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct hids_boot_mouse_inp_rep *boot_mouse_inp_rep =
			&hids_obj->boot_mouse_inp_rep;
	u8_t *rep_data;

	if (!conn) {
		return boot_mouse_inp_report_notify_all(hids_obj,
							buttons,
							boot_mouse_inp_rep,
							x_delta, y_delta);
	}

	if (!hids_is_notification_enabled(conn, boot_mouse_inp_rep->ccc)) {
		return -EACCES;
	}

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_obj->ctx_manager,
									  conn);

	static_assert(sizeof(conn_data->hids_boot_mouse_inp_rep_ctx) >= 3,
			"buffer is too short");

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->hids_boot_mouse_inp_rep_ctx;

	if (buttons) {
		/* If buttons data is not given use old values. */
		rep_data[0] = *buttons;
	}
	rep_data[1] = (u8_t) x_delta;
	rep_data[2] = (u8_t) y_delta;

	int err = bt_gatt_notify(conn,
				 &hids_obj->svc.attrs[rep_ind],
				 rep_data,
				 sizeof(conn_data->hids_boot_mouse_inp_rep_ctx));

	ble_link_ctx_manager_release(hids_obj->ctx_manager,
				     (void *)conn_data);

	return err;
}


static int boot_kb_inp_notify_all(struct hids *hids_obj,
				  u8_t const *rep, u16_t len,
				  struct hids_boot_kb_inp_rep *boot_kb_input_report)
{
	struct hids_conn_data *conn_data;
	u8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	u8_t *rep_data = NULL;

	for (size_t i = 0; i < ble_link_ctx_manager_get_ctx_num(hids_obj->ctx_manager); i++) {
		const struct ble_link_conn_ctx *conn_ctx =
				ble_link_ctx_manager_context_get(hids_obj->ctx_manager, i);

		if (conn_ctx) {
			bool notification_enabled =
					hids_is_notification_enabled(conn_ctx->conn,
								     boot_kb_input_report->ccc);

			if (notification_enabled) {
				conn_data = (struct hids_conn_data *)conn_ctx->data;
				rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

				memcpy(rep_data, rep, len);
				memset(&rep_data[len], 0,
				       (BOOT_KB_INPUT_REP_CHAR_LEN - len));
			}

			ble_link_ctx_manager_release(hids_obj->ctx_manager,
						     (void *)conn_ctx->data);
		}
	}

	if (rep_data != NULL) {
		return bt_gatt_notify(NULL,
			&hids_obj->svc.attrs[rep_ind],
			rep_data,
			sizeof(conn_data->hids_boot_kb_inp_rep_ctx));
	} else {
		return -ENODATA;
	}
}


int hids_boot_kb_inp_rep_send(struct hids *hids_obj,
			      struct bt_conn *conn,
			      u8_t const *rep,
			      u16_t len)
{
	u8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	struct hids_boot_kb_inp_rep *boot_kb_input_report =
			&hids_obj->boot_kb_inp_rep;
	u8_t *rep_data = NULL;

	if (!conn) {
		return boot_kb_inp_notify_all(hids_obj, rep,
					      len,
					      boot_kb_input_report);
	}

	if (!hids_is_notification_enabled(conn, boot_kb_input_report->ccc)) {
		return -EACCES;
	}

	struct hids_conn_data *conn_data =
			(struct hids_conn_data *)ble_link_ctx_manager_get(hids_obj->ctx_manager,
									  conn);

	if (len > sizeof(conn_data->hids_boot_kb_inp_rep_ctx)) {
		return -EINVAL;
	}

	if (!conn_data) {
		SYS_LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

	memcpy(rep_data, rep, len);
	memset(&rep_data[len], 0,
	       (sizeof(conn_data->hids_boot_kb_inp_rep_ctx) - len));

	int err = bt_gatt_notify(conn,
				 &hids_obj->svc.attrs[rep_ind],
				 rep_data,
				 sizeof(conn_data->hids_boot_kb_inp_rep_ctx));

	ble_link_ctx_manager_release(hids_obj->ctx_manager,
				     (void *)conn_data);

	return err;
}
