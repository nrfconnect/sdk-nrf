/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/ddfs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ddfs, CONFIG_BT_DDFS_LOG_LEVEL);

/* DDF Application error codes */
#define DDF_ERR_IN_PROGRESS		0x80
#define DDF_ERR_CCC_CONFIG		0x81

/* DDF Feature bits */
#define DDF_FEAT_DM_RANGING_MODE_RTT	BIT(0)
#define DDF_FEAT_DM_RANGING_MOCE_MCPD	BIT(1)

/* DDF Opcodes */
#define DDF_OP_DM_RANGING_MODE		0x01
#define DDF_OP_DM_READ_CONF		0x0A
#define DDF_OP_RESPONSE			0x10

/* DDF Control Point Response Values */
#define DDF_CP_RSP_SUCCESS		0x01
#define DDF_CP_RSP_OP_NOT_SUPP		0x02
#define DDF_CP_RSP_INVAL_PARAM		0x03
#define DDF_CP_RSP_FAILED		0x04

/* DFF Distance Measurement Flags */
#define DDF_DM_RTT_PRESENT		BIT(0)
#define DDF_DM_MCPD_PRESENT		BIT(1)
#define DDF_DM_HIGH_PRECISION_PRESENT	BIT(2)

/* Attribute indexes */
#define DDF_SVC_DISTANCE_MEAS_ATTR_IDX	1
#define DDF_SVC_AZIMUTH_MEAS_ATTR_IDX	4
#define DDF_SVC_ELEVATION_MEAS_ATTR_IDX	7

/* Buffer sizes */
#define DDF_CP_READ_CONF_BUF_SIZE	2

/* High Precision state*/
#define DDF_DM_HIGH_PRECISION_ENABLED	1
#define DDF_DM_HIGH_PRECISION_DISABLED	0

static struct {
	struct bt_gatt_indicate_params ind_params;
	struct bt_ddfs_features dm_features;
	const struct bt_ddfs_cb *cb;
	bool indicating;
} ddfs_inst;

struct write_ctrl_point_req {
	uint8_t op;
	union {
		uint8_t ranging_mode;
	};
} __packed;

struct ddfs_bt_addr {
	bt_addr_t a;
	uint8_t type;
} __packed;

struct ddfs_meas_notify {
	uint8_t flags;
	uint8_t quality;
	struct ddfs_bt_addr bt_addr;
} __packed;

struct dm_ctrl_point_ind {
	uint8_t op;
	uint8_t req_op;
	uint8_t status;
} __packed;

static void ddf_distance_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notification_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (ddfs_inst.cb && ddfs_inst.cb->dm_notification_config_changed) {
		ddfs_inst.cb->dm_notification_config_changed(notification_enabled);
	}
}

static void ddf_azimuth_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notification_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (ddfs_inst.cb && ddfs_inst.cb->am_notification_config_changed) {
		ddfs_inst.cb->am_notification_config_changed(notification_enabled);
	}
}

static void ddf_elevation_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notification_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (ddfs_inst.cb && ddfs_inst.cb->em_notification_config_changed) {
		ddfs_inst.cb->em_notification_config_changed(notification_enabled);
	}
}

static uint8_t dm_feature_encode(void)
{
	uint8_t feature = 0;

	if (ddfs_inst.dm_features.ranging_mode_rtt) {
		feature |= DDF_FEAT_DM_RANGING_MODE_RTT;
	}
	if (ddfs_inst.dm_features.ranging_mode_mcpd) {
		feature |= DDF_FEAT_DM_RANGING_MOCE_MCPD;
	}

	return feature;
}

static ssize_t read_ddf_dm_featute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint8_t feature = sys_cpu_to_le16(dm_feature_encode());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feature, sizeof(feature));
}

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t data_len);

static bool dm_ctrl_point_op_code_validate(uint8_t op_code)
{
	switch (op_code) {
	case DDF_OP_DM_RANGING_MODE:
		return (ddfs_inst.dm_features.ranging_mode_rtt ||
			ddfs_inst.dm_features.ranging_mode_mcpd);
	case DDF_OP_DM_READ_CONF:
		return (ddfs_inst.cb && ddfs_inst.cb->dm_config_read);
	}

	return false;
}

static bool dm_ctrl_point_param_validate(const struct write_ctrl_point_req *req, uint16_t len)
{
	switch (req->op) {
	case DDF_OP_DM_RANGING_MODE:
		return len == sizeof(req->op) + sizeof(req->ranging_mode);
	case DDF_OP_DM_READ_CONF:
		return len == sizeof(req->op);
	}

	return false;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	LOG_DBG("Indication %s", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	LOG_DBG("Indication complete");
	ddfs_inst.indicating = false;
}

static ssize_t dm_write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	NET_BUF_SIMPLE_DEFINE(conf_buf, DDF_CP_READ_CONF_BUF_SIZE);
	const struct write_ctrl_point_req *req = buf;
	struct bt_ddfs_dm_config dm_config;
	uint8_t status;

	status = DDF_CP_RSP_OP_NOT_SUPP;

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		return BT_GATT_ERR(DDF_ERR_CCC_CONFIG);
	}

	if (ddfs_inst.indicating) {
		return BT_GATT_ERR(DDF_ERR_IN_PROGRESS);
	}

	if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (!dm_ctrl_point_op_code_validate(req->op)) {
		LOG_WRN("Operation code not supported %#x", req->op);
		ctrl_point_ind(conn, req->op, DDF_CP_RSP_OP_NOT_SUPP, NULL, 0);
		return len;
	}

	if (!dm_ctrl_point_param_validate(req, len)) {
		LOG_WRN("Invalid parameter for op code %#x", req->op);
		ctrl_point_ind(conn, req->op, DDF_CP_RSP_INVAL_PARAM, NULL, 0);
		return len;
	}

	switch (req->op) {
	case DDF_OP_DM_RANGING_MODE:
		if (ddfs_inst.cb && ddfs_inst.cb->dm_ranging_mode_set) {
			if (!ddfs_inst.cb->dm_ranging_mode_set(req->ranging_mode)) {
				status = DDF_CP_RSP_SUCCESS;
				break;
			}
		}
		status = DDF_CP_RSP_FAILED;
		break;
	case DDF_OP_DM_READ_CONF:
		if (ddfs_inst.cb && ddfs_inst.cb->dm_config_read) {
			if (!ddfs_inst.cb->dm_config_read(&dm_config)) {
				status = DDF_CP_RSP_SUCCESS;
				net_buf_simple_add_u8(&conf_buf, dm_config.mode);
				net_buf_simple_add_u8(&conf_buf, dm_config.high_precision ?
				    DDF_DM_HIGH_PRECISION_ENABLED : DDF_DM_HIGH_PRECISION_DISABLED);
				ctrl_point_ind(conn, req->op, status, conf_buf.data, conf_buf.len);
				return len;
			}
			status = DDF_CP_RSP_FAILED;
		}
		break;
	default:
		status = DDF_CP_RSP_OP_NOT_SUPP;
		break;
	}

	ctrl_point_ind(conn, req->op, status, NULL, 0);

	return len;
}

BT_GATT_SERVICE_DEFINE(ddf_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DDFS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DDFS_DISTANCE_MEAS, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(ddf_distance_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DFFS_AZIMUTH_MEAS, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(ddf_azimuth_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DFFS_ELEVATION_MEAS, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(ddf_elevation_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DDFS_FEATURE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_ddf_dm_featute, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DFFS_CTRL_POINT, BT_GATT_CHRC_WRITE |  BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE, NULL, dm_write_ctrl_point, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t len)
{
	int err;
	static const struct bt_gatt_attr *attr;
	struct dm_ctrl_point_ind *ind;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*ind) + len);

	if (!attr) {
		attr = bt_gatt_find_by_uuid(ddf_svc.attrs, ddf_svc.attr_count,
					    BT_UUID_DFFS_CTRL_POINT);
	}

	__ASSERT_NO_MSG(attr);

	ind = net_buf_simple_add(&buf, sizeof(*ind));
	ind->op = DDF_OP_RESPONSE;
	ind->req_op = req_op;
	ind->status = status;

	if (data && len) {
		net_buf_simple_add_mem(&buf, data, len);
	}

	ddfs_inst.ind_params.attr = attr;
	ddfs_inst.ind_params.func = indicate_cb;
	ddfs_inst.ind_params.destroy = indicate_destroy;
	ddfs_inst.ind_params.data = ind;
	ddfs_inst.ind_params.len = buf.len;

	err = bt_gatt_indicate(conn, &ddfs_inst.ind_params);
	if (err) {
		LOG_ERR("Failed to send Control Point indication, err: %d", err);
	} else {
		ddfs_inst.indicating = true;
	}
}

int bt_ddfs_distance_measurement_notify(struct bt_conn *conn,
					const struct bt_ddfs_distance_measurement *measurement)
{
	static const struct bt_gatt_attr *attr = &ddf_svc.attrs[DDF_SVC_DISTANCE_MEAS_ATTR_IDX];

	if (!measurement) {
		return -EINVAL;
	}

	bool is_rtt = measurement->ranging_mode == BT_DDFS_DM_RANGING_MODE_RTT;
	bool is_mcpd = measurement->ranging_mode == BT_DDFS_DM_RANGING_MODE_MCPD;

	struct ddfs_meas_notify *dist_meas_notify;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*dist_meas_notify) +
				   (is_rtt ? sizeof(measurement->dist_estimates.rtt) : 0) +
				   (is_mcpd ? sizeof(measurement->dist_estimates.mcpd) : 0));

	dist_meas_notify = net_buf_simple_add(&buf, sizeof(*dist_meas_notify));
	dist_meas_notify->flags = 0;
	dist_meas_notify->quality = measurement->quality;
	dist_meas_notify->bt_addr.type = measurement->bt_addr.type;

	bt_addr_copy(&dist_meas_notify->bt_addr.a, &measurement->bt_addr.a);

	if (is_rtt) {
		dist_meas_notify->flags |= DDF_DM_RTT_PRESENT;
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.rtt.rtt);
	} else if (is_mcpd) {
		dist_meas_notify->flags |= DDF_DM_MCPD_PRESENT;
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.mcpd.ifft);
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.mcpd.phase_slope);
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.mcpd.rssi_openspace);
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.mcpd.best);
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
		dist_meas_notify->flags |= DDF_DM_HIGH_PRECISION_PRESENT;
		net_buf_simple_add_le16(&buf, measurement->dist_estimates.mcpd.high_precision);
#endif
	}

	if (!conn) {
		return bt_gatt_notify(NULL, attr, dist_meas_notify, buf.len);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(conn, attr, dist_meas_notify, buf.len);
	} else {
		return -EINVAL;
	}
}

int bt_ddfs_azimuth_measurement_notify(struct bt_conn *conn,
				       const struct bt_ddfs_azimuth_measurement *measurement)
{
	static const struct bt_gatt_attr *attr = &ddf_svc.attrs[DDF_SVC_AZIMUTH_MEAS_ATTR_IDX];

	if (!measurement) {
		return -EINVAL;
	}

	struct ddfs_meas_notify *azimuth_meas_notify;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*azimuth_meas_notify) + sizeof(measurement->value));

	azimuth_meas_notify = net_buf_simple_add(&buf, sizeof(*azimuth_meas_notify));
	azimuth_meas_notify->flags = 0;
	azimuth_meas_notify->quality = measurement->quality;
	azimuth_meas_notify->bt_addr.type = measurement->bt_addr.type;

	bt_addr_copy(&azimuth_meas_notify->bt_addr.a, &measurement->bt_addr.a);

	net_buf_simple_add_le16(&buf, measurement->value);

	if (!conn) {
		return bt_gatt_notify(NULL, attr, azimuth_meas_notify, buf.len);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(conn, attr, azimuth_meas_notify, buf.len);
	} else {
		return -EINVAL;
	}
}

int bt_ddfs_elevation_measurement_notify(struct bt_conn *conn,
					const struct bt_ddfs_elevation_measurement *measurement)
{
	static const struct bt_gatt_attr *attr = &ddf_svc.attrs[DDF_SVC_ELEVATION_MEAS_ATTR_IDX];

	if (!measurement) {
		return -EINVAL;
	}

	struct ddfs_meas_notify *elevation_meas_notify;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*elevation_meas_notify) + sizeof(measurement->value));

	elevation_meas_notify = net_buf_simple_add(&buf, sizeof(*elevation_meas_notify));
	elevation_meas_notify->flags = 0;
	elevation_meas_notify->quality = measurement->quality;
	elevation_meas_notify->bt_addr.type = measurement->bt_addr.type;

	bt_addr_copy(&elevation_meas_notify->bt_addr.a, &measurement->bt_addr.a);

	net_buf_simple_add_u8(&buf, measurement->value);

	if (!conn) {
		return bt_gatt_notify(NULL, attr, elevation_meas_notify, buf.len);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(conn, attr, elevation_meas_notify, buf.len);
	} else {
		return -EINVAL;
	}
}

int bt_ddfs_init(const struct bt_ddfs_init_params *init)
{
	if (!init) {
		return -EINVAL;
	}

	ddfs_inst.dm_features = init->dm_features;
	ddfs_inst.cb = init->cb;

	LOG_DBG("DDFS initialization successful");
	return 0;
}
