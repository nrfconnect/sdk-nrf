/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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

#include <bluetooth/services/rscs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_rscs, CONFIG_BT_RSCS_LOG_LEVEL);

/* RSC Application error codes */
#define RSC_ERR_IN_PROGRESS		0x80
#define RSC_ERR_CCC_CONFIG		0x81

/* SC Control Point Opcodes */
#define SC_CP_OP_SET_CUMULATIVE		0x01
#define SC_CP_OP_CALIBRATION		0x02
#define SC_CP_OP_UPDATE_LOC		0x03
#define SC_CP_OP_REQ_SUPP_LOC		0x04
#define SC_CP_OP_RESPONSE		0x10

/* SC Control Point Response Values */
#define SC_CP_RSP_SUCCESS		0x01
#define SC_CP_RSP_OP_NOT_SUPP		0x02
#define SC_CP_RSP_INVAL_PARAM		0x03
#define SC_CP_RSP_FAILED		0x04

/* RSC Measurement Flags */
#define RSC_INSTANT_STRIDE_LEN_PRESENT	BIT(0)
#define RSC_TOTAL_DISTANCE_PRESENT	BIT(1)
#define RSC_WALKING_OR_RUNNING_BIT	BIT(2)

/* Running Speed and Cadence Service Feature bits */
#define RSC_FEAT_INSTA_STRIDE_LEN	BIT(0)
#define RSC_FEAT_TOTAL_DISTANCE		BIT(1)
#define RSC_FEAT_WALKING_RUNNING	BIT(2)
#define RSC_FEAT_SENSOR_CALIB_PROC	BIT(3)
#define RSC_FEAT_MULTI_SENSORS		BIT(4)

/* Attribute indexes */
#define RSC_SVC_MEAS_ATTR_IDX		1

static struct {
	struct bt_gatt_indicate_params ind_params;
	enum bt_rscs_location supported_locations[RSC_LOC_AMT];
	enum bt_rscs_location sensor_loc;
	struct bt_rscs_features features;
	const struct bt_rscs_cp_cb *cp_cb;
	bt_rscs_evt_handler evt_handler;
	uint8_t num_location;
	bool indicating;
} rscs_inst;

struct write_sc_ctrl_point_req {
	uint8_t op;
	union {
		uint32_t total_dist;
		uint8_t location;
	};
} __packed;

struct rscs_meas_nfy {
	uint8_t flags;
	uint16_t inst_speed;
	uint8_t inst_cadence;
} __packed;

struct sc_ctrl_point_ind {
	uint8_t op;
	uint8_t req_op;
	uint8_t status;
} __packed;

static void rsc_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (rscs_inst.evt_handler) {
		rscs_inst.evt_handler(notify_enabled ? RSCS_EVT_MEAS_NOTIFY_ENABLE :
						       RSCS_EVT_MEAS_NOTIFY_DISABLE);
	}
}

static uint16_t feature_encode(void)
{
	uint16_t feature = 0;

	if (rscs_inst.features.inst_stride_len) {
		feature |= RSC_FEAT_INSTA_STRIDE_LEN;
	}
	if (rscs_inst.features.total_distance) {
		feature |= RSC_FEAT_TOTAL_DISTANCE;
	}
	if (rscs_inst.features.walking_or_running) {
		feature |= RSC_FEAT_WALKING_RUNNING;
	}
	if (rscs_inst.features.sensor_calib_proc) {
		feature |= RSC_FEAT_SENSOR_CALIB_PROC;
	}
	if (rscs_inst.features.multi_sensor_loc) {
		feature |= RSC_FEAT_MULTI_SENSORS;
	}

	return feature;
}

static ssize_t read_rsc_featute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint16_t feature = sys_cpu_to_le16(feature_encode());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feature, sizeof(feature));
}

static ssize_t read_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t data_len);

static bool ctrl_point_op_code_validate(uint8_t op_code)
{
	switch (op_code) {
	case SC_CP_OP_SET_CUMULATIVE:
		return rscs_inst.features.total_distance;
	case SC_CP_OP_CALIBRATION:
		return rscs_inst.features.sensor_calib_proc;
	case SC_CP_OP_UPDATE_LOC:
	case SC_CP_OP_REQ_SUPP_LOC:
		return rscs_inst.features.multi_sensor_loc;
	default:
		return false;
	}
}

static bool ctrl_point_param_validate(const struct write_sc_ctrl_point_req *req, uint16_t len)
{
	switch (req->op) {
	case SC_CP_OP_SET_CUMULATIVE:
		return len == sizeof(req->op) + sizeof(req->total_dist);
	case SC_CP_OP_UPDATE_LOC:
		return len == sizeof(req->op) + sizeof(req->location);
	case SC_CP_OP_CALIBRATION:
	case SC_CP_OP_REQ_SUPP_LOC:
		return len == sizeof(req->op);
	default:
		return false;
	}
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct write_sc_ctrl_point_req *req = buf;
	uint8_t status;

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		return BT_GATT_ERR(RSC_ERR_CCC_CONFIG);
	}

	if (rscs_inst.indicating) {
		return BT_GATT_ERR(RSC_ERR_IN_PROGRESS);
	}

	if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (!ctrl_point_op_code_validate(req->op)) {
		LOG_WRN("Operation code not supported %#x", req->op);
		ctrl_point_ind(conn, req->op, SC_CP_RSP_OP_NOT_SUPP, NULL, 0);
		return len;
	}

	if (!ctrl_point_param_validate(req, len)) {
		LOG_WRN("Invalid parameter for op code %#x", req->op);
		ctrl_point_ind(conn, req->op, SC_CP_RSP_INVAL_PARAM, NULL, 0);
		return len;
	}

	switch (req->op) {
	case SC_CP_OP_SET_CUMULATIVE:
		if (rscs_inst.cp_cb && rscs_inst.cp_cb->set_cumulative) {
			if (!rscs_inst.cp_cb->set_cumulative(sys_le32_to_cpu(req->total_dist))) {
				status = SC_CP_RSP_SUCCESS;
				break;
			}
		}
		status = SC_CP_RSP_FAILED;
		break;
	case SC_CP_OP_CALIBRATION:
		if (rscs_inst.cp_cb && rscs_inst.cp_cb->calibration) {
			if (!rscs_inst.cp_cb->calibration()) {
				status = SC_CP_RSP_SUCCESS;
				break;
			}
		}
		status = SC_CP_RSP_FAILED;
		break;
	case SC_CP_OP_UPDATE_LOC:
		status = SC_CP_RSP_INVAL_PARAM;
		for (size_t i = 0; i < rscs_inst.num_location; i++) {
			if (rscs_inst.supported_locations[i] == req->location) {
				rscs_inst.sensor_loc = req->location;
				status = SC_CP_RSP_SUCCESS;
				break;
			}
		}

		if (status == SC_CP_RSP_SUCCESS && rscs_inst.cp_cb && rscs_inst.cp_cb->update_loc) {
			rscs_inst.cp_cb->update_loc(rscs_inst.sensor_loc);
		}
		break;
	case SC_CP_OP_REQ_SUPP_LOC:
		ctrl_point_ind(conn, req->op, SC_CP_RSP_SUCCESS, &rscs_inst.supported_locations,
			       rscs_inst.num_location);
		return len;
	default:
		status = SC_CP_RSP_OP_NOT_SUPP;
		break;
	}

	ctrl_point_ind(conn, req->op, status, NULL, 0);

	return len;
}

BT_GATT_SERVICE_DEFINE(rsc_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RSCS),
	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(rsc_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_FEATURE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_rsc_featute, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_LOCATION, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_location, NULL, &rscs_inst.sensor_loc),
	BT_GATT_CHARACTERISTIC(BT_UUID_SC_CONTROL_POINT, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &rscs_inst.sensor_loc),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	LOG_DBG("Indication %s", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	LOG_DBG("Indication complete");
	rscs_inst.indicating = false;
}

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t len)
{
	int err;
	struct sc_ctrl_point_ind *ind;
	static const struct bt_gatt_attr *attr;

	if (!attr) {
		attr = bt_gatt_find_by_uuid(rsc_svc.attrs, rsc_svc.attr_count,
					    BT_UUID_SC_CONTROL_POINT);
	}

	__ASSERT_NO_MSG(attr);

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*ind) + len);

	ind = net_buf_simple_add(&buf, sizeof(*ind));
	ind->op = SC_CP_OP_RESPONSE;
	ind->req_op = req_op;
	ind->status = status;

	if (data && len) {
		net_buf_simple_add_mem(&buf, data, len);
	}

	rscs_inst.ind_params.attr = attr;
	rscs_inst.ind_params.func = indicate_cb;
	rscs_inst.ind_params.destroy = indicate_destroy;
	rscs_inst.ind_params.data = ind;
	rscs_inst.ind_params.len = buf.len;

	err = bt_gatt_indicate(conn, &rscs_inst.ind_params);
	if (err) {
		LOG_ERR("Failed to send Control Point indication, err: %d", err);
	} else {
		rscs_inst.indicating = true;
	}
}

int bt_rscs_measurement_send(struct bt_conn *conn, const struct bt_rscs_measurement *measurement)
{
	static const struct bt_gatt_attr *attr = &rsc_svc.attrs[RSC_SVC_MEAS_ATTR_IDX];

	if (!measurement) {
		return -EINVAL;
	}

	bool is_inst_stride = rscs_inst.features.inst_stride_len && measurement->is_inst_stride_len;
	bool is_total_dist = rscs_inst.features.total_distance && measurement->is_total_distance;
	struct rscs_meas_nfy *meas_nfy;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*meas_nfy) +
				   (is_inst_stride ? sizeof(measurement->inst_stride_length) : 0) +
				   (is_total_dist ? sizeof(measurement->total_distance) : 0));

	meas_nfy = net_buf_simple_add(&buf, sizeof(*meas_nfy));
	meas_nfy->flags = 0;
	meas_nfy->inst_speed = sys_cpu_to_le16(measurement->inst_speed);
	meas_nfy->inst_cadence = measurement->inst_cadence;

	if (is_inst_stride) {
		meas_nfy->flags |= RSC_INSTANT_STRIDE_LEN_PRESENT;
		net_buf_simple_add_le16(&buf, measurement->inst_stride_length);
	}

	if (is_total_dist) {
		meas_nfy->flags |= RSC_TOTAL_DISTANCE_PRESENT;
		net_buf_simple_add_le32(&buf, measurement->total_distance);
	}

	if (rscs_inst.features.walking_or_running && measurement->is_running) {
		meas_nfy->flags |= RSC_WALKING_OR_RUNNING_BIT;
	}

	if (!conn) {
		return bt_gatt_notify(NULL, attr, meas_nfy, buf.len);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(conn, attr, meas_nfy, buf.len);
	} else {
		return -EINVAL;
	}
}

static void location_encode(struct bt_rsc_supported_loc loc)
{
	uint8_t i = 0;

	if (loc.other) {
		rscs_inst.supported_locations[i++] = RSC_LOC_OTHER;
	}
	if (loc.top_of_shoe) {
		rscs_inst.supported_locations[i++] = RSC_LOC_TOP_OF_SHOE;
	}
	if (loc.in_shoe) {
		rscs_inst.supported_locations[i++] = RSC_LOC_IN_SHOE;
	}
	if (loc.hip) {
		rscs_inst.supported_locations[i++] = RSC_LOC_HIP;
	}
	if (loc.front_wheel) {
		rscs_inst.supported_locations[i++] = RSC_LOC_FRONT_WHEEL;
	}
	if (loc.left_crank) {
		rscs_inst.supported_locations[i++] = RSC_LOC_LEFT_CRANK;
	}
	if (loc.right_crank) {
		rscs_inst.supported_locations[i++] = RSC_LOC_RIGHT_CRANK;
	}
	if (loc.left_pedal) {
		rscs_inst.supported_locations[i++] = RSC_LOC_LEFT_PEDAL;
	}
	if (loc.right_pedal) {
		rscs_inst.supported_locations[i++] = RSC_LOC_RIGHT_PEDAL;
	}
	if (loc.front_hub) {
		rscs_inst.supported_locations[i++] = RSC_LOC_FRONT_HUB;
	}
	if (loc.rear_dropout) {
		rscs_inst.supported_locations[i++] = RSC_LOC_REAR_DROPOUT;
	}
	if (loc.chainstay) {
		rscs_inst.supported_locations[i++] = RSC_LOC_CHAINSTAY;
	}
	if (loc.rear_wheel) {
		rscs_inst.supported_locations[i++] = RSC_LOC_REAR_WHEEL;
	}
	if (loc.rear_hub) {
		rscs_inst.supported_locations[i++] = RSC_LOC_REAR_HUB;
	}
	if (loc.chest) {
		rscs_inst.supported_locations[i++] = RSC_LOC_CHEST;
	}
	if (loc.spider) {
		rscs_inst.supported_locations[i++] = RSC_LOC_SPIDER;
	}
	if (loc.chain_ring) {
		rscs_inst.supported_locations[i++] = RSC_LOC_CHAIN_RING;
	}

	rscs_inst.num_location = i;
}

int bt_rscs_init(const struct bt_rscs_init_params *init)
{
	rscs_inst.features = init->features;

	location_encode(init->supported_locations);

	for (size_t i = 0; i < rscs_inst.num_location; i++) {
		if (rscs_inst.supported_locations[i] == init->location) {
			rscs_inst.sensor_loc = init->location;
		}
	}
	rscs_inst.cp_cb = init->cp_cb;
	rscs_inst.evt_handler = init->evt_handler;

	LOG_DBG("RSCS initialization successful");
	return 0;
}
