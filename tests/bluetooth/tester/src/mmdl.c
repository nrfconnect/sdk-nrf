/* mmdl.c - Bluetooth Mesh Models Tester */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>

/* Private Mesh Model headers */
#include <model_utils.h>
#include <gen_battery_internal.h>
#include <gen_loc_internal.h>
#include <sensor.h>
#include <time_internal.h>
#include <lightness_internal.h>
#include <light_ctrl_internal.h>

#define LOG_LEVEL CONFIG_BT_MESH_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bttester_mmdl);

#include "model_handler.h"
#include "bttester.h"

#define CONTROLLER_INDEX 0

struct bt_mesh_onoff_cli onoff_cli = BT_MESH_ONOFF_CLI_INIT(NULL);
struct bt_mesh_lvl_cli lvl_cli = BT_MESH_LVL_CLI_INIT(NULL);
struct bt_mesh_dtt_cli dtt_cli = BT_MESH_DTT_CLI_INIT(NULL);
struct bt_mesh_ponoff_cli ponoff_cli = BT_MESH_PONOFF_CLI_INIT(NULL);
struct bt_mesh_plvl_cli plvl_cli = BT_MESH_PLVL_CLI_INIT(NULL);
struct bt_mesh_battery_cli battery_cli = BT_MESH_BATTERY_CLI_INIT(NULL);
struct bt_mesh_loc_cli loc_cli = BT_MESH_LOC_CLI_INIT(NULL);
struct bt_mesh_prop_cli prop_cli = BT_MESH_PROP_CLI_INIT(NULL, NULL);
struct bt_mesh_sensor_cli sensor_cli = BT_MESH_SENSOR_CLI_INIT(NULL);
struct bt_mesh_time_cli time_cli = BT_MESH_TIME_CLI_INIT(NULL);
struct bt_mesh_lightness_cli lightness_cli = BT_MESH_LIGHTNESS_CLI_INIT(NULL);
struct bt_mesh_light_ctrl_cli light_ctrl_cli =
	BT_MESH_LIGHT_CTRL_CLI_INIT(NULL);
struct bt_mesh_light_ctl_cli light_ctl_cli = BT_MESH_LIGHT_CTL_CLI_INIT(NULL);
struct bt_mesh_scene_cli scene_cli;
struct bt_mesh_light_xyl_cli xyl_cli;
struct bt_mesh_light_hsl_cli light_hsl_cli;
struct bt_mesh_scheduler_cli scheduler_cli;

static void supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);

	/* 1st octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf->data, MMDL_GEN_ONOFF_GET);
	tester_set_bit(buf->data, MMDL_GEN_ONOFF_SET);
	tester_set_bit(buf->data, MMDL_GEN_LVL_GET);
	tester_set_bit(buf->data, MMDL_GEN_LVL_SET);
	tester_set_bit(buf->data, MMDL_GEN_LVL_DELTA_SET);
	tester_set_bit(buf->data, MMDL_GEN_LVL_MOVE_SET);
	/* 2nd octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_GEN_DTT_GET);
	tester_set_bit(buf->data, MMDL_GEN_DTT_SET);
	tester_set_bit(buf->data, MMDL_GEN_PONOFF_GET);
	tester_set_bit(buf->data, MMDL_GEN_PONOFF_SET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_GET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_SET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_LAST_GET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_DFLT_GET);
	/* 3rd octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_DFLT_SET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_RANGE_GET);
	tester_set_bit(buf->data, MMDL_GEN_PLVL_RANGE_SET);
	tester_set_bit(buf->data, MMDL_GEN_BATTERY_GET);
	tester_set_bit(buf->data, MMDL_GEN_LOC_GLOBAL_GET);
	tester_set_bit(buf->data, MMDL_GEN_LOC_LOCAL_GET);
	tester_set_bit(buf->data, MMDL_GEN_LOC_GLOBAL_SET);
	tester_set_bit(buf->data, MMDL_GEN_LOC_LOCAL_SET);
	/* 4th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_GEN_PROPS_GET);
	tester_set_bit(buf->data, MMDL_GEN_PROP_GET);
	tester_set_bit(buf->data, MMDL_GEN_PROP_SET);
	tester_set_bit(buf->data, MMDL_SENSOR_DESC_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_COLUMN_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_SERIES_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_CADENCE_GET);
	/* 5th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_SENSOR_CADENCE_SET);
	tester_set_bit(buf->data, MMDL_SENSOR_SETTINGS_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_SETTING_GET);
	tester_set_bit(buf->data, MMDL_SENSOR_SETTING_SET);
	tester_set_bit(buf->data, MMDL_TIME_GET);
	tester_set_bit(buf->data, MMDL_TIME_SET);
	tester_set_bit(buf->data, MMDL_TIME_ROLE_GET);
	tester_set_bit(buf->data, MMDL_TIME_ROLE_SET);
	/* 6th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_TIME_ZONE_GET);
	tester_set_bit(buf->data, MMDL_TIME_ZONE_SET);
	tester_set_bit(buf->data, MMDL_TIME_TAI_UTC_DELTA_GET);
	tester_set_bit(buf->data, MMDL_TIME_TAI_UTC_DELTA_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_LINEAR_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_LINEAR_SET);
	/* 7th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_LAST_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_DEFAULT_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_DEFAULT_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_RANGE_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LIGHTNESS_RANGE_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_MODE_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_MODE_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_OCCUPANCY_MODE_GET);
	/* 8th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_OCCUPANCY_MODE_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_PROPERTY_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_LC_PROPERTY_SET);
	tester_set_bit(buf->data, MMDL_SENSOR_DATA_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_STATES_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_STATES_SET);
	/* 9th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_TEMPERATURE_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_TEMPERATURE_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_DEFAULT_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_DEFAULT_SET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_TEMPERATURE_RANGE_GET);
	tester_set_bit(buf->data, MMDL_LIGHT_CTL_TEMPERATURE_RANGE_SET);
	tester_set_bit(buf->data, MMDL_SCENE_GET);
	tester_set_bit(buf->data, MMDL_SCENE_REGISTER_GET);
	/* 10th octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MMDL_SCENE_STORE_PROCEDURE);
	tester_set_bit(buf->data, MMDL_SCENE_RECALL);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

struct model_data *lookup_model_bound(uint16_t id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model->id ==
		    BT_MESH_MODEL_ID_GEN_ONOFF_CLI) {
			if (model_bound[i].model) {
				return &model_bound[i];
			}
			break;
		}
	}

	return NULL;
}

static void gen_onoff_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_onoff_status status;
	struct model_data *model_bound;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_onoff_cli_get(&onoff_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.present_on_off);
	net_buf_simple_add_u8(buf, status.target_on_off);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_ONOFF_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_ONOFF_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_onoff_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_onoff_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_onoff_set set;
	struct bt_mesh_onoff_status status;
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.on_off = !!cmd->onoff;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_onoff_cli_set(&onoff_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_onoff_cli_set_unack(&onoff_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.present_on_off);
		net_buf_simple_add_u8(buf, status.target_on_off);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_ONOFF_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_ONOFF_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_lvl_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_lvl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_lvl_cli_get(&lvl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_lvl_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_lvl_set *cmd = (void *)data;
	struct bt_mesh_lvl_set set = {
		.new_transaction = true,
	};
	struct bt_mesh_lvl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lvl = cmd->lvl;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_lvl_cli_set(&lvl_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_lvl_cli_set_unack(&lvl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_lvl_delta_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_lvl_delta_set *cmd = (void *)data;
	struct bt_mesh_lvl_delta_set set = {
		.new_transaction = true,
	};
	struct bt_mesh_lvl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.delta = cmd->delta;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_lvl_cli_delta_set(&lvl_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_lvl_cli_delta_set_unack(&lvl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_DELTA_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_DELTA_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_lvl_move_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_lvl_move_set *cmd = (void *)data;
	struct bt_mesh_lvl_move_set set = {
		.new_transaction = true,
	};
	struct bt_mesh_lvl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.delta = cmd->delta;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_lvl_cli_move_set(&lvl_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_lvl_cli_move_set_unack(&lvl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_MOVE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LVL_MOVE_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_dtt_get(uint8_t *data, uint16_t len)
{
	int32_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound =
		lookup_model_bound(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_dtt_get(&dtt_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	status = model_transition_encode(status);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_DTT_GET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_DTT_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_dtt_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_dtt_set *cmd = (void *)data;
	int32_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound =
		lookup_model_bound(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->ack) {
		err = bt_mesh_dtt_set(
			&dtt_cli, &ctx,
			model_transition_decode(cmd->transition_time), &status);
	} else {
		err = bt_mesh_dtt_set_unack(
			&dtt_cli, &ctx,
			model_transition_decode(cmd->transition_time));
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		status = model_transition_encode(status);
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_DTT_SET,
			    CONTROLLER_INDEX, (uint8_t *)&status,
			    sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_DTT_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_ponoff_get(uint8_t *data, uint16_t len)
{
	enum bt_mesh_on_power_up status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_ponoff_cli_on_power_up_get(&ponoff_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PONOFF_GET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PONOFF_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_ponoff_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_ponoff_set *cmd = (void *)data;
	enum bt_mesh_on_power_up set;
	enum bt_mesh_on_power_up status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set = cmd->on_power_up;

	if (cmd->ack) {
		err = bt_mesh_ponoff_cli_on_power_up_set(&ponoff_cli, &ctx, set,
							 &status);
	} else {
		err = bt_mesh_ponoff_cli_on_power_up_set_unack(&ponoff_cli,
							       &ctx, set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PONOFF_SET,
			    CONTROLLER_INDEX, (uint8_t *)&status,
			    sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PONOFF_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_plvl_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_plvl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_plvl_cli_power_get(&plvl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_plvl_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_plvl_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct bt_mesh_plvl_status status;
	struct bt_mesh_plvl_set set;
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.power_lvl = cmd->power_lvl;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_plvl_cli_power_set(&plvl_cli, &ctx, &set,
						 &status);
	} else {
		err = bt_mesh_plvl_cli_power_set_unack(&plvl_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_plvl_last_get(uint8_t *data, uint16_t len)
{
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_plvl_cli_last_get(&plvl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_LAST_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_LAST_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void gen_plvl_dflt_get(uint8_t *data, uint16_t len)
{
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_plvl_cli_default_get(&plvl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_DFLT_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_DFLT_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void gen_plvl_dflt_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_plvl_dflt_set *cmd = (void *)data;
	uint16_t status;
	uint16_t set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set = cmd->power_dflt;

	if (cmd->ack) {
		err = bt_mesh_plvl_cli_default_set(&plvl_cli, &ctx, set,
						   &status);
	} else {
		err = bt_mesh_plvl_cli_default_set_unack(&plvl_cli, &ctx, set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_DFLT_SET,
			    CONTROLLER_INDEX, (uint8_t *)&status,
			    sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_DFLT_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_plvl_range_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_plvl_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_plvl_cli_range_get(&plvl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.status);
	net_buf_simple_add_le16(buf, status.range.min);
	net_buf_simple_add_le16(buf, status.range.max);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_RANGE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_RANGE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void gen_plvl_range_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_plvl_range_set *cmd = (void *)data;
	struct bt_mesh_plvl_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_plvl_range set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.min = cmd->range_min;
	set.max = cmd->range_max;

	if (cmd->ack) {
		err = bt_mesh_plvl_cli_range_set(&plvl_cli, &ctx, &set,
						 &status);
	} else {
		err = bt_mesh_plvl_cli_range_set_unack(&plvl_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.status);
		net_buf_simple_add_le16(buf, status.range.min);
		net_buf_simple_add_le16(buf, status.range.max);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_RANGE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PLVL_RANGE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void battery_get(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_BATTERY_MSG_LEN_STATUS);
	struct bt_mesh_battery_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_BATTERY_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_battery_cli_get(&battery_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	bt_mesh_gen_bat_encode_status(buf, &status);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_BATTERY_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_BATTERY_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void location_global_get(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);
	struct bt_mesh_loc_global status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_LOCATION_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_loc_cli_global_get(&loc_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf_rsp, 0);
	bt_mesh_loc_global_encode(buf_rsp, &status);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_GLOBAL_GET,
		    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_GLOBAL_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void location_local_get(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);
	struct bt_mesh_loc_local status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_LOCATION_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_loc_cli_local_get(&loc_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf_rsp, 0);
	bt_mesh_loc_local_encode(buf_rsp, &status);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_LOCAL_GET,
		    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_LOCAL_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void location_global_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_loc_global_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);
	struct bt_mesh_loc_global status;
	struct bt_mesh_loc_global set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_LOCATION_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	net_buf_simple_init_with_data(buf, (void *)&data[1], len - 1);
	bt_mesh_loc_global_decode(buf, &set);

	if (cmd->ack) {
		err = bt_mesh_loc_cli_global_set(&loc_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_loc_cli_global_set_unack(&loc_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf_rsp, 0);
		bt_mesh_loc_global_encode(buf_rsp, &status);
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_GLOBAL_SET,
			    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
		return;
	}
fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_GLOBAL_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

void location_local_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_loc_local_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);
	struct bt_mesh_loc_local status;
	struct bt_mesh_loc_local set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_LOCATION_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	net_buf_simple_init_with_data(buf, (void *)&data[1], len - 1);
	bt_mesh_loc_local_decode(buf, &set);

	if (cmd->ack) {
		err = bt_mesh_loc_cli_local_set(&loc_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_loc_cli_local_set_unack(&loc_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf_rsp, 0);
		bt_mesh_loc_local_encode(buf_rsp, &status);
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_LOCAL_SET,
			    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_LOC_LOCAL_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void gen_props_get(uint8_t *data, uint16_t len)
{
	struct mesh_gen_props_get *cmd = (void *)data;
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS);
	uint16_t ids[10];
	struct bt_mesh_prop_list status = {
		.count = ARRAY_SIZE(ids),
		.ids = ids,
	};
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_PROP_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->kind == BT_MESH_PROP_SRV_KIND_CLIENT) {
		err = bt_mesh_prop_cli_client_props_get(&prop_cli, &ctx,
							cmd->id, &status);
	} else {
		err = bt_mesh_prop_cli_props_get(
			&prop_cli, &ctx, (enum bt_mesh_prop_srv_kind)cmd->kind,
			&status);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	net_buf_simple_init(buf, 0);
	net_buf_simple_add_mem(buf, status.ids,
			       sizeof(*status.ids) * status.count);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROPS_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROPS_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_prop_get(uint8_t *data, uint16_t len)
{
	struct mesh_gen_prop_get *cmd = (void *)data;
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);
	uint8_t val[10];
	struct bt_mesh_prop_val status = {
		.size = ARRAY_SIZE(val),
		.value = val,
	};
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_PROP_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_prop_cli_prop_get(&prop_cli, &ctx,
					(enum bt_mesh_prop_srv_kind)cmd->kind,
					cmd->id, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.meta.id);
	net_buf_simple_add_u8(buf, status.meta.user_access);
	net_buf_simple_add_u8(buf, status.size);
	net_buf_simple_add_mem(buf, status.value, status.size);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROP_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROP_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void gen_prop_set(uint8_t *data, uint16_t len)
{
	struct mesh_gen_prop_set *cmd = (void *)data;
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);
	struct bt_mesh_prop prop = {
		.id = cmd->id,
		.user_access = (enum bt_mesh_prop_access)cmd->access,
	};
	uint8_t val[10];
	struct bt_mesh_prop_val status = {
		.meta.id = cmd->id,
		.meta.user_access = (enum bt_mesh_prop_access)cmd->access,
		.size = ARRAY_SIZE(val),
		.value = val,
	};
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_GEN_PROP_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->len > status.size) {
		err = -EINVAL;
		goto fail;
	}

	if (cmd->len > 0) {
		status.size = cmd->len;
		memcpy(status.value, cmd->data, status.size);
	}

	switch (cmd->kind) {
	case BT_MESH_PROP_SRV_KIND_MFR:
		if (cmd->ack) {
			err = bt_mesh_prop_cli_mfr_prop_set(&prop_cli, &ctx,
							    &prop, &status);
		} else {
			err = bt_mesh_prop_cli_mfr_prop_set_unack(&prop_cli,
								  &ctx, &prop);
		}
		break;
	case BT_MESH_PROP_SRV_KIND_ADMIN:
		if (cmd->ack) {
			err = bt_mesh_prop_cli_admin_prop_set(&prop_cli, &ctx,
							      &status, &status);
		} else {
			err = bt_mesh_prop_cli_admin_prop_set_unack(
				&prop_cli, &ctx, &status);
		}
		break;
	case BT_MESH_PROP_SRV_KIND_USER:
		if (cmd->ack) {
			err = bt_mesh_prop_cli_user_prop_set(&prop_cli, &ctx,
							     &status, &status);
		} else {
			err = bt_mesh_prop_cli_user_prop_set_unack(
				&prop_cli, &ctx, &status);
		}
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.meta.id);
		net_buf_simple_add_u8(buf, status.meta.user_access);
		net_buf_simple_add_u8(buf, status.size);
		net_buf_simple_add_mem(buf, status.value, status.size);
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROP_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_GEN_PROP_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void sensor_desc_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_desc_get *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX);
	struct bt_mesh_sensor sensor;
	struct bt_mesh_sensor_descriptor rsp;
	struct bt_mesh_sensor_info sensors[5];
	uint32_t count = ARRAY_SIZE(sensors);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err = 0;
	int i;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (len == 2) {
		sensor.type = bt_mesh_sensor_type_get(cmd->id);
		if (!sensor.type) {
			goto fail;
		}

		err = bt_mesh_sensor_cli_desc_get(&sensor_cli, &ctx,
						  sensor.type, &rsp);
	} else {
		err = bt_mesh_sensor_cli_desc_all_get(&sensor_cli, &ctx,
						      sensors, &count);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);

	if (len == 2) {
		sensor.descriptor = &rsp;
		sensor_descriptor_encode(buf, &sensor);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_DESC_GET,
			    CONTROLLER_INDEX, buf->data, buf->len);
	} else {
		for (i = 0; i < count; ++i) {
			sensor.type = bt_mesh_sensor_type_get(sensors[i].id);
			sensor.descriptor = &sensors[i].descriptor;
			sensor_descriptor_encode(buf, &sensor);
		}
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_DESC_GET,
			    CONTROLLER_INDEX, buf->data, buf->len);
	}
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_DESC_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void sensor_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_get *cmd = (void *)data;
	const struct bt_mesh_sensor_type *sensor;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX);
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	struct bt_mesh_sensor_data values[5];
	uint32_t count = ARRAY_SIZE(values);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err = 0;
	int i;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (len == 2) {
		sensor = bt_mesh_sensor_type_get(cmd->id);
		if (!sensor) {
			LOG_ERR("Sensor ID not found");
			goto fail;
		}

		err = bt_mesh_sensor_cli_get(&sensor_cli, &ctx, sensor, value);
	} else {
		err = bt_mesh_sensor_cli_all_get(&sensor_cli, &ctx, values,
						 &count);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);

	if (len == 2) {
		net_buf_simple_add_le16(buf, sensor->id);
		err = sensor_value_encode(buf, sensor, value);
		if (err) {
			goto fail;
		}

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_GET,
			    CONTROLLER_INDEX, buf->data, buf->len);
	} else {
		for (i = 0; i < count; ++i) {
			sensor = bt_mesh_sensor_type_get(values[i].type->id);
			if (!sensor) {
				LOG_ERR("Sensor ID not found");
				goto fail;
			}

			net_buf_simple_add_le16(buf, values[i].type->id);
			err = sensor_value_encode(buf, sensor,
						  values[i].value);
			if (err) {
				goto fail;
			}
		}
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_GET,
			    CONTROLLER_INDEX, buf->data, buf->len);
	}
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void sensor_cadence_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_cadence_get *cmd = (void *)data;
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	const struct bt_mesh_sensor_type *sensor;
	struct bt_mesh_sensor_cadence_status rsp;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		LOG_ERR("Sensor ID not found");
		goto fail;
	}

	err = bt_mesh_sensor_cli_cadence_get(&sensor_cli, &ctx, sensor, &rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	net_buf_simple_init(buf, 0);
	err = sensor_cadence_encode(buf, sensor, rsp.fast_period_div,
				    rsp.min_int, &rsp.threshold);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_add_le16(buf, sensor->id);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_CADENCE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_CADENCE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void sensor_cadence_set(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_cadence_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	const struct bt_mesh_sensor_type *sensor;
	struct bt_mesh_sensor_cadence_status cadence;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	net_buf_simple_init_with_data(buf, (void *)&cmd->data, cmd->len);
	sensor = bt_mesh_sensor_type_get(cmd->id);
	err = sensor_cadence_decode(buf, sensor, &cadence.fast_period_div,
				    &cadence.min_int, &cadence.threshold);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		err = bt_mesh_sensor_cli_cadence_set(&sensor_cli, &ctx, sensor,
						     &cadence, &cadence);
	} else {
		err = bt_mesh_sensor_cli_cadence_set_unack(&sensor_cli, &ctx,
							   sensor, &cadence);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf_rsp, 0);
		err = sensor_cadence_encode(buf_rsp, sensor,
					    cadence.fast_period_div,
					    cadence.min_int,
					    &cadence.threshold);
		if (err) {
			LOG_ERR("err=%d", err);
			goto fail;
		}
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_CADENCE_SET,
			    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_CADENCE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void sensor_settings_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_settings_get *cmd = (void *)data;
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS);
	const struct bt_mesh_sensor_type *sensor;
	uint16_t ids[10];
	uint32_t count = ARRAY_SIZE(ids);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;
	int i;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	err = bt_mesh_sensor_cli_settings_get(&sensor_cli, &ctx, sensor, ids,
					      &count);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, sensor->id);
	for (i = 0; i < count; ++i) {
		net_buf_simple_add_le16(buf, ids[i]);
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SETTINGS_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SETTINGS_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void sensor_setting_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_setting_get *cmd = (void *)data;
	const struct bt_mesh_sensor_type *sensor;
	const struct bt_mesh_sensor_type *setting;
	struct bt_mesh_sensor_setting_status rsp;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		err = -EINVAL;
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	setting = bt_mesh_sensor_type_get(cmd->setting_id);
	if (!setting) {
		err = -EINVAL;
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	err = bt_mesh_sensor_cli_setting_get(&sensor_cli, &ctx, sensor,
					     setting, &rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SETTING_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void sensor_setting_set(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_setting_set *cmd = (void *)data;
	const struct bt_mesh_sensor_type *sensor;
	const struct bt_mesh_sensor_type *setting;
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	struct bt_mesh_sensor_setting_status rsp;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		err = -EINVAL;
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	setting = bt_mesh_sensor_type_get(cmd->setting_id);
	if (!setting) {
		err = -EINVAL;
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	if (cmd->len > sizeof(value)) {
		err = -EINVAL;
		LOG_ERR("Payload too long");
		goto fail;
	}

	memset(value, 0, sizeof(value));
	memcpy(value, cmd->data, cmd->len);

	if (cmd->ack) {
		err = bt_mesh_sensor_cli_setting_set(&sensor_cli, &ctx, sensor,
						     setting, value, &rsp);
	} else {
		err = bt_mesh_sensor_cli_setting_set_unack(
			&sensor_cli, &ctx, sensor, setting, value);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SETTING_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

struct sensor_value val_to_sensorval(int val)
{
	struct sensor_value out;

	out.val1 = val;
	out.val2 = (val - out.val1) * 1000000;

	return out;
}

static void sensor_column_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_column_get *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_STATUS);
	const struct bt_mesh_sensor_type *sensor;
	struct bt_mesh_sensor_column column = { 0 };
	struct bt_mesh_sensor_series_entry rsp;
	const struct bt_mesh_sensor_format *col_format;
	struct sensor_value width;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	net_buf_simple_init_with_data(buf, (void *)&cmd->data, cmd->len);

	col_format = bt_mesh_sensor_column_format_get(sensor);
	if (col_format == NULL) {
		column.start = val_to_sensorval(cmd->data[0]);
	} else {
		err = sensor_ch_decode(buf, col_format, &column.start);

		if (err) {
			LOG_ERR("err=%d", err);
			goto fail;
		}
	}

	err = bt_mesh_sensor_cli_series_entry_get(&sensor_cli, &ctx, sensor,
						  &column, &rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	net_buf_simple_init(buf_rsp, 0);
	net_buf_simple_add_le16(buf_rsp, sensor->id);

	if (col_format == NULL) {
		sensor_value_encode(buf_rsp, sensor, rsp.value);
		goto send;
	}

	LOG_ERR("err=%d", err);
	err = sensor_ch_encode(buf_rsp, col_format, &rsp.column.start);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	width.val1 = rsp.column.end.val1 - rsp.column.start.val1;
	width.val2 = rsp.column.end.val2 - rsp.column.start.val2;

	err = sensor_ch_encode(buf_rsp, col_format, &width);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	err = sensor_value_encode(buf_rsp, sensor, rsp.value);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
send:
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_COLUMN_GET,
		    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_COLUMN_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void sensor_series_get(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_series_get *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX);
	const struct bt_mesh_sensor_type *sensor;
	struct bt_mesh_sensor_column range;
	struct bt_mesh_sensor_series_entry rsp[10];
	struct sensor_value width;
	uint32_t count = ARRAY_SIZE(rsp);
	const struct bt_mesh_sensor_format *col_format;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;
	int i;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SENSOR_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	sensor = bt_mesh_sensor_type_get(cmd->id);
	if (!sensor) {
		err = -EINVAL;
		LOG_ERR("Unknown Sensor ID");
		goto fail;
	}

	net_buf_simple_init_with_data(buf, (void *)&cmd->data, cmd->len);

	col_format = bt_mesh_sensor_column_format_get(sensor);

	if (col_format != NULL) {
		err = sensor_ch_decode(buf, col_format, &range.end);
		if (err) {
			goto fail;
		}
	} else {
		range.start.val1 = net_buf_simple_pull_u8(buf);
		range.end.val1 = net_buf_simple_pull_u8(buf);
	}


	err = bt_mesh_sensor_cli_series_entries_get(&sensor_cli, &ctx, sensor,
						    &range, rsp, &count);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf_rsp, 0);

	if (col_format == NULL) {
		for (i = 0; i < count; ++i) {
			sensor_value_encode(buf_rsp, sensor, rsp[i].value);
		}
		goto send;
	}

	for (i = 0; i < count; ++i) {
		err = sensor_ch_encode(buf_rsp, col_format,
				       &rsp[i].column.start);
		if (err) {
			LOG_ERR("err=%d", err);
			goto fail;
		}

		width.val1 = rsp[i].column.end.val1 - rsp[i].column.start.val1;
		width.val2 = rsp[i].column.end.val2 - rsp[i].column.start.val2;

		err = sensor_ch_encode(buf_rsp, col_format, &width);
		if (err) {
			LOG_ERR("err=%d", err);
			goto fail;
		}

		err = sensor_value_encode(buf_rsp, sensor, rsp[i].value);
		if (err) {
			LOG_ERR("err=%d", err);
			goto fail;
		}
	}

send:
	net_buf_simple_add_le16(buf_rsp, sensor->id);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SERIES_GET,
		    CONTROLLER_INDEX, buf_rsp->data, buf_rsp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_SERIES_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void time_get(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf =
		NET_BUF_SIMPLE(BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS);
	struct bt_mesh_time_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_time_cli_time_get(&time_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	bt_mesh_time_encode_time_params(buf, &status);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_set(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);
	struct net_buf_simple *buf_rsp =
		NET_BUF_SIMPLE(BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS);
	struct bt_mesh_time_status set;
	struct bt_mesh_time_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	net_buf_simple_init_with_data(buf, data, len);
	bt_mesh_time_decode_time_params(buf, &set);

	err = bt_mesh_time_cli_time_set(&time_cli, &ctx, &set, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf_rsp, 0);
	bt_mesh_time_encode_time_params(buf_rsp, &status);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_SET, CONTROLLER_INDEX,
		    buf_rsp->data, buf_rsp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_role_get(uint8_t *data, uint16_t len)
{
	uint8_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_time_cli_role_get(&time_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_ROLE_GET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_ROLE_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_role_set(uint8_t *data, uint16_t len)
{
	struct mesh_time_role_set *cmd = (void *)data;
	uint8_t status;
	uint8_t set = cmd->role;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_time_cli_role_set(&time_cli, &ctx, &set, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_ROLE_SET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_ROLE_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_zone_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_time_zone_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_time_cli_zone_get(&time_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current_offset);
	net_buf_simple_add_le16(buf, status.time_zone_change.new_offset);
	net_buf_simple_add_le64(buf, status.time_zone_change.timestamp);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_ZONE_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_ZONE_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_zone_set(uint8_t *data, uint16_t len)
{
	struct mesh_time_zone_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_time_zone_change set;
	struct bt_mesh_time_zone_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.new_offset = cmd->new_offset;
	set.timestamp = cmd->timestamp;

	err = bt_mesh_time_cli_zone_set(&time_cli, &ctx, &set, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.time_zone_change.new_offset);
	net_buf_simple_add_le64(buf, status.time_zone_change.timestamp);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_ZONE_SET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_ZONE_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void time_tai_utc_delta_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_time_tai_utc_delta_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_time_cli_tai_utc_delta_get(&time_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.delta_current);
	net_buf_simple_add_le16(buf, status.tai_utc_change.delta_new);
	net_buf_simple_add_le64(buf, status.tai_utc_change.timestamp);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_TAI_UTC_DELTA_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_TAI_UTC_DELTA_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void time_tai_utc_delta_set(uint8_t *data, uint16_t len)
{
	struct mesh_time_tai_utc_delta_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_time_tai_utc_change set;
	struct bt_mesh_time_tai_utc_delta_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_TIME_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.delta_new = cmd->delta_new;
	set.timestamp = cmd->timestamp;

	err = bt_mesh_time_cli_tai_utc_delta_set(&time_cli, &ctx, &set,
						 &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.tai_utc_change.delta_new);
	net_buf_simple_add_le64(buf, status.tai_utc_change.timestamp);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_TIME_TAI_UTC_DELTA_SET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_TIME_TAI_UTC_DELTA_SET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_lightness_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_lightness_cli_light_get(&lightness_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lightness_set *cmd = (void *)data;
	struct bt_mesh_lightness_set set;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct bt_mesh_lightness_status status;
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lvl = cmd->lightness;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_lightness_cli_light_set(&lightness_cli, &ctx,
						      &set, &status);
	} else {
		err = bt_mesh_lightness_cli_light_set_unack(&lightness_cli,
							    &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lightness_linear_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_lightness_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(4);
	uint16_t current, target;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = lightness_cli_light_get(&lightness_cli, &ctx, LINEAR, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	current = to_linear(status.current);
	target = to_linear(status.target);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, current);
	net_buf_simple_add_le16(buf, target);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_LINEAR_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_LINEAR_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_linear_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lightness_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	uint16_t current, target;
	struct bt_mesh_lightness_set set;
	struct bt_mesh_lightness_status status;
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lvl = cmd->lightness;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = lightness_cli_light_set(&lightness_cli, &ctx, LINEAR,
					      &set, &status);

		current = to_linear(status.current);
		target = to_linear(status.target);

		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, current);
		net_buf_simple_add_le16(buf, target);
		net_buf_simple_add_le32(buf, status.remaining_time);

	} else {
		err = lightness_cli_light_set_unack(&lightness_cli, &ctx,
						    LINEAR, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL,
			    MMDL_LIGHT_LIGHTNESS_LINEAR_SET, CONTROLLER_INDEX,
			    buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_LINEAR_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lightness_last_get(uint8_t *data, uint16_t len)
{
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_lightness_cli_last_get(&lightness_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_LAST_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_LAST_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_default_get(uint8_t *data, uint16_t len)
{
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_lightness_cli_default_get(&lightness_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_DEFAULT_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_DEFAULT_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_default_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lightness_default_set *cmd = (void *)data;
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->ack) {
		err = bt_mesh_lightness_cli_default_set(&lightness_cli, &ctx,
							cmd->dflt, &status);
	} else {
		err = bt_mesh_lightness_cli_default_set_unack(&lightness_cli,
							      &ctx, cmd->dflt);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL,
			    MMDL_LIGHT_LIGHTNESS_DEFAULT_SET, CONTROLLER_INDEX,
			    (uint8_t *)&status, sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_DEFAULT_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lightness_range_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_lightness_range_status status;
	struct model_data *model_bound;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_lightness_cli_range_get(&lightness_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.status);
	net_buf_simple_add_le16(buf, status.range.min);
	net_buf_simple_add_le16(buf, status.range.max);
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_RANGE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_RANGE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lightness_range_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lightness_range_set *cmd = (void *)data;
	struct bt_mesh_lightness_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_lightness_range set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.min = cmd->min;
	set.max = cmd->max;

	if (cmd->ack) {
		err = bt_mesh_lightness_cli_range_set(&lightness_cli, &ctx,
						      &set, &status);

		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.status);
		net_buf_simple_add_le16(buf, status.range.min);
		net_buf_simple_add_le16(buf, status.range.max);

	} else {
		err = bt_mesh_lightness_cli_range_set_unack(&lightness_cli,
							    &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_RANGE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LIGHTNESS_RANGE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lc_mode_get(uint8_t *data, uint16_t len)
{
	bool status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_ctrl_cli_mode_get(&light_ctrl_cli, &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_MODE_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_MODE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lc_mode_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lc_mode_set *cmd = (void *)data;
	bool status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->ack) {
		err = bt_mesh_light_ctrl_cli_mode_set(&light_ctrl_cli, &ctx,
						      !!cmd->enabled, &status);
	} else {
		err = bt_mesh_light_ctrl_cli_mode_set_unack(
			&light_ctrl_cli, &ctx, !!cmd->enabled);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_MODE_SET,
			    CONTROLLER_INDEX, (uint8_t *)&status,
			    sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_MODE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lc_occupancy_mode_get(uint8_t *data, uint16_t len)
{
	bool status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_ctrl_cli_occupancy_enabled_get(&light_ctrl_cli,
							   &ctx, &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_OCCUPANCY_MODE_GET,
		    CONTROLLER_INDEX, (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_OCCUPANCY_MODE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lc_occupancy_mode_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lc_occupancy_mode_set *cmd = (void *)data;
	bool status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	if (cmd->ack) {
		err = bt_mesh_light_ctrl_cli_occupancy_enabled_set(
			&light_ctrl_cli, &ctx, !!cmd->enabled, &status);
	} else {
		err = bt_mesh_light_ctrl_cli_occupancy_enabled_set_unack(
			&light_ctrl_cli, &ctx, !!cmd->enabled);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MMDL,
			    MMDL_LIGHT_LC_OCCUPANCY_MODE_SET, CONTROLLER_INDEX,
			    (uint8_t *)&status, sizeof(status));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_OCCUPANCY_MODE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lc_light_onoff_mode_get(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_onoff_status status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_ctrl_cli_light_onoff_get(&light_ctrl_cli, &ctx,
						     &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, !!status.present_on_off);
	net_buf_simple_add_u8(buf, !!status.target_on_off);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lc_light_onoff_mode_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lc_light_onoff_mode_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_onoff_status status;
	struct bt_mesh_onoff_set set;
	struct bt_mesh_model_transition transition;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	set.on_off = !!cmd->onoff;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_light_ctrl_cli_light_onoff_set(
			&light_ctrl_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_light_ctrl_cli_light_onoff_set_unack(
			&light_ctrl_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, !!status.present_on_off);
		net_buf_simple_add_u8(buf, !!status.target_on_off);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL,
			    MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_lc_property_get(uint8_t *data, uint16_t len)
{
	struct mesh_light_lc_property_get *cmd = (void *)data;
	struct net_buf_simple *rsp_buf = NET_BUF_SIMPLE(4);
	struct sensor_value status;
	const struct bt_mesh_sensor_format *format;
	enum bt_mesh_light_ctrl_prop id;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	id = (enum bt_mesh_light_ctrl_prop)cmd->id;

	err = bt_mesh_light_ctrl_cli_prop_get(&light_ctrl_cli, &ctx, id,
					      &status);
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	format = prop_format_get(id);
	if (!format) {
		LOG_ERR("Invalid format");
		goto fail;
	}

	net_buf_simple_init(rsp_buf, 0);
	net_buf_simple_add_le16(rsp_buf, cmd->id);
	err = sensor_ch_encode(rsp_buf, format, &status);
	if (err) {
		LOG_ERR("Cannot encode");
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_PROPERTY_GET,
		    CONTROLLER_INDEX, rsp_buf->data, rsp_buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_PROPERTY_GET,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void light_lc_property_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_lc_property_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(2);
	struct net_buf_simple *rsp_buf = NET_BUF_SIMPLE(4);
	const struct bt_mesh_sensor_format *format;
	enum bt_mesh_light_ctrl_prop id;
	struct sensor_value val;
	struct sensor_value status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_LC_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	id = (enum bt_mesh_light_ctrl_prop)cmd->id;

	net_buf_simple_init_with_data(buf, &cmd->val, sizeof(cmd->val));

	format = prop_format_get(id);
	if (!format) {
		LOG_ERR("Invalid format");
		goto fail;
	}

	err = sensor_ch_decode(buf, format, &val);
	if (err) {
		LOG_ERR("Cannot decode");
		goto fail;
	}

	if (cmd->ack) {
		err = bt_mesh_light_ctrl_cli_prop_set(&light_ctrl_cli, &ctx, id,
						      &val, &status);
	} else {
		err = bt_mesh_light_ctrl_cli_prop_set_unack(&light_ctrl_cli,
							    &ctx, id, &val);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(rsp_buf, 0);
		net_buf_simple_add_le16(rsp_buf, cmd->id);
		err = sensor_ch_encode(rsp_buf, format, &status);
		if (err) {
			LOG_ERR("Cannot encode");
			goto fail;
		}

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_PROPERTY_SET,
			    CONTROLLER_INDEX, rsp_buf->data, rsp_buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_LC_PROPERTY_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_ctl_states_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_ctl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_ctl_get(&light_ctl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current_light);
	net_buf_simple_add_le16(buf, status.current_temp);
	net_buf_simple_add_le16(buf, status.target_light);
	net_buf_simple_add_le16(buf, status.target_temp);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_STATES_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_STATES_GET,
		   CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void light_ctl_states_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_ctl_states_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct bt_mesh_light_ctl_status status;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_ctl_set set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.params.light = cmd->light;
	set.params.temp = cmd->temp;
	set.params.delta_uv = cmd->delta_uv;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_light_ctl_set(&light_ctl_cli, &ctx, &set,
					    &status);
	} else {
		err = bt_mesh_light_ctl_set_unack(&light_ctl_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current_light);
		net_buf_simple_add_le16(buf, status.current_temp);
		net_buf_simple_add_le16(buf, status.target_light);
		net_buf_simple_add_le16(buf, status.target_temp);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_STATES_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_STATES_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_ctl_temperature_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_temp_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_temp_get(&light_ctl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current.temp);
	net_buf_simple_add_le16(buf, status.current.delta_uv);
	net_buf_simple_add_le16(buf, status.target.temp);
	net_buf_simple_add_le16(buf, status.target.delta_uv);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_GET,
		   CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void light_ctl_temperature_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_ctl_temperature_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct bt_mesh_light_temp_status status;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_temp_set set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.params.temp = cmd->temp;
	set.params.delta_uv = cmd->delta_uv;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	LOG_DBG("tt=%u delay=%u", transition.time, transition.delay);

	if (cmd->ack) {
		err = bt_mesh_light_temp_set(&light_ctl_cli, &ctx, &set,
					     &status);
	} else {
		err = bt_mesh_light_temp_set_unack(&light_ctl_cli, &ctx, &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current.temp);
		net_buf_simple_add_le16(buf, status.current.delta_uv);
		net_buf_simple_add_le16(buf, status.target.temp);
		net_buf_simple_add_le16(buf, status.target.delta_uv);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_ctl_default_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_ctl status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_ctl_default_get(&light_ctl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.light);
	net_buf_simple_add_le16(buf, status.temp);
	net_buf_simple_add_le16(buf, status.delta_uv);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_DEFAULT_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_DEFAULT_GET,
		   CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}
static void light_ctl_default_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_ctl_default_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_light_ctl status;
	struct bt_mesh_light_ctl set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.light = cmd->light;
	set.temp = cmd->temp;
	set.delta_uv = cmd->delta_uv;

	if (cmd->ack) {
		err = bt_mesh_light_ctl_default_set(&light_ctl_cli, &ctx, &set,
						    &status);
	} else {
		err = bt_mesh_light_ctl_default_set_unack(&light_ctl_cli, &ctx,
							  &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.light);
		net_buf_simple_add_le16(buf, status.temp);
		net_buf_simple_add_le16(buf, status.delta_uv);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_DEFAULT_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_DEFAULT_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_ctl_temp_range_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_temp_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(5);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_temp_range_get(&light_ctl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.status);
	net_buf_simple_add_le16(buf, status.range.min);
	net_buf_simple_add_le16(buf, status.range.max);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_RANGE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_RANGE_GET,
		   CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}
static void light_ctl_temp_range_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_ctl_temp_range_set *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(5);
	struct bt_mesh_light_temp_range_status status;
	struct bt_mesh_light_temp_range set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.min = cmd->min;
	set.max = cmd->max;

	if (cmd->ack) {
		err = bt_mesh_light_temp_range_set(&light_ctl_cli, &ctx, &set,
						   &status);
	} else {
		err = bt_mesh_light_temp_range_set_unack(&light_ctl_cli, &ctx,
							 &set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.status);
		net_buf_simple_add_le16(buf, status.range.min);
		net_buf_simple_add_le16(buf, status.range.max);

		tester_send(BTP_SERVICE_ID_MMDL,
			    MMDL_LIGHT_CTL_TEMPERATURE_RANGE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_CTL_TEMPERATURE_RANGE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void scene_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_scene_state status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCENE_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_scene_cli_get(&scene_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.status);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCENE_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCENE_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void scene_register_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_scene_register status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(4);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCENE_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_scene_cli_register_get(&scene_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.status);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_u8(buf, status.count);
	net_buf_simple_add_mem(buf, &status.scenes, status.count);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCENE_REGISTER_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCENE_REGISTER_GET,
		   CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void scene_store_procedure(uint8_t *data, uint16_t len)
{
	struct mesh_scene_ctl_store_procedure *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(3);
	uint16_t scene;
	struct bt_mesh_scene_register status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCENE_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	scene = cmd->scene;

	if (cmd->ack) {
		err = bt_mesh_scene_cli_store(&scene_cli, &ctx, scene, &status);
	} else {
		err = bt_mesh_scene_cli_store_unack(&scene_cli, &ctx, scene);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.status);
		net_buf_simple_add_le16(buf, status.current);
		if (err) {
			LOG_ERR("Cannot encode");
			goto fail;
		}

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCENE_STORE_PROCEDURE,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCENE_STORE_PROCEDURE,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void scene_recall(uint8_t *data, uint16_t len)
{
	struct mesh_scene_ctl_recall *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	uint16_t scene;
	struct bt_mesh_scene_state status;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_model_transition *set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCENE_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	scene = cmd->scene;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set = &transition;
	} else {
		set = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_scene_cli_recall(&scene_cli, &ctx, scene, set,
					       &status);
	} else {
		err = bt_mesh_scene_cli_recall_unack(&scene_cli, &ctx, scene,
						     set);
	}
	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.status);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCENE_RECALL,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCENE_RECALL, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_xyl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_xyl_get(&xyl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.params.lightness);
	net_buf_simple_add_le16(buf, status.params.xy.x);
	net_buf_simple_add_le16(buf, status.params.xy.y);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_GET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_xyl_set *cmd = (void *)data;
	struct bt_mesh_light_xyl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_xyl_set_params set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.params.lightness = cmd->lightness;
	set.params.xy.x = cmd->x;
	set.params.xy.y = cmd->y;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_light_xyl_set(&xyl_cli, &ctx, &set, &status);
	} else {
		err = bt_mesh_light_xyl_set_unack(&xyl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.params.lightness);
		net_buf_simple_add_le16(buf, status.params.xy.x);
		net_buf_simple_add_le16(buf, status.params.xy.y);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_target_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_xyl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_xyl_target_get(&xyl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.params.lightness);
	net_buf_simple_add_le16(buf, status.params.xy.x);
	net_buf_simple_add_le16(buf, status.params.xy.y);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_TARGET_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_TARGET_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_default_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_xyl status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_xyl_default_get(&xyl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.lightness);
	net_buf_simple_add_le16(buf, status.xy.x);
	net_buf_simple_add_le16(buf, status.xy.y);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_DEFAULT_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_DEFAULT_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_default_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_xyl_set *cmd = (void *)data;
	struct bt_mesh_light_xyl status;
	struct bt_mesh_light_xyl set;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lightness = cmd->lightness;
	set.xy.x = cmd->x;
	set.xy.y = cmd->y;

	if (cmd->ack) {
		err = bt_mesh_light_xyl_default_set(&xyl_cli, &ctx, &set,
						    &status);
	} else {
		err = bt_mesh_light_xyl_default_set_unack(&xyl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.lightness);
		net_buf_simple_add_le16(buf, status.xy.x);
		net_buf_simple_add_le16(buf, status.xy.y);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_DEFAULT_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_DEFAULT_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_range_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_xyl_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_xyl_range_get(&xyl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.status_code);
	net_buf_simple_add_le16(buf, status.range.min.x);
	net_buf_simple_add_le16(buf, status.range.min.y);
	net_buf_simple_add_le16(buf, status.range.max.x);
	net_buf_simple_add_le16(buf, status.range.max.y);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_RANGE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_RANGE_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_xyl_range_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_xyl_range_set *cmd = (void *)data;
	struct bt_mesh_light_xyl_range_status status;
	struct bt_mesh_light_xy_range set;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_XYL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.min.x = cmd->min_x;
	set.min.y = cmd->min_y;
	set.max.x = cmd->max_x;
	set.max.y = cmd->max_y;

	if (cmd->ack) {
		err = bt_mesh_light_xyl_range_set(&xyl_cli, &ctx, &set,
						  &status);
	} else {
		err = bt_mesh_light_xyl_range_set_unack(&xyl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.status_code);
		net_buf_simple_add_le16(buf, status.range.min.x);
		net_buf_simple_add_le16(buf, status.range.min.y);
		net_buf_simple_add_le16(buf, status.range.max.x);
		net_buf_simple_add_le16(buf, status.range.max.y);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_RANGE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_XYL_RANGE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_hsl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_hsl_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.params.lightness);
	net_buf_simple_add_le16(buf, status.params.hue);
	net_buf_simple_add_le16(buf, status.params.saturation);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_GET, CONTROLLER_INDEX,
		    buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_GET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_hsl_set *cmd = (void *)data;
	struct bt_mesh_light_hsl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_hsl_params set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.params.lightness = cmd->lightness;
	set.params.hue = cmd->hue;
	set.params.saturation = cmd->saturation;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_light_hsl_set(&light_hsl_cli, &ctx, &set,
					    &status);
	} else {
		err = bt_mesh_light_hsl_set_unack(&light_hsl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.params.lightness);
		net_buf_simple_add_le16(buf, status.params.hue);
		net_buf_simple_add_le16(buf, status.params.saturation);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_target_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_hsl_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_hsl_target_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.params.lightness);
	net_buf_simple_add_le16(buf, status.params.hue);
	net_buf_simple_add_le16(buf, status.params.saturation);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_TARGET_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_TARGET_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_default_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_hsl status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_hsl_default_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.lightness);
	net_buf_simple_add_le16(buf, status.hue);
	net_buf_simple_add_le16(buf, status.saturation);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_DEFAULT_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_DEFAULT_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_default_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_hsl_default_set *cmd = (void *)data;
	struct bt_mesh_light_hsl status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(6);
	struct bt_mesh_light_hsl set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lightness = cmd->lightness;
	set.hue = cmd->hue;
	set.saturation = cmd->saturation;

	if (cmd->ack) {
		err = bt_mesh_light_hsl_default_set(&light_hsl_cli, &ctx, &set,
						    &status);
	} else {
		err = bt_mesh_light_hsl_default_set_unack(&light_hsl_cli, &ctx,
							  &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.lightness);
		net_buf_simple_add_le16(buf, status.hue);
		net_buf_simple_add_le16(buf, status.saturation);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_DEFAULT_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_DEFAULT_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_range_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_hsl_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_hsl_range_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.status_code);
	net_buf_simple_add_le16(buf, status.range.min.hue);
	net_buf_simple_add_le16(buf, status.range.min.saturation);
	net_buf_simple_add_le16(buf, status.range.max.hue);
	net_buf_simple_add_le16(buf, status.range.max.saturation);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_RANGE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_RANGE_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_range_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_hsl_range_set *cmd = (void *)data;
	struct bt_mesh_light_hsl_range_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(9);
	struct bt_mesh_light_hue_sat_range set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.min.hue = cmd->hue_min;
	set.min.saturation = cmd->saturation_min;
	set.max.hue = cmd->hue_max;
	set.max.saturation = cmd->saturation_max;

	if (cmd->ack) {
		err = bt_mesh_light_hsl_range_set(&light_hsl_cli, &ctx, &set,
						  &status);
	} else {
		err = bt_mesh_light_hsl_range_set_unack(&light_hsl_cli, &ctx,
							&set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.status_code);
		net_buf_simple_add_le16(buf, status.range.min.hue);
		net_buf_simple_add_le16(buf, status.range.min.saturation);
		net_buf_simple_add_le16(buf, status.range.max.hue);
		net_buf_simple_add_le16(buf, status.range.max.saturation);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_RANGE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_RANGE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_hue_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_hue_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_hue_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_HUE_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_HUE_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_hue_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_hsl_hue_set *cmd = (void *)data;
	struct bt_mesh_light_hue_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_hue set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lvl = cmd->hue;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_light_hue_set(&light_hsl_cli, &ctx, &set,
					    &status);
	} else {
		err = bt_mesh_light_hue_set_unack(&light_hsl_cli, &ctx, &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_HUE_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_HUE_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_saturation_get(uint8_t *data, uint16_t len)
{
	struct bt_mesh_light_sat_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(8);
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_light_saturation_get(&light_hsl_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_le16(buf, status.current);
	net_buf_simple_add_le16(buf, status.target);
	net_buf_simple_add_le32(buf, status.remaining_time);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SATURATION_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SATURATION_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void light_hsl_saturation_set(uint8_t *data, uint16_t len)
{
	struct mesh_light_hsl_saturation_set *cmd = (void *)data;
	struct bt_mesh_light_sat_status status;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(10);
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_sat set;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	set.lvl = cmd->saturation;

	if (len > sizeof(*cmd)) {
		transition.time =
			model_transition_decode(cmd->transition->time);
		transition.delay = model_delay_decode(cmd->transition->delay);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	if (cmd->ack) {
		err = bt_mesh_light_saturation_set(&light_hsl_cli, &ctx, &set,
						   &status);
	} else {
		err = bt_mesh_light_saturation_set_unack(&light_hsl_cli, &ctx,
							 &set);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_le16(buf, status.current);
		net_buf_simple_add_le16(buf, status.target);
		net_buf_simple_add_le32(buf, status.remaining_time);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SATURATION_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_LIGHT_HSL_SATURATION_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void scheduler_get(uint8_t *data, uint16_t len)
{
	uint16_t status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCHEDULER_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_scheduler_cli_get(&scheduler_cli, &ctx, &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_GET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_GET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}
static void scheduler_action_get(uint8_t *data, uint16_t len)
{
	struct mesh_scheduler_action_get *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(11);
	struct bt_mesh_schedule_entry status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCHEDULER_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_scheduler_cli_action_get(&scheduler_cli, &ctx, cmd->index,
					       &status);

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status.year);
	net_buf_simple_add_u8(buf, status.month);
	net_buf_simple_add_u8(buf, status.day);
	net_buf_simple_add_u8(buf, status.hour);
	net_buf_simple_add_u8(buf, status.minute);
	net_buf_simple_add_u8(buf, status.second);
	net_buf_simple_add_u8(buf, status.day_of_week);
	net_buf_simple_add_u8(buf, status.action);
	net_buf_simple_add_u8(buf, status.transition_time);
	net_buf_simple_add_le16(buf, status.scene_number);

	tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_ACTION_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_ACTION_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}
static void scheduler_action_set(uint8_t *data, uint16_t len)
{
	struct mesh_scheduler_action_set *cmd = (void *)data;
	struct bt_mesh_schedule_entry entry;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(12);
	struct bt_mesh_schedule_entry status;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_SCHEDULER_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	entry.year = cmd->year;
	entry.month = cmd->month;
	entry.day = cmd->day;
	entry.hour = cmd->hour;
	entry.minute = cmd->minute;
	entry.second = cmd->second;
	entry.day_of_week = cmd->day_of_week;
	entry.action = cmd->action;
	entry.transition_time = cmd->transition_time;
	entry.scene_number = cmd->scene_number;

	if (cmd->ack) {
		err = bt_mesh_scheduler_cli_action_set(
			&scheduler_cli, &ctx, cmd->index, &entry, &status);
	} else {
		err = bt_mesh_scheduler_cli_action_set_unack(
			&scheduler_cli, &ctx, cmd->index, &entry);
	}

	if (err) {
		LOG_ERR("err=%d", err);
		goto fail;
	}
	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, status.year);
		net_buf_simple_add_le16(buf, status.month);
		net_buf_simple_add_u8(buf, status.day);
		net_buf_simple_add_u8(buf, status.hour);
		net_buf_simple_add_u8(buf, status.minute);
		net_buf_simple_add_u8(buf, status.second);
		net_buf_simple_add_u8(buf, status.day_of_week);
		net_buf_simple_add_u8(buf, status.action);
		net_buf_simple_add_u8(buf, status.transition_time);
		net_buf_simple_add_le16(buf, status.scene_number);

		tester_send(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_ACTION_SET,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}
fail:
	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SCHEDULER_ACTION_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

void tester_handle_mmdl(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len)
{
	switch (opcode) {
	case MMDL_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		break;
	case MMDL_GEN_ONOFF_GET:
		gen_onoff_get(data, len);
		break;
	case MMDL_GEN_ONOFF_SET:
		gen_onoff_set(data, len);
		break;
	case MMDL_GEN_LVL_GET:
		gen_lvl_get(data, len);
		break;
	case MMDL_GEN_LVL_SET:
		gen_lvl_set(data, len);
		break;
	case MMDL_GEN_LVL_DELTA_SET:
		gen_lvl_delta_set(data, len);
		break;
	case MMDL_GEN_LVL_MOVE_SET:
		gen_lvl_move_set(data, len);
		break;
	case MMDL_GEN_DTT_GET:
		gen_dtt_get(data, len);
		break;
	case MMDL_GEN_DTT_SET:
		gen_dtt_set(data, len);
		break;
	case MMDL_GEN_PONOFF_GET:
		gen_ponoff_get(data, len);
		break;
	case MMDL_GEN_PONOFF_SET:
		gen_ponoff_set(data, len);
		break;
	case MMDL_GEN_PLVL_GET:
		gen_plvl_get(data, len);
		break;
	case MMDL_GEN_PLVL_SET:
		gen_plvl_set(data, len);
		break;
	case MMDL_GEN_PLVL_LAST_GET:
		gen_plvl_last_get(data, len);
		break;
	case MMDL_GEN_PLVL_DFLT_GET:
		gen_plvl_dflt_get(data, len);
		break;
	case MMDL_GEN_PLVL_DFLT_SET:
		gen_plvl_dflt_set(data, len);
		break;
	case MMDL_GEN_PLVL_RANGE_GET:
		gen_plvl_range_get(data, len);
		break;
	case MMDL_GEN_PLVL_RANGE_SET:
		gen_plvl_range_set(data, len);
		break;
	case MMDL_GEN_BATTERY_GET:
		battery_get(data, len);
		break;
	case MMDL_GEN_LOC_GLOBAL_GET:
		location_global_get(data, len);
		break;
	case MMDL_GEN_LOC_LOCAL_GET:
		location_local_get(data, len);
		break;
	case MMDL_GEN_LOC_GLOBAL_SET:
		location_global_set(data, len);
		break;
	case MMDL_GEN_LOC_LOCAL_SET:
		location_local_set(data, len);
		break;
	case MMDL_GEN_PROPS_GET:
		gen_props_get(data, len);
		break;
	case MMDL_GEN_PROP_GET:
		gen_prop_get(data, len);
		break;
	case MMDL_GEN_PROP_SET:
		gen_prop_set(data, len);
		break;
	case MMDL_SENSOR_DESC_GET:
		sensor_desc_get(data, len);
		break;
	case MMDL_SENSOR_GET:
		sensor_get(data, len);
		break;
	case MMDL_SENSOR_CADENCE_GET:
		sensor_cadence_get(data, len);
		break;
	case MMDL_SENSOR_CADENCE_SET:
		sensor_cadence_set(data, len);
		break;
	case MMDL_SENSOR_SETTINGS_GET:
		sensor_settings_get(data, len);
		break;
	case MMDL_SENSOR_SETTING_GET:
		sensor_setting_get(data, len);
		break;
	case MMDL_SENSOR_SETTING_SET:
		sensor_setting_set(data, len);
		break;
	case MMDL_SENSOR_COLUMN_GET:
		sensor_column_get(data, len);
		break;
	case MMDL_SENSOR_SERIES_GET:
		sensor_series_get(data, len);
		break;
	case MMDL_TIME_GET:
		time_get(data, len);
		break;
	case MMDL_TIME_SET:
		time_set(data, len);
		break;
	case MMDL_TIME_ROLE_GET:
		time_role_get(data, len);
		break;
	case MMDL_TIME_ROLE_SET:
		time_role_set(data, len);
		break;
	case MMDL_TIME_ZONE_GET:
		time_zone_get(data, len);
		break;
	case MMDL_TIME_ZONE_SET:
		time_zone_set(data, len);
		break;
	case MMDL_TIME_TAI_UTC_DELTA_GET:
		time_tai_utc_delta_get(data, len);
		break;
	case MMDL_TIME_TAI_UTC_DELTA_SET:
		time_tai_utc_delta_set(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_GET:
		light_lightness_get(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_SET:
		light_lightness_set(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_LINEAR_GET:
		light_lightness_linear_get(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_LINEAR_SET:
		light_lightness_linear_set(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_LAST_GET:
		light_lightness_last_get(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_DEFAULT_GET:
		light_lightness_default_get(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_DEFAULT_SET:
		light_lightness_default_set(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_RANGE_GET:
		light_lightness_range_get(data, len);
		break;
	case MMDL_LIGHT_LIGHTNESS_RANGE_SET:
		light_lightness_range_set(data, len);
		break;
	case MMDL_LIGHT_LC_MODE_GET:
		light_lc_mode_get(data, len);
		break;
	case MMDL_LIGHT_LC_MODE_SET:
		light_lc_mode_set(data, len);
		break;
	case MMDL_LIGHT_LC_OCCUPANCY_MODE_GET:
		light_lc_occupancy_mode_get(data, len);
		break;
	case MMDL_LIGHT_LC_OCCUPANCY_MODE_SET:
		light_lc_occupancy_mode_set(data, len);
		break;
	case MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_GET:
		light_lc_light_onoff_mode_get(data, len);
		break;
	case MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_SET:
		light_lc_light_onoff_mode_set(data, len);
		break;
	case MMDL_LIGHT_LC_PROPERTY_GET:
		light_lc_property_get(data, len);
		break;
	case MMDL_LIGHT_LC_PROPERTY_SET:
		light_lc_property_set(data, len);
		break;
	case MMDL_SENSOR_DATA_SET:
		sensor_data_set(data, len);
		break;
	case MMDL_LIGHT_CTL_STATES_GET:
		light_ctl_states_get(data, len);
		break;
	case MMDL_LIGHT_CTL_STATES_SET:
		light_ctl_states_set(data, len);
		break;
	case MMDL_LIGHT_CTL_TEMPERATURE_GET:
		light_ctl_temperature_get(data, len);
		break;
	case MMDL_LIGHT_CTL_TEMPERATURE_SET:
		light_ctl_temperature_set(data, len);
		break;
	case MMDL_LIGHT_CTL_DEFAULT_GET:
		light_ctl_default_get(data, len);
		break;
	case MMDL_LIGHT_CTL_DEFAULT_SET:
		light_ctl_default_set(data, len);
		break;
	case MMDL_LIGHT_CTL_TEMPERATURE_RANGE_GET:
		light_ctl_temp_range_get(data, len);
		break;
	case MMDL_LIGHT_CTL_TEMPERATURE_RANGE_SET:
		light_ctl_temp_range_set(data, len);
		break;
	case MMDL_SCENE_GET:
		scene_get(data, len);
		break;
	case MMDL_SCENE_REGISTER_GET:
		scene_register_get(data, len);
		break;
	case MMDL_SCENE_STORE_PROCEDURE:
		scene_store_procedure(data, len);
		break;
	case MMDL_SCENE_RECALL:
		scene_recall(data, len);
		break;
	case MMDL_LIGHT_XYL_GET:
		light_xyl_get(data, len);
		break;
	case MMDL_LIGHT_XYL_SET:
		light_xyl_set(data, len);
		break;
	case MMDL_LIGHT_XYL_TARGET_GET:
		light_xyl_target_get(data, len);
		break;
	case MMDL_LIGHT_XYL_DEFAULT_GET:
		light_xyl_default_get(data, len);
		break;
	case MMDL_LIGHT_XYL_DEFAULT_SET:
		light_xyl_default_set(data, len);
		break;
	case MMDL_LIGHT_XYL_RANGE_GET:
		light_xyl_range_get(data, len);
		break;
	case MMDL_LIGHT_XYL_RANGE_SET:
		light_xyl_range_set(data, len);
		break;
	case MMDL_LIGHT_HSL_GET:
		light_hsl_get(data, len);
		break;
	case MMDL_LIGHT_HSL_SET:
		light_hsl_set(data, len);
		break;
	case MMDL_LIGHT_HSL_TARGET_GET:
		light_hsl_target_get(data, len);
		break;
	case MMDL_LIGHT_HSL_DEFAULT_GET:
		light_hsl_default_get(data, len);
		break;
	case MMDL_LIGHT_HSL_DEFAULT_SET:
		light_hsl_default_set(data, len);
		break;
	case MMDL_LIGHT_HSL_RANGE_GET:
		light_hsl_range_get(data, len);
		break;
	case MMDL_LIGHT_HSL_RANGE_SET:
		light_hsl_range_set(data, len);
		break;
	case MMDL_LIGHT_HSL_HUE_GET:
		light_hsl_hue_get(data, len);
		break;
	case MMDL_LIGHT_HSL_HUE_SET:
		light_hsl_hue_set(data, len);
		break;
	case MMDL_LIGHT_HSL_SATURATION_GET:
		light_hsl_saturation_get(data, len);
		break;
	case MMDL_LIGHT_HSL_SATURATION_SET:
		light_hsl_saturation_set(data, len);
		break;
	case MMDL_SCHEDULER_GET:
		scheduler_get(data, len);
		break;
	case MMDL_SCHEDULER_ACTION_GET:
		scheduler_action_get(data, len);
		break;
	case MMDL_SCHEDULER_ACTION_SET:
		scheduler_action_set(data, len);
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_MMDL, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

uint8_t tester_init_mmdl(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mmdl(void)
{
	return BTP_STATUS_SUCCESS;
}
