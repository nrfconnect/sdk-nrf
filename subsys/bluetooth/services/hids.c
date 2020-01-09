/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <stddef.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <bluetooth/services/hids.h>

#include <logging/log.h>

#define BOOT_MOUSE_INPUT_REPORT_MIN_SIZE 3

#define GATT_PERM_READ_MASK     (BT_GATT_PERM_READ | \
				 BT_GATT_PERM_READ_ENCRYPT | \
				 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_WRITE_MASK    (BT_GATT_PERM_WRITE | \
				 BT_GATT_PERM_WRITE_ENCRYPT | \
				 BT_GATT_PERM_WRITE_AUTHEN)

#ifndef CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_AUTHEN
#define CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_AUTHEN 0
#endif
#ifndef CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_ENCRYPT
#define CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW
#define CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW 0
#endif

#define HIDS_GATT_PERM_DEFAULT ( \
	CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_AUTHEN ?                 \
	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN) :     \
	CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_ENCRYPT ?                \
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT) :   \
	(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)                     \
	)


LOG_MODULE_REGISTER(bt_gatt_hids, CONFIG_BT_GATT_HIDS_LOG_LEVEL);

int bt_gatt_hids_notify_connected(struct bt_gatt_hids *hids_obj,
				  struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(hids_obj != NULL);

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_alloc(hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("There is no free memory to "
			"allocate the connection context");
		return -ENOMEM;
	}

	memset(conn_data, 0, bt_conn_ctx_block_size_get(hids_obj->conn_ctx));

	conn_data->pm_ctx_value = BT_GATT_HIDS_PM_REPORT;

	/* Assign input report context. */
	conn_data->inp_rep_ctx =
	    (u8_t *)conn_data + sizeof(struct bt_gatt_hids_conn_data);

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

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return 0;
}

int bt_gatt_hids_notify_disconnected(struct bt_gatt_hids *hids_obj,
				     struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(hids_obj != NULL);

	int err = bt_conn_ctx_free(hids_obj->conn_ctx, conn);

	if (err) {
		LOG_WRN("The memory was not allocated for the context of this "
			"connection.");

		return err;
	}

	return 0;
}

static ssize_t hids_protocol_mode_write(struct bt_conn *conn,
					struct bt_gatt_attr const *attr,
					void const *buf, u16_t len,
					u16_t offset, u8_t flags)
{
	LOG_DBG("Writing to Protocol Mode characteristic.");

	struct bt_gatt_hids_pm_data *pm = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(pm, struct bt_gatt_hids, pm);
	u8_t const *new_pm = (u8_t const *)buf;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	u8_t *cur_pm = &conn_data->pm_ctx_value;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_pm) {
	case BT_GATT_HIDS_PM_BOOT:
		if (pm->evt_handler) {
			pm->evt_handler(BT_GATT_HIDS_PM_EVT_BOOT_MODE_ENTERED,
					conn);
		}
		break;
	case BT_GATT_HIDS_PM_REPORT:
		if (pm->evt_handler) {
			pm->evt_handler(BT_GATT_HIDS_PM_EVT_REPORT_MODE_ENTERED,
					conn);
		}
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(cur_pm + offset, new_pm, len);

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return len;
}

static ssize_t hids_protocol_mode_read(struct bt_conn *conn,
				       struct bt_gatt_attr const *attr,
				       void *buf, u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Protocol Mode characteristic.");

	struct bt_gatt_hids_pm_data *pm = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(pm, struct bt_gatt_hids, pm);
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	u8_t *protocol_mode = &conn_data->pm_ctx_value;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, protocol_mode,
				    sizeof(*protocol_mode));

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_report_map_read(struct bt_conn *conn,
				    struct bt_gatt_attr const *attr, void *buf,
				    u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Report Map characteristic.");

	struct bt_gatt_hids_rep_map *rep_map = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, rep_map->data,
				 rep_map->size);
}

static ssize_t hids_inp_rep_read(struct bt_conn *conn,
				 struct bt_gatt_attr const *attr, void *buf,
				 u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Input Report characteristic.");

	struct bt_gatt_hids_inp_rep *rep = attr->user_data;
	u8_t idx = rep->idx;
	struct bt_gatt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_gatt_hids,
						 inp_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->inp_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_inp_rep_ref_read(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr, void *buf,
				     u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Input Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_GATT_HIDS_REPORT_TYPE_INPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static ssize_t hids_outp_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr, void *buf,
				  u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Output Report characteristic.");

	struct bt_gatt_hids_outp_feat_rep *rep = attr->user_data;
	u8_t idx = rep->idx;
	struct bt_gatt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_gatt_hids,
						 outp_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = buf,
		    .size = rep->size,
		};
		rep->handler(&report, conn, false);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_outp_rep_write(struct bt_conn *conn,
				   struct bt_gatt_attr const *attr,
				   void const *buf, u16_t len, u16_t offset,
				   u8_t flags)
{
	LOG_DBG("Writing to Output Report characteristic.");

	struct bt_gatt_hids_outp_feat_rep *rep = attr->user_data;
	u8_t idx = rep->idx;
	struct bt_gatt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_gatt_hids,
						 outp_rep_group.reports);
	u8_t *rep_data;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = rep_data,
		    .size = rep->size,
		};
		rep->handler(&report, conn, true);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return len;
}

static ssize_t hids_outp_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Output Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_GATT_HIDS_REPORT_TYPE_OUTPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static ssize_t hids_feat_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr, void *buf,
				  u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Feature Report characteristic.");

	struct bt_gatt_hids_outp_feat_rep *rep = attr->user_data;
	u8_t idx = rep->idx;
	struct bt_gatt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_gatt_hids,
						 feat_rep_group.reports);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = buf,
		    .size = rep->size,
		};
		rep->handler(&report, conn, false);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_feat_rep_write(struct bt_conn *conn,
				   struct bt_gatt_attr const *attr,
				   void const *buf, u16_t len, u16_t offset,
				   u8_t flags)
{
	LOG_DBG("Writing to Feature Report characteristic.");

	struct bt_gatt_hids_outp_feat_rep *rep = attr->user_data;
	u8_t idx = rep->idx;
	struct bt_gatt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_gatt_hids,
						 feat_rep_group.reports);
	u8_t *rep_data;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = rep_data,
		    .size = rep->size,
		};
		rep->handler(&report, conn, true);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return len;
}

static ssize_t hids_feat_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Feature Report Reference descriptor.");

	u8_t report_ref[2];

	report_ref[0] = *((u8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_GATT_HIDS_REPORT_TYPE_FEATURE;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static void hids_input_report_ccc_changed(struct bt_gatt_attr const *attr,
					  u16_t value)
{
	LOG_DBG("Input Report CCCD has changed.");

	struct bt_gatt_hids_inp_rep *inp_rep =
	    CONTAINER_OF((struct _bt_gatt_ccc *)attr->user_data,
			 struct bt_gatt_hids_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification has been turned on");
		if (inp_rep->handler != NULL) {
			inp_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		LOG_DBG("Notification has been turned off");
		if (inp_rep->handler != NULL) {
			inp_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}

static ssize_t hids_boot_mouse_inp_report_read(struct bt_conn *conn,
					       struct bt_gatt_attr const *attr,
					       void *buf, u16_t len,
					       u16_t offset)
{
	LOG_DBG("Reading from Boot Mouse Input Report characteristic.");

	struct bt_gatt_hids_boot_mouse_inp_rep *rep = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(rep, struct bt_gatt_hids,
						 boot_mouse_inp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_mouse_inp_rep_ctx;

	ret_len =
	    bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
			      sizeof(conn_data->hids_boot_mouse_inp_rep_ctx));
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static void hids_boot_mouse_inp_rep_ccc_changed(struct bt_gatt_attr const *attr,
						u16_t value)
{
	LOG_DBG("Boot Mouse Input Report CCCD has changed.");

	struct bt_gatt_hids_boot_mouse_inp_rep *boot_mouse_rep =
	    CONTAINER_OF((struct _bt_gatt_ccc *)attr->user_data,
			 struct bt_gatt_hids_boot_mouse_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for Boot Mouse has been turned on");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		LOG_DBG("Notification for Boot Mouse has been turned off");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}

static ssize_t hids_boot_kb_inp_report_read(struct bt_conn *conn,
					    struct bt_gatt_attr const *attr,
					    void *buf, u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Boot Keyboard Input Report characteristic.");

	struct bt_gatt_hids_boot_kb_inp_rep *rep = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(rep, struct bt_gatt_hids,
						 boot_kb_inp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

	ret_len =
	    bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
			      sizeof(conn_data->hids_boot_kb_inp_rep_ctx));
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static void hids_boot_kb_inp_rep_ccc_changed(struct bt_gatt_attr const *attr,
					     u16_t value)
{
	LOG_DBG("Boot Keyboard Input Report CCCD has changed.");

	struct bt_gatt_hids_boot_kb_inp_rep *boot_kb_inp_rep =
	    CONTAINER_OF((struct _bt_gatt_ccc *)attr->user_data,
			 struct bt_gatt_hids_boot_kb_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for Boot Keyboard has been turned "
			"on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED);
		}
	} else {
		LOG_DBG("Notification for Boot Keyboard has been turned "
			"on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED);
		}
	}
}

static ssize_t hids_boot_kb_outp_report_read(struct bt_conn *conn,
					     struct bt_gatt_attr const *attr,
					     void *buf, u16_t len, u16_t offset)
{
	LOG_DBG("Reading from Boot Keyboard Output Report characteristic.");

	struct bt_gatt_hids_boot_kb_outp_rep *rep = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(rep, struct bt_gatt_hids,
						 boot_kb_outp_rep);
	u8_t *rep_data;
	ssize_t ret_len;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_outp_rep_ctx;

	ret_len =
	    bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
			      sizeof(conn_data->hids_boot_kb_outp_rep_ctx));

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = buf,
		};
		rep->handler(&report, conn, false);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_boot_kb_outp_report_write(struct bt_conn *conn,
					      struct bt_gatt_attr const *attr,
					      void const *buf, u16_t len,
					      u16_t offset, u8_t flags)
{
	LOG_DBG("Writing to Boot Keyboard Output Report characteristic.");

	struct bt_gatt_hids_boot_kb_outp_rep *rep = attr->user_data;
	struct bt_gatt_hids *hids = CONTAINER_OF(rep, struct bt_gatt_hids,
						 boot_kb_outp_rep);
	u8_t *rep_data;

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_outp_rep_ctx;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_gatt_hids_rep report = {
		    .data = rep_data,
		    .size = sizeof(conn_data->hids_boot_kb_outp_rep_ctx),
		};

		rep->handler(&report, conn, true);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return len;
}

static ssize_t hids_info_read(struct bt_conn *conn,
			      struct bt_gatt_attr const *attr, void *buf,
			      u16_t len, u16_t offset)
{
	LOG_DBG("Reading from HID information characteristic.");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 BT_GATT_HIDS_INFORMATION_LEN);
}

static ssize_t hids_ctrl_point_write(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr,
				     void const *buf, u16_t len, u16_t offset,
				     u8_t flags)
{
	LOG_DBG("Writing to Control Point characteristic.");

	struct bt_gatt_hids_cp *cp = (struct bt_gatt_hids_cp *)attr->user_data;

	u8_t cur_cp = cp->value;
	u8_t const *new_cp = (u8_t const *)buf;

	if (offset + len > sizeof(u8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (*new_cp) {
	case BT_GATT_HIDS_CONTROL_POINT_SUSPEND:
		if (cp->evt_handler) {
			cp->evt_handler(BT_GATT_HIDS_CP_EVT_HOST_SUSP);
		}
		break;
	case BT_GATT_HIDS_CONTROL_POINT_EXIT_SUSPEND:
		if (cp->evt_handler) {
			cp->evt_handler(BT_GATT_HIDS_CP_EVT_HOST_EXIT_SUSP);
		}
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	memcpy(&cur_cp + offset, new_cp, len);
	return len;
}

static u8_t hid_information_encode(u8_t *buffer,
				   struct bt_gatt_hids_info const *hid_info)
{
	u8_t len = 0;

	sys_put_le16(hid_info->bcd_hid, buffer);
	len += sizeof(u16_t);

	buffer[len++] = hid_info->b_country_code;
	buffer[len++] = hid_info->flags;

	__ASSERT(len == BT_GATT_HIDS_INFORMATION_LEN,
		 "HIDS Information encoding failed");
	return len;
}

static void
hids_input_reports_register(struct bt_gatt_hids *hids_obj,
			    const struct bt_gatt_hids_init_param *init_param)
{
	__ASSERT_NO_MSG(init_param->inp_rep_group_init.cnt <=
			CONFIG_BT_GATT_HIDS_INPUT_REP_MAX);
	u8_t offset = 0;

	memcpy(&hids_obj->inp_rep_group, &init_param->inp_rep_group_init,
	       sizeof(hids_obj->inp_rep_group));

	for (size_t i = 0; i < hids_obj->inp_rep_group.cnt; i++) {
		struct bt_gatt_hids_inp_rep *hids_inp_rep =
		    &hids_obj->inp_rep_group.reports[i];
		u8_t rperm = hids_inp_rep->perm & GATT_PERM_READ_MASK;
		u8_t wperm;

		if (rperm == 0) {
			rperm = HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK;
		}
		wperm = ((rperm & BT_GATT_PERM_READ) ?
				BT_GATT_PERM_WRITE : 0) |
			((rperm & BT_GATT_PERM_READ_ENCRYPT) ?
				BT_GATT_PERM_WRITE_ENCRYPT : 0) |
			((rperm & BT_GATT_PERM_READ_AUTHEN) ?
				BT_GATT_PERM_WRITE_AUTHEN : 0);

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				  rperm, hids_inp_rep_read, NULL,
				  hids_inp_rep);

		hids_inp_rep->att_ind = hids_obj->gp.svc.attr_count - 1;
		hids_inp_rep->offset = offset;
		hids_inp_rep->idx = i;

		BT_GATT_POOL_CCC(&hids_obj->gp, hids_inp_rep->ccc,
				 hids_input_report_ccc_changed,  wperm | rperm);
		BT_GATT_POOL_DESC(&hids_obj->gp, BT_UUID_HIDS_REPORT_REF,
				  rperm, hids_inp_rep_ref_read,
				  NULL, &hids_inp_rep->id);
		offset += hids_inp_rep->size;
	}
}

static void hids_outp_reports_register(struct bt_gatt_hids *hids_obj,
				       const struct bt_gatt_hids_init_param *init_param)
{
	__ASSERT_NO_MSG(init_param->outp_rep_group_init.cnt <=
			CONFIG_BT_GATT_HIDS_OUTPUT_REP_MAX);

	u8_t offset = 0;

	memcpy(&hids_obj->outp_rep_group, &init_param->outp_rep_group_init,
	       sizeof(hids_obj->outp_rep_group));

	for (size_t i = 0; i < hids_obj->outp_rep_group.cnt; i++) {
		struct bt_gatt_hids_outp_feat_rep *hids_outp_rep =
		    &hids_obj->outp_rep_group.reports[i];
		u8_t perm = hids_outp_rep->perm;

		if (0 == (perm & GATT_PERM_READ_MASK)) {
			perm |= HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK;
		}
		if (0 == (perm & GATT_PERM_WRITE_MASK)) {
			perm |= HIDS_GATT_PERM_DEFAULT & GATT_PERM_WRITE_MASK;
		}

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				  BT_GATT_CHRC_WRITE_WITHOUT_RESP, perm,
				  hids_outp_rep_read, hids_outp_rep_write,
				  hids_outp_rep);

		hids_outp_rep->att_ind = hids_obj->gp.svc.attr_count - 1;
		hids_outp_rep->offset = offset;
		hids_outp_rep->idx = i;

		BT_GATT_POOL_DESC(&hids_obj->gp, BT_UUID_HIDS_REPORT_REF,
				  perm & GATT_PERM_READ_MASK,
				  hids_outp_rep_ref_read,
				  NULL, &hids_outp_rep->id);

		offset += hids_outp_rep->size;
	}
}

static void hids_feat_reports_register(struct bt_gatt_hids *hids_obj,
				       const struct bt_gatt_hids_init_param *init_param)
{
	__ASSERT_NO_MSG(init_param->feat_rep_group_init.cnt <=
			CONFIG_BT_GATT_HIDS_FEATURE_REP_MAX);

	memcpy(&hids_obj->feat_rep_group, &init_param->feat_rep_group_init,
	       sizeof(hids_obj->feat_rep_group));

	u8_t offset = 0;

	for (size_t i = 0; i < hids_obj->feat_rep_group.cnt; i++) {
		struct bt_gatt_hids_outp_feat_rep *hids_feat_rep =
		    &hids_obj->feat_rep_group.reports[i];
		u8_t perm = hids_feat_rep->perm;

		if (0 == (perm & GATT_PERM_READ_MASK)) {
			perm |= HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK;
		}
		if (0 == (perm & GATT_PERM_WRITE_MASK)) {
			perm |= HIDS_GATT_PERM_DEFAULT & GATT_PERM_WRITE_MASK;
		}

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, perm,
				  hids_feat_rep_read, hids_feat_rep_write,
				  hids_feat_rep);

		hids_feat_rep->att_ind = hids_obj->gp.svc.attr_count - 1;
		hids_feat_rep->offset = offset;
		hids_feat_rep->idx = i;

		BT_GATT_POOL_DESC(&hids_obj->gp, BT_UUID_HIDS_REPORT_REF,
				  perm & GATT_PERM_READ_MASK,
				  hids_feat_rep_ref_read,
				  NULL, &hids_feat_rep->id);

		offset += hids_feat_rep->size;
	}
}

int bt_gatt_hids_init(struct bt_gatt_hids *hids_obj,
		      const struct bt_gatt_hids_init_param *init_param)
{
	LOG_DBG("Initializing HIDS.");

	hids_obj->pm.evt_handler = init_param->pm_evt_handler;
	hids_obj->cp.evt_handler = init_param->cp_evt_handler;

	/* Register primary service. */
	BT_GATT_POOL_SVC(&hids_obj->gp, BT_UUID_HIDS);

	/* Register Protocol Mode characteristic. */
	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_PROTOCOL_MODE,
			  BT_GATT_CHRC_READ |
			  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			  HIDS_GATT_PERM_DEFAULT,
			  hids_protocol_mode_read, hids_protocol_mode_write,
			  &hids_obj->pm);

	/* Register Input Report characteristics. */
	hids_input_reports_register(hids_obj, init_param);

	/* Register Output Report characteristics. */
	hids_outp_reports_register(hids_obj, init_param);

	/* Register Feature Report characteristics. */
	hids_feat_reports_register(hids_obj, init_param);

	/* Register Report Map characteristic and its descriptor. */
	memcpy(&hids_obj->rep_map, &init_param->rep_map,
	       sizeof(hids_obj->rep_map));

	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_REPORT_MAP,
			  BT_GATT_CHRC_READ,
			  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			  hids_report_map_read, NULL, &hids_obj->rep_map);

	/* Register HID Boot Mouse Input Report characteristic, its descriptor
	 * and CCC.
	 */
	if (init_param->is_mouse) {
		hids_obj->is_mouse = init_param->is_mouse;

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
				  hids_boot_mouse_inp_report_read, NULL,
				  &hids_obj->boot_mouse_inp_rep);

		hids_obj->boot_mouse_inp_rep.att_ind =
			hids_obj->gp.svc.attr_count - 1;
		hids_obj->boot_mouse_inp_rep.handler =
			init_param->boot_mouse_notif_handler;

		BT_GATT_POOL_CCC(&hids_obj->gp,
				 hids_obj->boot_mouse_inp_rep.ccc,
				 hids_boot_mouse_inp_rep_ccc_changed,
				 HIDS_GATT_PERM_DEFAULT);
	}

	/* Register HID Boot Keyboard Input/Output Report characteristic, its
	 * descriptor and CCC.
	 */
	if (init_param->is_kb) {
		hids_obj->is_kb = init_param->is_kb;

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_BOOT_KB_IN_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
				  hids_boot_kb_inp_report_read, NULL,
				  &hids_obj->boot_kb_inp_rep);

		hids_obj->boot_kb_inp_rep.att_ind =
			hids_obj->gp.svc.attr_count - 1;
		hids_obj->boot_kb_inp_rep.handler =
		    init_param->boot_kb_notif_handler;

		BT_GATT_POOL_CCC(&hids_obj->gp,
				 hids_obj->boot_kb_inp_rep.ccc,
				 hids_boot_kb_inp_rep_ccc_changed,
				 HIDS_GATT_PERM_DEFAULT);

		BT_GATT_POOL_CHRC(&hids_obj->gp,
				  BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
				  HIDS_GATT_PERM_DEFAULT,
				  hids_boot_kb_outp_report_read,
				  hids_boot_kb_outp_report_write,
				  &hids_obj->boot_kb_outp_rep);

		hids_obj->boot_kb_outp_rep.att_ind =
			hids_obj->gp.svc.attr_count - 1;
		hids_obj->boot_kb_outp_rep.handler =
			init_param->boot_kb_outp_rep_handler;
	}

	/* Register HID Information characteristic and its descriptor. */
	hid_information_encode(hids_obj->info, &init_param->info);

	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_INFO,
			  BT_GATT_CHRC_READ,
			  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			  hids_info_read, NULL, hids_obj->info);

	/* Register HID Control Point characteristic and its descriptor. */
	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_CTRL_POINT,
			  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			  HIDS_GATT_PERM_DEFAULT | GATT_PERM_WRITE_MASK,
			  NULL, hids_ctrl_point_write, &hids_obj->cp);

	/* Register HIDS attributes in GATT database. */
	return bt_gatt_service_register(&hids_obj->gp.svc);
}

int bt_gatt_hids_uninit(struct bt_gatt_hids *hids_obj)
{
	int err;

	/* Unregister HIDS attributes in GATT database. */
	err = bt_gatt_service_unregister(&hids_obj->gp.svc);
	if (err) {
		return err;
	}

	struct bt_gatt_attr *attr_start = hids_obj->gp.svc.attrs;
	struct bt_conn_ctx_lib *conn_ctx = hids_obj->conn_ctx;

	/* Free the whole GATT pool */
	bt_gatt_pool_free(&hids_obj->gp);

	/* Free all allocated memory. */
	bt_conn_ctx_free_all(hids_obj->conn_ctx);

	/* Reset HIDS instance. */
	memset(hids_obj, 0, sizeof(*hids_obj));
	hids_obj->gp.svc.attrs = attr_start;
	hids_obj->conn_ctx = conn_ctx;

	return 0;
}

static void store_input_report(struct bt_gatt_hids_inp_rep *hids_inp_rep,
			       u8_t *rep_data, u8_t const *rep, u8_t len)
{
	if (!hids_inp_rep->rep_mask) {
		memcpy(rep_data, rep, len);
		return;
	}

	const u8_t *rep_mask = hids_inp_rep->rep_mask;

	for (size_t i = 0; i < len; i++) {
		if ((rep_mask[i / 8] & BIT(i % 8)) != 0) {
			rep_data[i] = rep[i];
		}
	}
}

static int inp_rep_notify_all(struct bt_gatt_hids *hids_obj,
			      struct bt_gatt_hids_inp_rep *hids_inp_rep,
			      u8_t const *rep, u8_t len,
			      bt_gatt_complete_func_t cb)
{
	struct bt_gatt_hids_conn_data *conn_data;
	u8_t *rep_data = NULL;
	struct bt_gatt_attr *rep_attr =
		&hids_obj->gp.svc.attrs[hids_inp_rep->att_ind];

	const size_t contexts =
	    bt_conn_ctx_count(hids_obj->conn_ctx);

	for (size_t i = 0; i < contexts; i++) {
		const struct bt_conn_ctx *ctx =
			bt_conn_ctx_get_by_id(hids_obj->conn_ctx, i);

		if (ctx) {
			bool notification_enabled = bt_gatt_is_subscribed(
				ctx->conn, rep_attr, BT_GATT_CCC_NOTIFY);

			if (notification_enabled) {
				conn_data = ctx->data;
				rep_data = conn_data->inp_rep_ctx +
					   hids_inp_rep->offset;

				store_input_report(hids_inp_rep, rep_data, rep,
						   len);
			}

			bt_conn_ctx_release(hids_obj->conn_ctx,
					    (void *)ctx->data);
		}
	}

	if (rep_data != NULL) {
		struct bt_gatt_notify_params params = {0};

		params.attr = rep_attr;
		params.data = rep;
		params.len = hids_inp_rep->size;
		params.func = cb;

		return bt_gatt_notify_cb(NULL, &params);
	} else {
		return -ENODATA;
	}
}

int bt_gatt_hids_inp_rep_send(struct bt_gatt_hids *hids_obj,
			      struct bt_conn *conn, u8_t rep_index,
			      u8_t const *rep, u8_t len,
			      bt_gatt_complete_func_t cb)
{
	struct bt_gatt_hids_inp_rep *hids_inp_rep =
	    &hids_obj->inp_rep_group.reports[rep_index];
	struct bt_gatt_attr *rep_attr =
	    &hids_obj->gp.svc.attrs[hids_inp_rep->att_ind];
	u8_t *rep_data;

	if (hids_inp_rep->size != len) {
		return -EINVAL;
	}

	if (!conn) {
		return inp_rep_notify_all(hids_obj, hids_inp_rep, rep, len, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->inp_rep_ctx + hids_inp_rep->offset;

	store_input_report(hids_inp_rep, rep_data, rep, len);

	struct bt_gatt_notify_params params = {0};

	params.attr = &hids_obj->gp.svc.attrs[hids_inp_rep->att_ind];
	params.data = rep;
	params.len = hids_inp_rep->size;
	params.func = cb;

	int err = bt_gatt_notify_cb(conn, &params);

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return err;
}

static int boot_mouse_inp_report_notify_all(
	struct bt_gatt_hids *hids_obj, const u8_t *buttons,
	struct bt_gatt_hids_boot_mouse_inp_rep *boot_mouse_inp_rep,
	s8_t x_delta, s8_t y_delta, bt_gatt_complete_func_t cb)
{
	struct bt_gatt_hids_conn_data *conn_data;
	u8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	u8_t *rep_data = NULL;
	u8_t rep_buff[BT_GATT_HIDS_BOOT_MOUSE_REP_LEN] = {0};

	rep_buff[1] = (u8_t)x_delta;
	rep_buff[2] = (u8_t)y_delta;

	const size_t contexts = bt_conn_ctx_count(hids_obj->conn_ctx);

	for (size_t i = 0; i < contexts; i++) {
		const struct bt_conn_ctx *ctx =
			bt_conn_ctx_get_by_id(hids_obj->conn_ctx, i);

		if (ctx) {
			bool notification_enabled = bt_gatt_is_subscribed(
				ctx->conn, rep_attr, BT_GATT_CCC_NOTIFY);

			if (notification_enabled) {
				conn_data = ctx->data;
				rep_data =
				    conn_data->hids_boot_mouse_inp_rep_ctx;

				if (buttons) {
					/* If buttons data is not given
					 * use old values.
					 */
					rep_data[0] = *buttons;
				}

				rep_buff[0] = rep_data[0];
			}

			bt_conn_ctx_release(hids_obj->conn_ctx,
					    (void *)ctx->data);
		}
	}

	if (rep_data != NULL) {
		struct bt_gatt_notify_params params = {0};

		params.attr = rep_attr;
		params.data = rep_buff;
		params.len = sizeof(conn_data->hids_boot_mouse_inp_rep_ctx);
		params.func = cb;

		return bt_gatt_notify_cb(NULL, &params);
	} else {
		return -ENODATA;
	}
}

int bt_gatt_hids_boot_mouse_inp_rep_send(struct bt_gatt_hids *hids_obj,
					 struct bt_conn *conn,
					 const u8_t *buttons,
					 s8_t x_delta, s8_t y_delta,
					 bt_gatt_complete_func_t cb)
{
	u8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct bt_gatt_hids_boot_mouse_inp_rep *boot_mouse_inp_rep =
	    &hids_obj->boot_mouse_inp_rep;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	u8_t *rep_data;

	if (!conn) {
		return boot_mouse_inp_report_notify_all(hids_obj, buttons,
							boot_mouse_inp_rep,
							x_delta, y_delta, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids_obj->conn_ctx, conn);

	BUILD_ASSERT_MSG(sizeof(conn_data->hids_boot_mouse_inp_rep_ctx) >= 3,
			 "buffer is too short");

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->hids_boot_mouse_inp_rep_ctx;

	if (buttons) {
		/* If buttons data is not given use old values. */
		rep_data[0] = *buttons;
	}
	rep_data[1] = (u8_t)x_delta;
	rep_data[2] = (u8_t)y_delta;

	struct bt_gatt_notify_params params = {0};

	params.attr = &hids_obj->gp.svc.attrs[rep_ind];
	params.data = rep_data;
	params.len = sizeof(conn_data->hids_boot_mouse_inp_rep_ctx);
	params.func = cb;

	int err = bt_gatt_notify_cb(conn, &params);

	rep_data[1] = 0;
	rep_data[2] = 0;

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return err;
}

static int
boot_kb_inp_notify_all(struct bt_gatt_hids *hids_obj, u8_t const *rep,
		       u16_t len,
		       struct bt_gatt_hids_boot_kb_inp_rep *boot_kb_inp_rep,
		       bt_gatt_complete_func_t cb)
{
	struct bt_gatt_hids_conn_data *conn_data;
	u8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	u8_t *rep_data = NULL;

	const size_t contexts = bt_conn_ctx_count(hids_obj->conn_ctx);

	for (size_t i = 0; i < contexts; i++) {
		const struct bt_conn_ctx *ctx =
		    bt_conn_ctx_get_by_id(hids_obj->conn_ctx, i);

		if (ctx) {
			bool notification_enabled = bt_gatt_is_subscribed(
				ctx->conn, rep_attr, BT_GATT_CCC_NOTIFY);

			if (notification_enabled) {
				conn_data = ctx->data;
				rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

				memcpy(rep_data, rep, len);
				memset(&rep_data[len], 0,
				       (BT_GATT_HIDS_BOOT_KB_INPUT_REP_LEN -
					len));
			}

			bt_conn_ctx_release(hids_obj->conn_ctx,
					    (void *)ctx->data);
		}
	}

	if (rep_data != NULL) {
		struct bt_gatt_notify_params params = {0};

		params.attr = rep_attr;
		params.data = rep_data;
		params.len = sizeof(conn_data->hids_boot_kb_inp_rep_ctx);
		params.func = cb;

		return bt_gatt_notify_cb(NULL, &params);
	} else {
		return -ENODATA;
	}
}

int bt_gatt_hids_boot_kb_inp_rep_send(struct bt_gatt_hids *hids_obj,
				      struct bt_conn *conn, u8_t const *rep,
				      u16_t len, bt_gatt_complete_func_t cb)
{
	u8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	struct bt_gatt_hids_boot_kb_inp_rep *boot_kb_input_report =
	    &hids_obj->boot_kb_inp_rep;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	u8_t *rep_data = NULL;

	if (!conn) {
		return boot_kb_inp_notify_all(hids_obj, rep, len,
					      boot_kb_input_report, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_gatt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids_obj->conn_ctx, conn);

	if (len > sizeof(conn_data->hids_boot_kb_inp_rep_ctx)) {
		return -EINVAL;
	}

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return -EINVAL;
	}

	rep_data = conn_data->hids_boot_kb_inp_rep_ctx;

	memcpy(rep_data, rep, len);
	memset(&rep_data[len], 0,
	       (sizeof(conn_data->hids_boot_kb_inp_rep_ctx) - len));

	struct bt_gatt_notify_params params = {0};

	params.attr = rep_attr;
	params.data = rep_data;
	params.len = sizeof(conn_data->hids_boot_kb_inp_rep_ctx);
	params.func = cb;

	int err = bt_gatt_notify_cb(conn, &params);

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return err;
}
