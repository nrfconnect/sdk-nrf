/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>

#include <bluetooth/services/hids.h>

#include <zephyr/logging/log.h>

#define GATT_PERM_READ_MASK     (BT_GATT_PERM_READ | \
				 BT_GATT_PERM_READ_ENCRYPT | \
				 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_WRITE_MASK    (BT_GATT_PERM_WRITE | \
				 BT_GATT_PERM_WRITE_ENCRYPT | \
				 BT_GATT_PERM_WRITE_AUTHEN)

#ifndef CONFIG_BT_HIDS_DEFAULT_PERM_RW_AUTHEN
#define CONFIG_BT_HIDS_DEFAULT_PERM_RW_AUTHEN 0
#endif
#ifndef CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT
#define CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_BT_HIDS_DEFAULT_PERM_RW
#define CONFIG_BT_HIDS_DEFAULT_PERM_RW 0
#endif

#define HIDS_GATT_PERM_DEFAULT ( \
	CONFIG_BT_HIDS_DEFAULT_PERM_RW_AUTHEN ?                 \
	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN) :     \
	CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT ?                \
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT) :   \
	(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)                     \
	)

#if defined(CONFIG_BT_HIDS_SCI)

/** Expands to CONFIG_BT_HIDS_SCI_{mode}{suffix} via token pasting.
 *  Example: CONFIG_BT_HIDS_SCI_DEFAULT_INTERVAL_MIN_125US.
 */
#define HIDS_SCI_KCONFIG(mode, suffix) CONFIG_BT_HIDS_SCI_##mode##suffix

/** Expands to a static const struct bt_conn_le_conn_rate_param for the given mode.
 *  BUILD_ASSERTs mirror Zephyr @c le_conn_rate_common_params_valid() in
 *  @c conn.c (Core 6.2 Vol 4 Part E §7.8.154) plus Link Layer supervision pacing
 *  (Vol 6 Part B §§4.5.1–4.5.2).
 */
#define HIDS_SCI_DEFINE_CONN_RATE_PARAM(name, mode) \
	BUILD_ASSERT( \
		HIDS_SCI_KCONFIG(mode, _SUBRATE_MAX) * \
		(1 + HIDS_SCI_KCONFIG(mode, _MAX_LATENCY)) \
		<= 500, \
		"HIDS SCI " #mode ": Core connSubrate*(latency+1) must be <= 500"); \
	BUILD_ASSERT(HIDS_SCI_KCONFIG(mode, _CONTINUATION_NUM) \
		< HIDS_SCI_KCONFIG(mode, _SUBRATE_MAX), \
		"HIDS SCI " #mode ": continuation number must be less than subrate maximum"); \
	BUILD_ASSERT( \
		HIDS_SCI_KCONFIG(mode, _SUPERVISION_TIMEOUT_10MS) * 10000 > \
		(uint64_t)(1 + HIDS_SCI_KCONFIG(mode, _MAX_LATENCY)) * \
		HIDS_SCI_KCONFIG(mode, _SUBRATE_MAX) * \
		HIDS_SCI_KCONFIG(mode, _INTERVAL_MAX_125US) * 250, \
		"HIDS SCI " #mode ": supervision timeout must exceed " \
		"2*(1+latency)*subrate*interval (Vol 6 Part B §4.5.2)"); \
	static const struct bt_conn_le_conn_rate_param (name) = { \
		.interval_min_125us = (uint16_t)HIDS_SCI_KCONFIG(mode, _INTERVAL_MIN_125US), \
		.interval_max_125us = (uint16_t)HIDS_SCI_KCONFIG(mode, _INTERVAL_MAX_125US), \
		.subrate_min = (uint16_t)HIDS_SCI_KCONFIG(mode, _SUBRATE_MIN), \
		.subrate_max = (uint16_t)HIDS_SCI_KCONFIG(mode, _SUBRATE_MAX), \
		.max_latency = (uint16_t)HIDS_SCI_KCONFIG(mode, _MAX_LATENCY), \
		.continuation_number = (uint16_t)HIDS_SCI_KCONFIG( \
			mode, _CONTINUATION_NUM), \
		.supervision_timeout_10ms = (uint16_t)HIDS_SCI_KCONFIG( \
			mode, _SUPERVISION_TIMEOUT_10MS), \
		.min_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MIN_125US, \
		.max_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MAX_125US, \
	}

/* Required by HID over GATT Profile specification (chapter 7.4.1) */
BUILD_ASSERT(CONFIG_BT_HIDS_SCI_FAST_INTERVAL_MAX_125US < (7500 / 125),
	     "HID SCI FAST max conn interval must be less than 7.5 ms");

/* Required by HID over GATT Profile specification (chapter 7.4.1) */
BUILD_ASSERT(CONFIG_BT_HIDS_SCI_FULL_RANGE_INTERVAL_MIN_125US ==
	     CONFIG_BT_HIDS_SCI_FAST_INTERVAL_MIN_125US,
	     "HID SCI FULL_RANGE min conn interval must equal FAST min conn interval");

HIDS_SCI_DEFINE_CONN_RATE_PARAM(hids_sci_conn_rate_default, DEFAULT);
HIDS_SCI_DEFINE_CONN_RATE_PARAM(hids_sci_conn_rate_fast, FAST);
#if defined(CONFIG_BT_HIDS_SCI_LOW_POWER_MODE)
HIDS_SCI_DEFINE_CONN_RATE_PARAM(hids_sci_conn_rate_low_power, LOW_POWER);
#endif
HIDS_SCI_DEFINE_CONN_RATE_PARAM(hids_sci_conn_rate_full_range, FULL_RANGE);

/* TODO: Currently only a single HID service supporting SCI is supported.
 * Fix this in NCSDK-38984
 *
 * A pointer to the HID service is stored to be able to notify the service about
 * SCI mode changes and make sure that only one SCI-supporting HID service is present.
 */
static struct bt_hids *sci_mode_hids_obj;
#endif /* CONFIG_BT_HIDS_SCI */

LOG_MODULE_REGISTER(bt_hids, CONFIG_BT_HIDS_LOG_LEVEL);

int bt_hids_connected(struct bt_hids *hids_obj, struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(hids_obj != NULL);

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_alloc(hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("There is no free memory to "
			"allocate the connection context");
		return -ENOMEM;
	}

	memset(conn_data, 0, bt_conn_ctx_block_size_get(hids_obj->conn_ctx));

	conn_data->pm_ctx_value = BT_HIDS_PM_REPORT;

	/* Assign input report context. */
	conn_data->inp_rep_ctx =
		(uint8_t *)conn_data + sizeof(struct bt_hids_conn_data);

	/* Assign output report context. */
	conn_data->outp_rep_ctx = conn_data->inp_rep_ctx;

	size_t cnt = MIN(hids_obj->inp_rep_group.cnt, ARRAY_SIZE(hids_obj->inp_rep_group.reports));

	for (size_t i = 0; i < cnt; i++) {
		conn_data->outp_rep_ctx +=
		    hids_obj->inp_rep_group.reports[i].size;
	}

	/* Assign feature report context. */
	conn_data->feat_rep_ctx = conn_data->outp_rep_ctx;
	cnt = MIN(hids_obj->outp_rep_group.cnt, ARRAY_SIZE(hids_obj->outp_rep_group.reports));

	for (size_t i = 0; i < cnt; i++) {
		conn_data->feat_rep_ctx +=
		    hids_obj->outp_rep_group.reports[i].size;
	}

#if defined(CONFIG_BT_HIDS_SCI)
	conn_data->sci_mode = BT_HIDS_SCI_MODE_NONE;
#endif

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return 0;
}

int bt_hids_disconnected(struct bt_hids *hids_obj, struct bt_conn *conn)
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
					void const *buf, uint16_t len,
					uint16_t offset, uint8_t flags)
{
	ssize_t ret = len;
	LOG_DBG("Writing to Protocol Mode characteristic.");

	struct bt_hids_pm_data *pm = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(pm, struct bt_hids, pm);
	uint8_t const *new_pm = (uint8_t const *)buf;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	uint8_t *cur_pm = &conn_data->pm_ctx_value;

	if (offset > 0) {
		ret = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto release_ctx;
	}

	if (len > sizeof(uint8_t)) {
		ret = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		goto release_ctx;
	}

	switch (*new_pm) {
	case BT_HIDS_PM_BOOT:
		if (pm->evt_handler) {
			pm->evt_handler(BT_HIDS_PM_EVT_BOOT_MODE_ENTERED,
					conn);
		}
		break;
	case BT_HIDS_PM_REPORT:
		if (pm->evt_handler) {
			pm->evt_handler(BT_HIDS_PM_EVT_REPORT_MODE_ENTERED,
					conn);
		}
		break;
	default:
		ret = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
		goto release_ctx;
	}

	memcpy(cur_pm + offset, new_pm, len);
release_ctx:
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret;
}

static ssize_t hids_protocol_mode_read(struct bt_conn *conn,
				       struct bt_gatt_attr const *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Protocol Mode characteristic.");

	struct bt_hids_pm_data *pm = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(pm, struct bt_hids, pm);
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	uint8_t *protocol_mode = &conn_data->pm_ctx_value;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, protocol_mode,
				    sizeof(*protocol_mode));

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_report_map_read(struct bt_conn *conn,
				    struct bt_gatt_attr const *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Report Map characteristic.");

	struct bt_hids_rep_map *rep_map = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, rep_map->data,
				 rep_map->size);
}

static ssize_t hids_inp_rep_read(struct bt_conn *conn,
				 struct bt_gatt_attr const *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Input Report characteristic.");

	struct bt_hids_inp_rep *rep = attr->user_data;
	uint8_t idx = rep->idx;
	struct bt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_hids,
						 inp_rep_group.reports[0]);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
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
				     uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Input Report Reference descriptor.");

	uint8_t report_ref[2];

	report_ref[0] = *((uint8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_HIDS_REPORT_TYPE_INPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static ssize_t hids_outp_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Output Report characteristic.");

	struct bt_hids_outp_feat_rep *rep = attr->user_data;
	uint8_t idx = rep->idx;
	struct bt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_hids,
						 outp_rep_group.reports[0]);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	if (rep->handler) {
		struct bt_hids_rep report = {
		    .id = rep->id,
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
				   void const *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	ssize_t ret = len;
	LOG_DBG("Writing to Output Report characteristic.");

	struct bt_hids_outp_feat_rep *rep = attr->user_data;
	uint8_t idx = rep->idx;
	struct bt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_hids,
						 outp_rep_group.reports[0]);
	uint8_t *rep_data;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->outp_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		ret = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto release_ctx;
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_hids_rep report = {
		    .id = rep->id,
		    .data = rep_data,
		    .size = rep->size,
		};
		rep->handler(&report, conn, true);
	}
release_ctx:
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret;
}

static ssize_t hids_outp_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Output Report Reference descriptor.");

	uint8_t report_ref[2];

	report_ref[0] = *((uint8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_HIDS_REPORT_TYPE_OUTPUT;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static ssize_t hids_feat_rep_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Feature Report characteristic.");

	struct bt_hids_outp_feat_rep *rep = attr->user_data;
	uint8_t idx = rep->idx;
	struct bt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_hids,
						 feat_rep_group.reports[0]);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, rep_data,
				    rep->size);

	if (rep->handler) {
		struct bt_hids_rep report = {
		    .id = rep->id,
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
				   void const *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	ssize_t ret = len;
	/* Write command operation is not allowed for this characteristic. */
	if (flags & BT_GATT_WRITE_FLAG_CMD) {
		LOG_DBG("Feature Report write command received. Ignore received data.");
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	LOG_DBG("Writing to Feature Report characteristic.");

	struct bt_hids_outp_feat_rep *rep = attr->user_data;
	uint8_t idx = rep->idx;
	struct bt_hids *hids = CONTAINER_OF((rep - idx),
						 struct bt_hids,
						 feat_rep_group.reports[0]);
	uint8_t *rep_data;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->feat_rep_ctx + rep->offset;

	if (offset + len > rep->size) {
		ret = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto release_ctx;
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_hids_rep report = {
		    .id = rep->id,
		    .data = rep_data,
		    .size = rep->size,
		};
		rep->handler(&report, conn, true);
	}
release_ctx:
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret;
}

static ssize_t hids_feat_rep_ref_read(struct bt_conn *conn,
				      struct bt_gatt_attr const *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from Feature Report Reference descriptor.");

	uint8_t report_ref[2];

	report_ref[0] = *((uint8_t *)attr->user_data); /* Report ID */
	report_ref[1] = BT_HIDS_REPORT_TYPE_FEATURE;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_ref,
				 sizeof(report_ref));
}

static void hids_input_report_ccc_changed(struct bt_gatt_attr const *attr,
					  uint16_t value)
{
	LOG_DBG("Input Report CCCD has changed.");

	struct bt_hids_inp_rep *inp_rep =
	    CONTAINER_OF((struct bt_gatt_ccc_managed_user_data *)attr->user_data,
			 struct bt_hids_inp_rep, ccc);

	uint8_t report_id = inp_rep->id;
	enum bt_hids_notify_evt evt;

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification has been turned on");
		evt = BT_HIDS_CCCD_EVT_NOTIFY_ENABLED;
	} else {
		LOG_DBG("Notification has been turned off");
		evt = BT_HIDS_CCCD_EVT_NOTIFY_DISABLED;
	}

	if (inp_rep->handler_ext != NULL) {
		inp_rep->handler_ext(report_id, evt);
	} else if (inp_rep->handler != NULL) {
		inp_rep->handler(evt);
	}
}

static ssize_t hids_boot_mouse_inp_report_read(struct bt_conn *conn,
					       struct bt_gatt_attr const *attr,
					       void *buf, uint16_t len,
					       uint16_t offset)
{
	LOG_DBG("Reading from Boot Mouse Input Report characteristic.");

	struct bt_hids_boot_mouse_inp_rep *rep = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(rep, struct bt_hids,
						 boot_mouse_inp_rep);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
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
						uint16_t value)
{
	LOG_DBG("Boot Mouse Input Report CCCD has changed.");

	struct bt_hids_boot_mouse_inp_rep *boot_mouse_rep =
		CONTAINER_OF((struct bt_gatt_ccc_managed_user_data *)attr->user_data,
			     struct bt_hids_boot_mouse_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for Boot Mouse has been turned on");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(
				BT_HIDS_CCCD_EVT_NOTIFY_ENABLED);
		}
	} else {
		LOG_DBG("Notification for Boot Mouse has been turned off");
		if (boot_mouse_rep->handler != NULL) {
			boot_mouse_rep->handler(
				BT_HIDS_CCCD_EVT_NOTIFY_DISABLED);
		}
	}
}

static ssize_t hids_boot_kb_inp_report_read(struct bt_conn *conn,
					    struct bt_gatt_attr const *attr,
					    void *buf, uint16_t len,
					    uint16_t offset)
{
	LOG_DBG("Reading from Boot Keyboard Input Report characteristic.");

	struct bt_hids_boot_kb_inp_rep *rep = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(rep, struct bt_hids,
					    boot_kb_inp_rep);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
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
					     uint16_t value)
{
	LOG_DBG("Boot Keyboard Input Report CCCD has changed.");

	struct bt_hids_boot_kb_inp_rep *boot_kb_inp_rep =
		CONTAINER_OF((struct bt_gatt_ccc_managed_user_data *)attr->user_data,
			     struct bt_hids_boot_kb_inp_rep, ccc);

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for Boot Keyboard has been turned "
			"on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(
				BT_HIDS_CCCD_EVT_NOTIFY_ENABLED);
		}
	} else {
		LOG_DBG("Notification for Boot Keyboard has been turned "
			"on.");
		if (boot_kb_inp_rep->handler != NULL) {
			boot_kb_inp_rep->handler(
				BT_HIDS_CCCD_EVT_NOTIFY_DISABLED);
		}
	}
}

static ssize_t hids_boot_kb_outp_report_read(struct bt_conn *conn,
					     struct bt_gatt_attr const *attr,
					     void *buf, uint16_t len,
					     uint16_t offset)
{
	LOG_DBG("Reading from Boot Keyboard Output Report characteristic.");

	struct bt_hids_boot_kb_outp_rep *rep = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(rep, struct bt_hids,
						 boot_kb_outp_rep);
	uint8_t *rep_data;
	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
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
		struct bt_hids_rep report = {
		    .id = 0,
		    .data = buf,
		    .size = sizeof(conn_data->hids_boot_kb_outp_rep_ctx),
		};
		rep->handler(&report, conn, false);
	}

	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret_len;
}

static ssize_t hids_boot_kb_outp_report_write(struct bt_conn *conn,
					      struct bt_gatt_attr const *attr,
					      void const *buf, uint16_t len,
					      uint16_t offset, uint8_t flags)
{
	ssize_t ret = len;
	LOG_DBG("Writing to Boot Keyboard Output Report characteristic.");

	struct bt_hids_boot_kb_outp_rep *rep = attr->user_data;
	struct bt_hids *hids = CONTAINER_OF(rep, struct bt_hids,
						 boot_kb_outp_rep);
	uint8_t *rep_data;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	rep_data = conn_data->hids_boot_kb_outp_rep_ctx;

	if (offset + len > sizeof(uint8_t)) {
		ret = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto release_ctx;
	}
	memcpy(rep_data + offset, buf, len);

	if (rep->handler) {
		struct bt_hids_rep report = {
		    .id = 0,
		    .data = rep_data,
		    .size = sizeof(conn_data->hids_boot_kb_outp_rep_ctx),
		};

		rep->handler(&report, conn, true);
	}
release_ctx:
	bt_conn_ctx_release(hids->conn_ctx, (void *)conn_data);

	return ret;
}

static ssize_t hids_info_read(struct bt_conn *conn,
			      struct bt_gatt_attr const *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from HID information characteristic.");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 BT_HIDS_INFORMATION_LEN);
}

#if defined(CONFIG_BT_HIDS_SCI)
static ssize_t hid_sci_information_load(struct net_buf_simple *sbuf)
{
	int err = 0;

	struct bt_conn_le_min_conn_interval_info sci_info;

	err = bt_conn_le_read_min_conn_interval_groups(&sci_info);

	if (err) {
		LOG_ERR("Failed to read connection interval info: %d", err);
		switch (err) {
		case -ENOMEM:
		case -ENOBUFS:
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		default:
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	if (sci_info.num_groups > BT_HIDS_SCI_INFORMATION_MAX_GROUPS) {
		LOG_WRN("Number of connection interval groups exceeds max allowed group count "
			"in SCI Information: %u > %u. Truncating to %u groups.",
			sci_info.num_groups, BT_HIDS_SCI_INFORMATION_MAX_GROUPS,
			BT_HIDS_SCI_INFORMATION_MAX_GROUPS);
		sci_info.num_groups = BT_HIDS_SCI_INFORMATION_MAX_GROUPS;
	}

	net_buf_simple_add_u8(sbuf, (uint8_t)(sci_info.min_supported_conn_interval_us
					      / BT_HCI_LE_SCI_INTERVAL_UNIT_US));
	LOG_DBG("min_supported_conn_interval_us: %u",
	       sci_info.min_supported_conn_interval_us);
	net_buf_simple_add_u8(sbuf, sci_info.num_groups);
	LOG_DBG("num_groups: %u", sci_info.num_groups);

	for (size_t i = 0; i < sci_info.num_groups; i++) {
		net_buf_simple_add_le16(sbuf, sci_info.groups[i].min_125us);
		LOG_DBG("group[%zu] min_125us: %u",
		       i,
		       sci_info.groups[i].min_125us);
		net_buf_simple_add_le16(sbuf, sci_info.groups[i].max_125us);
		LOG_DBG("group[%zu] max_125us: %u",
		       i,
		       sci_info.groups[i].max_125us);
		net_buf_simple_add_le16(sbuf, sci_info.groups[i].stride_125us);
		LOG_DBG("group[%zu] stride_125us: %u",
		       i,
		       sci_info.groups[i].stride_125us);
	}

	return (ssize_t)sbuf->len;
}

static ssize_t hids_sci_info_read(struct bt_conn *conn,
	struct bt_gatt_attr const *attr, void *buf,
	uint16_t len, uint16_t offset)
{
	LOG_DBG("Reading from HID SCI information characteristic.");

	NET_BUF_SIMPLE_DEFINE(sci_sbuf, BT_HIDS_SCI_INFORMATION_MAX_LEN);
	ssize_t sci_info_len = 0;

	sci_info_len = hid_sci_information_load(&sci_sbuf);

	if (sci_info_len < 0) {
		LOG_ERR("Failed to load SCI information");
		return sci_info_len;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, sci_sbuf.data, sci_info_len);
}

static ssize_t hids_sci_mode_read(struct bt_conn *conn,
				  struct bt_gatt_attr const *attr,
				  void *buf, uint16_t len,
				  uint16_t offset)
{
	LOG_DBG("Reading from SCI Mode characteristic.");

	if (!sci_mode_hids_obj) {
		LOG_ERR("No HID service found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	ssize_t ret_len;

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(sci_mode_hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_ERR("The context was not found");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, &conn_data->sci_mode,
				 sizeof(uint8_t));

	bt_conn_ctx_release(sci_mode_hids_obj->conn_ctx, (void *)conn_data);

	return ret_len;

}

static void hids_sci_mode_ccc_changed(struct bt_gatt_attr const *attr, uint16_t value)
{
	LOG_DBG("SCI Mode CCCD has changed.");

	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for SCI Mode has been turned on");
	} else {
		LOG_DBG("Notification for SCI Mode has been turned off");
	}
}

int bt_hids_sci_mode_get(struct bt_conn *conn, enum bt_hids_sci_mode_value *mode)
{
	if (conn == NULL || mode == NULL) {
		return -EINVAL;
	}

	if (sci_mode_hids_obj == NULL) {
		LOG_ERR("No HID service found");
		return -ENOENT;
	}

	struct bt_hids_conn_data *conn_data = bt_conn_ctx_get(sci_mode_hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return -ENOENT;
	}

	*mode = conn_data->sci_mode;

	bt_conn_ctx_release(sci_mode_hids_obj->conn_ctx, (void *)conn_data);

	return 0;
}

const struct bt_conn_le_conn_rate_param *bt_hids_sci_mode_conn_rate_param_get(
	enum bt_hids_sci_mode_value mode)
{
	switch (mode) {
	case BT_HIDS_SCI_MODE_DEFAULT:
		return &hids_sci_conn_rate_default;
	case BT_HIDS_SCI_MODE_FAST:
		return &hids_sci_conn_rate_fast;
#if defined(CONFIG_BT_HIDS_SCI_LOW_POWER_MODE)
	case BT_HIDS_SCI_MODE_LOW_POWER:
		return &hids_sci_conn_rate_low_power;
#endif
	case BT_HIDS_SCI_MODE_FULL_RANGE:
		return &hids_sci_conn_rate_full_range;
	default:
		LOG_ERR("Invalid SCI mode: %d", mode);
		return NULL;
	}
}

int bt_hids_sci_mode_change_request(struct bt_conn *conn,
				 enum bt_hids_sci_mode_value mode)
{
	struct bt_conn_le_conn_rate_param const *params = NULL;
	int err = 0;

	if (conn == NULL) {
		return -EINVAL;
	}

	if (sci_mode_hids_obj == NULL) {
		LOG_ERR("No HID service found");
		return -ENOENT;
	}

	struct bt_hids_conn_data *conn_data = bt_conn_ctx_get(sci_mode_hids_obj->conn_ctx, conn);

	if (!conn_data) {
		LOG_WRN("The context was not found");
		return -ENOENT;
	}

	params = bt_hids_sci_mode_conn_rate_param_get(mode);
	if (params == NULL) {
		bt_conn_ctx_release(sci_mode_hids_obj->conn_ctx, (void *)conn_data);
		return -EINVAL;
	}

	err = bt_conn_le_conn_rate_request(conn, params);

	if (err) {
		LOG_ERR("SCI conn rate request failed (%d)", err);
	}

	bt_conn_ctx_release(sci_mode_hids_obj->conn_ctx, (void *)conn_data);

	return err;
}

bool bt_hids_sci_mode_validate(enum bt_hids_sci_mode_value mode,
			       const struct bt_conn_le_conn_rate_changed *params)
{
	const struct bt_conn_le_conn_rate_param *mode_params =
		bt_hids_sci_mode_conn_rate_param_get(mode);

	if (params == NULL || mode_params == NULL) {
		return false;
	}

	const uint64_t interval_min_us =
		(uint64_t)mode_params->interval_min_125us * 125;
	const uint64_t interval_max_us =
		(uint64_t)mode_params->interval_max_125us * 125;

	if (params->interval_us < interval_min_us || params->interval_us > interval_max_us) {
		LOG_WRN("Interval: %u us is outside allowed range %llu-%llu us for mode %d",
			params->interval_us, (unsigned long long)interval_min_us,
			(unsigned long long)interval_max_us, mode);
		return false;
	}

	if (params->subrate_factor < mode_params->subrate_min ||
	    params->subrate_factor > mode_params->subrate_max) {
		LOG_WRN("Subrate factor: %u is outside allowed range %u-%u for mode %d",
			params->subrate_factor, mode_params->subrate_min,
			mode_params->subrate_max, mode);
		return false;
	}

	if (params->peripheral_latency > mode_params->max_latency) {
		LOG_WRN("Peripheral latency: %u is outside allowed range 0-%u for mode %d",
			params->peripheral_latency, mode_params->max_latency, mode);
		return false;
	}

	return true;
}

static void sci_mode_update_notify(struct bt_hids *hids_obj, struct bt_conn *conn,
				   uint8_t mode)
{
	int err = 0;
	struct bt_gatt_attr *sci_mode_attr =
		&hids_obj->gp.svc.attrs[hids_obj->sci_mode_data.att_ind];

	if (bt_gatt_is_subscribed(conn, sci_mode_attr, BT_GATT_CCC_NOTIFY)) {
		err = bt_gatt_notify(conn, sci_mode_attr, &mode, sizeof(mode));
		if (err) {
			LOG_ERR("Failed to notify SCI mode update: %d", err);
		}
	}
}

int bt_hids_sci_mode_updated(struct bt_conn *conn, enum bt_hids_sci_mode_value mode)
{
	int err = 0;

	if (!sci_mode_hids_obj) {
		LOG_WRN("SCI mode updated, but no HID service found");
		return -ENOENT;
	}
	if (conn == NULL) {
		return -EINVAL;
	}

	if (mode != BT_HIDS_SCI_MODE_NONE
	   && mode != BT_HIDS_SCI_MODE_DEFAULT
	   && mode != BT_HIDS_SCI_MODE_FAST
#if defined(CONFIG_BT_HIDS_SCI_LOW_POWER_MODE)
	   && mode != BT_HIDS_SCI_MODE_LOW_POWER
#endif
	   && mode != BT_HIDS_SCI_MODE_FULL_RANGE) {
		LOG_ERR("Invalid SCI mode: %d", mode);
		return -EINVAL;
	}

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(sci_mode_hids_obj->conn_ctx, conn);
	if (!conn_data) {
		LOG_ERR("The context was not found");
		return -ENOENT;
	}

	if (conn_data->sci_mode != mode) {
		conn_data->sci_mode = mode;
		LOG_DBG("SCI mode updated to: %d", conn_data->sci_mode);

		sci_mode_update_notify(sci_mode_hids_obj, conn, (uint8_t) conn_data->sci_mode);
	}

	bt_conn_ctx_release(sci_mode_hids_obj->conn_ctx, (void *)conn_data);

	return err;
}
#endif

static ssize_t hids_ctrl_point_write(struct bt_conn *conn,
				     struct bt_gatt_attr const *attr,
				     void const *buf, uint16_t len,
				     uint16_t offset, uint8_t flags)
{
	LOG_DBG("Writing to Control Point characteristic.");

	struct bt_hids_cp *cp = (struct bt_hids_cp *)attr->user_data;
	ssize_t ret = 0;

	uint8_t *cur_cp = &cp->value;
	uint8_t const *new_cp = (uint8_t const *)buf;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	switch (*new_cp) {
	case BT_HIDS_CP_EVT_HOST_SUSP:
	case BT_HIDS_CP_EVT_HOST_EXIT_SUSP:
		if (cp->conn_evt_handler) {
			cp->conn_evt_handler(*new_cp, conn);
		} else if (cp->evt_handler) {
			cp->evt_handler(*new_cp);
		}
		break;
#if defined(CONFIG_BT_HIDS_SCI)
	case BT_HIDS_CP_EVT_HOST_SCI_DEFAULT_REQ:
	case BT_HIDS_CP_EVT_HOST_SCI_FAST_REQ:
#if defined(CONFIG_BT_HIDS_SCI_LOW_POWER_MODE)
	case BT_HIDS_CP_EVT_HOST_SCI_LOW_POWER_REQ:
#endif
	case BT_HIDS_CP_EVT_HOST_SCI_FULL_RANGE_REQ:
		if (cp->conn_evt_handler) {
			cp->conn_evt_handler(*new_cp, conn);
		}
#endif

	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	if (ret == BT_GATT_ERR(BT_ATT_ERR_SUCCESS)) {
		*cur_cp = *new_cp;
		ret = len;
	}

	return ret;
}

static uint8_t hid_information_encode(uint8_t *buffer,
				      struct bt_hids_info const *hid_info)
{
	uint8_t len = 0;
	uint8_t flags_extra = 0;
#if defined(CONFIG_BT_HIDS_SCI)
	flags_extra |= BT_HIDS_SCI_SUPPORTED;
#endif
#if defined(CONFIG_BT_HIDS_SCI_LOW_POWER_MODE)
	flags_extra |= BT_HIDS_SCI_LOW_POWER_MODE_SUPPORTED;
#endif

	sys_put_le16(hid_info->bcd_hid, buffer);
	len += sizeof(uint16_t);

	buffer[len++] = hid_info->b_country_code;
	buffer[len++] = hid_info->flags | flags_extra;

	__ASSERT(len == BT_HIDS_INFORMATION_LEN,
		 "HIDS Information encoding failed");
	return len;
}

static void
hids_input_reports_register(struct bt_hids *hids_obj,
			    const struct bt_hids_init_param *init_param)
{
	__ASSERT_NO_MSG(init_param->inp_rep_group_init.cnt <=
			CONFIG_BT_HIDS_INPUT_REP_MAX);
	uint8_t offset = 0;

	memcpy(&hids_obj->inp_rep_group, &init_param->inp_rep_group_init,
	       sizeof(hids_obj->inp_rep_group));

	size_t cnt = MIN(hids_obj->inp_rep_group.cnt, ARRAY_SIZE(hids_obj->inp_rep_group.reports));

	for (size_t i = 0; i < cnt; i++) {
		struct bt_hids_inp_rep *hids_inp_rep =
			&hids_obj->inp_rep_group.reports[i];
		uint8_t rperm = hids_inp_rep->perm & GATT_PERM_READ_MASK;
		uint8_t wperm;

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

static void hids_outp_reports_register(struct bt_hids *hids_obj,
				       const struct bt_hids_init_param *init)
{
	__ASSERT_NO_MSG(init->outp_rep_group_init.cnt <=
			CONFIG_BT_HIDS_OUTPUT_REP_MAX);

	uint8_t offset = 0;

	memcpy(&hids_obj->outp_rep_group, &init->outp_rep_group_init,
	       sizeof(hids_obj->outp_rep_group));

	size_t cnt = MIN(hids_obj->outp_rep_group.cnt,
			 ARRAY_SIZE(hids_obj->outp_rep_group.reports));

	for (size_t i = 0; i < cnt; i++) {
		struct bt_hids_outp_feat_rep *hids_outp_rep =
			&hids_obj->outp_rep_group.reports[i];
		uint8_t perm = hids_outp_rep->perm;

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

static void hids_feat_reports_register(struct bt_hids *hids_obj,
				       const struct bt_hids_init_param *init)
{
	__ASSERT_NO_MSG(init->feat_rep_group_init.cnt <=
			CONFIG_BT_HIDS_FEATURE_REP_MAX);

	memcpy(&hids_obj->feat_rep_group, &init->feat_rep_group_init,
	       sizeof(hids_obj->feat_rep_group));

	uint8_t offset = 0;
	size_t cnt = MIN(hids_obj->feat_rep_group.cnt,
			 ARRAY_SIZE(hids_obj->feat_rep_group.reports));

	for (size_t i = 0; i < cnt; i++) {
		struct bt_hids_outp_feat_rep *hids_feat_rep =
		    &hids_obj->feat_rep_group.reports[i];
		uint8_t perm = hids_feat_rep->perm;

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

int bt_hids_init(struct bt_hids *hids_obj,
		 const struct bt_hids_init_param *init_param)
{
	LOG_DBG("Initializing HIDS.");

	int err = 0;

#if defined(CONFIG_BT_HIDS_SCI)
	if (sci_mode_hids_obj) {
		/* TODO: NCSDK-38984: This should be fixed,
		 * more services should be allowed
		 */
		LOG_ERR("Currently only one HID service supporting SCI is supported");
		bt_gatt_pool_free(&hids_obj->gp);
		return -EALREADY;
	}
#endif

	if (init_param->rep_map.size > BT_ATT_MAX_ATTRIBUTE_LEN) {
		LOG_WRN("Report map size exceeds max ATT attribute length");
		return -EMSGSIZE;
	}

	hids_obj->pm.evt_handler = init_param->pm_evt_handler;
	if (init_param->cp_evt_handler) {
		LOG_WRN("cp_evt_handler is deprecated, use conn_cp_evt_handler instead");
		hids_obj->cp.evt_handler = init_param->cp_evt_handler;
	}
	hids_obj->cp.conn_evt_handler = init_param->conn_cp_evt_handler;

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
			  HIDS_GATT_PERM_DEFAULT & GATT_PERM_WRITE_MASK,
			  NULL, hids_ctrl_point_write, &hids_obj->cp);

#if defined(CONFIG_BT_HIDS_SCI)
	/* Register HID SCI Information characteristic and its descriptor.
	 * Note this characteristic does not contain valid data for now,
	 * as this function is called before bt_enable()
	 * Thus, there is no way to acquire the supported connection intervals.
	 * The value of the SCI Information will be updated on the first attempt
	 * to read from the characteristic.
	 */
	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_SCI_INFO,
			  BT_GATT_CHRC_READ,
			  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			  hids_sci_info_read, NULL, NULL);

	/* Register HID SCI Mode Characteristic, its descriptor and CCC. */
	BT_GATT_POOL_CHRC(&hids_obj->gp,
			  BT_UUID_HIDS_SCI_MODE,
			  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			  HIDS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			  hids_sci_mode_read, NULL, NULL);

	hids_obj->sci_mode_data.att_ind = hids_obj->gp.svc.attr_count - 1;
	BT_GATT_POOL_CCC(&hids_obj->gp,
			 hids_obj->sci_mode_data.ccc,
			 hids_sci_mode_ccc_changed,
			 HIDS_GATT_PERM_DEFAULT);

	sci_mode_hids_obj = hids_obj;
#endif

	/* Register HIDS attributes in GATT database. */
	err = bt_gatt_service_register(&hids_obj->gp.svc);
	if (err) {
#if defined(CONFIG_BT_HIDS_SCI)
		sci_mode_hids_obj = NULL;
#endif
		bt_gatt_pool_free(&hids_obj->gp);
		return err;
	}

	return err;
}

int bt_hids_uninit(struct bt_hids *hids_obj)
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

#if defined(CONFIG_BT_HIDS_SCI)
	if (sci_mode_hids_obj == hids_obj) {
		sci_mode_hids_obj = NULL;
	}
#endif

	return 0;
}

static void store_input_report(struct bt_hids_inp_rep *hids_inp_rep,
			       uint8_t *rep_data, uint8_t const *rep,
			       uint8_t len)
{
	if (!hids_inp_rep->rep_mask) {
		memcpy(rep_data, rep, len);
		return;
	}

	const uint8_t *rep_mask = hids_inp_rep->rep_mask;

	for (size_t i = 0; i < len; i++) {
		if ((rep_mask[i / 8] & BIT(i % 8)) != 0) {
			rep_data[i] = rep[i];
		}
	}
}

static int inp_rep_notify_all(struct bt_hids *hids_obj,
			      struct bt_hids_inp_rep *hids_inp_rep,
			      uint8_t const *rep, uint8_t len,
			      bt_gatt_complete_func_t cb)
{
	struct bt_hids_conn_data *conn_data;
	uint8_t *rep_data = NULL;
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

int bt_hids_inp_rep_send_userdata(struct bt_hids *hids_obj,
				  struct bt_conn *conn, uint8_t rep_index,
				  uint8_t const *rep, uint8_t len,
				  bt_gatt_complete_func_t cb, void *userdata)
{
	struct bt_hids_inp_rep *hids_inp_rep =
	    &hids_obj->inp_rep_group.reports[rep_index];
	struct bt_gatt_attr *rep_attr =
	    &hids_obj->gp.svc.attrs[hids_inp_rep->att_ind];
	uint8_t *rep_data;

	if (hids_inp_rep->size != len) {
		return -EINVAL;
	}

	if (!conn) {
		return inp_rep_notify_all(hids_obj, hids_inp_rep, rep, len, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_hids_conn_data *conn_data =
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
	params.user_data = userdata;

	int err = bt_gatt_notify_cb(conn, &params);

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return err;
}

static int boot_mouse_inp_report_notify_all(
	struct bt_hids *hids_obj, const uint8_t *buttons,
	struct bt_hids_boot_mouse_inp_rep *boot_mouse_inp_rep,
	int8_t x_delta, int8_t y_delta, bt_gatt_complete_func_t cb)
{
	struct bt_hids_conn_data *conn_data;
	uint8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	uint8_t *rep_data = NULL;
	uint8_t rep_buff[BT_HIDS_BOOT_MOUSE_REP_LEN] = {0};

	rep_buff[1] = (uint8_t)x_delta;
	rep_buff[2] = (uint8_t)y_delta;

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

int bt_hids_boot_mouse_inp_rep_send(struct bt_hids *hids_obj,
				    struct bt_conn *conn,
				    const uint8_t *buttons,
				    int8_t x_delta, int8_t y_delta,
				    bt_gatt_complete_func_t cb)
{
	uint8_t rep_ind = hids_obj->boot_mouse_inp_rep.att_ind;
	struct bt_hids_boot_mouse_inp_rep *boot_mouse_inp_rep =
	    &hids_obj->boot_mouse_inp_rep;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	uint8_t *rep_data;

	if (!conn) {
		return boot_mouse_inp_report_notify_all(hids_obj, buttons,
							boot_mouse_inp_rep,
							x_delta, y_delta, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_hids_conn_data *conn_data =
		bt_conn_ctx_get(hids_obj->conn_ctx, conn);

	BUILD_ASSERT(sizeof(conn_data->hids_boot_mouse_inp_rep_ctx) >= 3,
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
	rep_data[1] = (uint8_t)x_delta;
	rep_data[2] = (uint8_t)y_delta;

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
boot_kb_inp_notify_all(struct bt_hids *hids_obj, uint8_t const *rep,
		       uint16_t len,
		       struct bt_hids_boot_kb_inp_rep *boot_kb_inp_rep,
		       bt_gatt_complete_func_t cb)
{
	struct bt_hids_conn_data *conn_data;
	uint8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	uint8_t *rep_data = NULL;

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
				       (BT_HIDS_BOOT_KB_INPUT_REP_LEN - len));
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

int bt_hids_boot_kb_inp_rep_send(struct bt_hids *hids_obj,
				      struct bt_conn *conn, uint8_t const *rep,
				      uint16_t len, bt_gatt_complete_func_t cb)
{
	uint8_t rep_ind = hids_obj->boot_kb_inp_rep.att_ind;
	struct bt_hids_boot_kb_inp_rep *boot_kb_input_report =
		&hids_obj->boot_kb_inp_rep;
	struct bt_gatt_attr *rep_attr = &hids_obj->gp.svc.attrs[rep_ind];
	uint8_t *rep_data = NULL;

	if (!conn) {
		return boot_kb_inp_notify_all(hids_obj, rep, len,
					      boot_kb_input_report, cb);
	}

	if (!bt_gatt_is_subscribed(conn, rep_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	struct bt_hids_conn_data *conn_data =
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
	if (len < sizeof(conn_data->hids_boot_kb_inp_rep_ctx)) {
		memset(&rep_data[len], 0, (sizeof(conn_data->hids_boot_kb_inp_rep_ctx) - len));
	}

	struct bt_gatt_notify_params params = {0};

	params.attr = rep_attr;
	params.data = rep_data;
	params.len = sizeof(conn_data->hids_boot_kb_inp_rep_ctx);
	params.func = cb;

	int err = bt_gatt_notify_cb(conn, &params);

	bt_conn_ctx_release(hids_obj->conn_ctx, (void *)conn_data);

	return err;
}
