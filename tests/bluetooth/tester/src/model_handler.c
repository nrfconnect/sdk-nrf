/* model_handler.c - Bluetooth Mesh Models handler */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>

#include <logging/log.h>
#define LOG_MODULE_NAME bttester_model_handler
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"
#include "model_handler.h"

/* Vendor Model data */
#define VND_MODEL_ID_1 0x1234
uint8_t cur_faults[CUR_FAULTS_MAX];
uint8_t reg_faults[CUR_FAULTS_MAX * 2];

#define PWM_SIZE_STEP 64

static void pwr_set(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_plvl_set *set,
		    struct bt_mesh_plvl_status *rsp);

static void pwr_get(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_plvl_status *rsp);

static struct bt_mesh_plvl_srv_handlers plvl_handlers = {
	.power_set = pwr_set,
	.power_get = pwr_get,
};

struct lvl_ctx {
	struct bt_mesh_plvl_srv srv;
	struct k_delayed_work work;
	uint32_t remaining;
	uint32_t period;
	uint16_t current;
	uint16_t target;
} lvl_ctx = {
	.srv = BT_MESH_PLVL_SRV_INIT(&plvl_handlers),
};

static void start_new_trans(uint32_t step_cnt,
			    const struct bt_mesh_model_transition *transition,
			    struct lvl_ctx *ctx)
{
	k_delayed_work_cancel(&ctx->work);
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_delayed_work_submit(&ctx->work, K_MSEC(transition->delay));
}

static void lvl_status(struct lvl_ctx *ctx, struct bt_mesh_plvl_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void periodic_led_work(struct k_work *work)
{
	struct lvl_ctx *ctx = CONTAINER_OF(work, struct lvl_ctx, work);

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_plvl_status status;

		lvl_status(ctx, &status);
		bt_mesh_plvl_srv_pub(&ctx->srv, NULL, &status);
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_delayed_work_submit(&ctx->work, K_MSEC(ctx->period));
}

static void pwr_set(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_plvl_set *set,
		    struct bt_mesh_plvl_status *rsp)
{
	struct lvl_ctx *l_ctx = CONTAINER_OF(srv, struct lvl_ctx, srv);
	uint32_t step_cnt;

	l_ctx->target = set->power_lvl;
	l_ctx->remaining = set->transition->time + set->transition->delay;
	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}

	lvl_status(l_ctx, rsp);
}

static void pwr_get(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_plvl_status *rsp)
{
	struct lvl_ctx *l_ctx = CONTAINER_OF(srv, struct lvl_ctx, srv);

	lvl_status(l_ctx, rsp);
}

static void battery_get(struct bt_mesh_battery_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			struct bt_mesh_battery_status *rsp);

struct bat_ctx {
	struct bt_mesh_battery_srv srv;
} bat_ctx = {
	.srv = BT_MESH_BATTERY_SRV_INIT(battery_get),
};

static void battery_get(struct bt_mesh_battery_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			struct bt_mesh_battery_status *rsp)
{
	rsp->battery_lvl = BT_MESH_BATTERY_LVL_UNKNOWN;
	rsp->discharge_minutes = BT_MESH_BATTERY_TIME_UNKNOWN;
	rsp->charge_minutes = BT_MESH_BATTERY_TIME_UNKNOWN;
	rsp->presence = BT_MESH_BATTERY_PRESENCE_UNKNOWN;
	rsp->indicator = BT_MESH_BATTERY_INDICATOR_UNKNOWN;
	rsp->charging = BT_MESH_BATTERY_CHARGING_UNKNOWN;
	rsp->service = BT_MESH_BATTERY_SERVICE_UNKNOWN;
}

static void global_get(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_loc_global *rsp);

static void global_set(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_loc_global *loc);

static void local_get(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_loc_local *rsp);

static void local_set(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_loc_local *loc);

static struct bt_mesh_loc_srv_handlers loc_handlers = {
	.global_get = global_get,
	.global_set = global_set,
	.local_get = local_get,
	.local_set = local_set,
};

struct loc_ctx {
	struct bt_mesh_loc_srv srv;
	struct bt_mesh_loc_global global;
	struct bt_mesh_loc_local local;
} loc_ctx = {
	.srv = BT_MESH_LOC_SRV_INIT(&loc_handlers),
};

static void global_get(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_loc_global *rsp)
{
	struct loc_ctx *l_ctx = CONTAINER_OF(srv, struct loc_ctx, srv);

	*rsp = l_ctx->global;
}

static void global_set(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_loc_global *loc)
{
	struct loc_ctx *l_ctx = CONTAINER_OF(srv, struct loc_ctx, srv);

	l_ctx->global = *loc;
}

static void local_get(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_loc_local *rsp)
{
	struct loc_ctx *l_ctx = CONTAINER_OF(srv, struct loc_ctx, srv);

	*rsp = l_ctx->local;
}

static void local_set(struct bt_mesh_loc_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_loc_local *loc)
{
	struct loc_ctx *l_ctx = CONTAINER_OF(srv, struct loc_ctx, srv);

	l_ctx->local = *loc;
}

static void prop_set(struct bt_mesh_prop_srv *srv, struct bt_mesh_msg_ctx *ctx,
		     struct bt_mesh_prop_val *val);

static void prop_get(struct bt_mesh_prop_srv *srv, struct bt_mesh_msg_ctx *ctx,
		     struct bt_mesh_prop_val *val);

static void prop_mfr_get(struct bt_mesh_prop_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_prop_val *val);

#define PROP_ID 2U
#define PROP_MFR_ID 3U

static struct bt_mesh_prop
	properties[] = { [0] = {
				 .id = PROP_ID,
				 .user_access = BT_MESH_PROP_ACCESS_READ_WRITE,
			 } };

static struct bt_mesh_prop
	mfr_properties[] = { [0] = {
				     .id = PROP_MFR_ID,
				     .user_access = BT_MESH_PROP_ACCESS_READ,
			     } };

static struct {
	uint16_t current;
	uint8_t sensing_duration;
} __attribute__((__packed__)) property;

struct prop_ctx {
	struct bt_mesh_prop_srv admin_srv;
	struct bt_mesh_prop_srv cli_srv;
	struct bt_mesh_prop_srv usr_srv;
	struct bt_mesh_prop_srv mfr_srv;
} prop_ctx = {
	.admin_srv =
		BT_MESH_PROP_SRV_ADMIN_INIT(properties, prop_get, prop_set),
	.cli_srv = BT_MESH_PROP_SRV_CLIENT_INIT(properties),
	.usr_srv = BT_MESH_PROP_SRV_USER_INIT(),
	.mfr_srv = BT_MESH_PROP_SRV_MFR_INIT(mfr_properties, prop_mfr_get),
};

static void prop_set(struct bt_mesh_prop_srv *srv, struct bt_mesh_msg_ctx *ctx,
		     struct bt_mesh_prop_val *val)
{
	if ((val->meta.id != PROP_ID) || (val->size != sizeof(property))) {
		val->meta.id = BT_MESH_PROP_ID_PROHIBITED;
		return;
	}

	property.current = (val->value[1] << 8) | val->value[0];
	property.sensing_duration = val->value[2];
}

static void prop_get(struct bt_mesh_prop_srv *srv, struct bt_mesh_msg_ctx *ctx,
		     struct bt_mesh_prop_val *val)
{
	if (val->meta.id != PROP_ID) {
		val->meta.user_access = BT_MESH_PROP_ACCESS_PROHIBITED;
		return;
	}

	val->size = sizeof(property);
	memcpy(val->value, &property.current, sizeof(property));
}

static void prop_mfr_get(struct bt_mesh_prop_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_prop_val *val)
{
	if (val->meta.id != PROP_MFR_ID) {
		val->meta.user_access = BT_MESH_PROP_ACCESS_PROHIBITED;
		return;
	}

	val->size = sizeof(property);
	memcpy(val->value, &property.current, sizeof(property));
}

static int sensor_data_get(struct bt_mesh_sensor *sensor,
			   struct bt_mesh_msg_ctx *ctx,
			   struct sensor_value *rsp);

static struct bt_mesh_sensor time_since_presence_detected = {
	.type = &bt_mesh_sensor_time_since_presence_detected,
	.get = sensor_data_get,
};

static struct bt_mesh_sensor *const sensors[] = {
	&time_since_presence_detected,
};

static struct sensor_value values[ARRAY_SIZE(sensors)];

static struct bt_mesh_sensor_srv sensor_srv =
	BT_MESH_SENSOR_SRV_INIT(sensors, ARRAY_SIZE(sensors));

static int sensor_data_get(struct bt_mesh_sensor *sensor,
			   struct bt_mesh_msg_ctx *ctx,
			   struct sensor_value *rsp)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sensors); ++i) {
		if (sensor->type->id != sensors[i]->type->id) {
			continue;
		}

		memcpy(rsp, &values[i], sizeof(*rsp));
		break;
	}

	return 0;
}

void sensor_data_set(uint8_t *data, uint16_t len)
{
	struct mesh_sensor_data_set_cmd *cmd = (void *)data;
	int i;

	for (i = 0; i < ARRAY_SIZE(sensors); ++i) {
		if (cmd->prop_id != sensors[i]->type->id) {
			continue;
		}

		if ((cmd->len > 0) && (cmd->len < sizeof(struct sensor_value))) {
			memcpy(&values[i], cmd->data, cmd->len);
			bt_mesh_sensor_srv_sample(&sensor_srv, sensors[i]);
		}
		break;
	}

	tester_rsp(BTP_SERVICE_ID_MMDL, MMDL_SENSOR_DATA_SET, 0,
		   BTP_STATUS_SUCCESS);
}

static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);

struct lightness_ctx {
	struct bt_mesh_lightness_srv srv;
	struct k_delayed_work work;
	uint32_t remaining;
	uint32_t period;
	uint16_t current;
	uint16_t target;
};

static void
start_new_lightness_trans(uint32_t step_cnt,
			  const struct bt_mesh_model_transition *transition,
			  struct lightness_ctx *ctx)
{
	k_delayed_work_cancel(&ctx->work);
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_delayed_work_submit(&ctx->work, K_MSEC(transition->delay));
}

static void lightness_status(struct lightness_ctx *ctx,
			     struct bt_mesh_lightness_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void periodic_led_lightness_work(struct k_work *work)
{
	struct lightness_ctx *ctx =
		CONTAINER_OF(work, struct lightness_ctx, work);

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_lightness_status status;

		lightness_status(ctx, &status);
		bt_mesh_lightness_srv_pub(&ctx->srv, NULL, &status);
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_delayed_work_submit(&ctx->work, K_MSEC(ctx->period));
}

static void light_set(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_lightness_set *set,
		      struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx =
		CONTAINER_OF(srv, struct lightness_ctx, srv);
	uint32_t step_cnt;

	l_ctx->target = set->lvl;
	l_ctx->remaining = set->transition->time + set->transition->delay;
	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_lightness_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}

	lightness_status(l_ctx, rsp);
}

static void light_get(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx =
		CONTAINER_OF(srv, struct lightness_ctx, srv);

	lightness_status(l_ctx, rsp);
}

static const struct bt_mesh_lightness_srv_handlers lightness_srv_handlers = {
	.light_set = light_set,
	.light_get = light_get,
};

static struct lightness_ctx lightness_ctx = {
	.srv = BT_MESH_LIGHTNESS_SRV_INIT(&lightness_srv_handlers),
};

static struct bt_mesh_light_ctrl_srv light_ctrl_srv =
	BT_MESH_LIGHT_CTRL_SRV_INIT(&lightness_ctx.srv);

static void get_faults(uint8_t *faults, uint8_t faults_size, uint8_t *dst,
		       uint8_t *count)
{
	uint8_t i, limit = *count;

	for (i = 0U, *count = 0U; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int fault_get_cur(struct bt_mesh_model *model, uint8_t *test_id,
			 uint16_t *company_id, uint8_t *faults,
			 uint8_t *fault_count)
{
	LOG_DBG("");

	*test_id = HEALTH_TEST_ID;
	*company_id = CONFIG_BT_COMPANY_ID;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t company_id,
			 uint8_t *test_id, uint8_t *faults,
			 uint8_t *fault_count)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CONFIG_BT_COMPANY_ID) {
		return -EINVAL;
	}

	*test_id = HEALTH_TEST_ID;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CONFIG_BT_COMPANY_ID) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	LOG_DBG("test_id 0x%02x company_id 0x%04x", test_id, company_id);

	if (company_id != CONFIG_BT_COMPANY_ID || test_id != HEALTH_TEST_ID) {
		return -EINVAL;
	}

	return 0;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);

struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CONFIG_BT_COMPANY_ID, VND_MODEL_ID_1,
			  BT_MESH_MODEL_NO_OPS, NULL, NULL),
};

static struct bt_mesh_cfg_cli cfg_cli = {};

void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults,
		 size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x: no faults",
			test_id, cid);
		return;
	}

	LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu: ",
		test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		LOG_DBG("0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				  uint8_t test_id, uint16_t cid,
				  uint8_t *faults, size_t fault_count)
{
	LOG_DBG("Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_CFG_CLI(&cfg_cli),
					BT_MESH_MODEL_HEALTH_SRV(&health_srv,
								 &health_pub),
					BT_MESH_MODEL_HEALTH_CLI(&health_cli),
					BT_MESH_MODEL_PLVL_SRV(&lvl_ctx.srv)),
		     vnd_models),
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_BATTERY_SRV(&bat_ctx.srv),
			     BT_MESH_MODEL_LOC_SRV(&loc_ctx.srv),
			     BT_MESH_MODEL_BATTERY_SRV(&bat_ctx.srv),
			     BT_MESH_MODEL_PROP_SRV_ADMIN(&prop_ctx.admin_srv),
			     BT_MESH_MODEL_PROP_SRV_CLIENT(&prop_ctx.cli_srv),
			     BT_MESH_MODEL_PROP_SRV_USER(&prop_ctx.usr_srv),
			     BT_MESH_MODEL_PROP_SRV_MFR(&prop_ctx.mfr_srv),
			     BT_MESH_MODEL_SENSOR_SRV(&sensor_srv),
			     BT_MESH_MODEL_TIME_SRV(&time_srv),
			     BT_MESH_MODEL_LIGHTNESS_SRV(&lightness_ctx.srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(2,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_CTRL_SRV(&light_ctrl_srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(3,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_ONOFF_CLI(&onoff_cli),
			     BT_MESH_MODEL_LVL_CLI(&lvl_cli),
			     BT_MESH_MODEL_DTT_CLI(&dtt_cli),
			     BT_MESH_MODEL_PONOFF_CLI(&ponoff_cli),
			     BT_MESH_MODEL_PLVL_CLI(&plvl_cli),
			     BT_MESH_MODEL_BATTERY_CLI(&battery_cli),
			     BT_MESH_MODEL_LOC_CLI(&loc_cli),
			     BT_MESH_MODEL_PROP_CLI(&prop_cli),
			     BT_MESH_MODEL_SENSOR_CLI(&sensor_cli),
			     BT_MESH_MODEL_TIME_CLI(&time_cli),
			     BT_MESH_MODEL_LIGHTNESS_CLI(&lightness_cli),
			     BT_MESH_MODEL_LIGHT_CTRL_CLI(&light_ctrl_cli),
			     BT_MESH_MODEL_LIGHT_CTL_CLI(&light_ctl_cli),
			     BT_MESH_MODEL_SCENE_CLI(&scene_cli)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_delayed_work_init(&lvl_ctx.work, periodic_led_work);
	k_delayed_work_init(&lightness_ctx.work, periodic_led_lightness_work);
	return &comp;
}
