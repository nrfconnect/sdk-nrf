/* model_handler.c - Bluetooth Mesh Models handler */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>

#include <zephyr/logging/log.h>
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
	struct k_work_delayable work;
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
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
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

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void pwr_set(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_plvl_set *set,
		    struct bt_mesh_plvl_status *rsp)
{
	struct lvl_ctx *l_ctx = CONTAINER_OF(srv, struct lvl_ctx, srv);
	uint32_t step_cnt;

	l_ctx->target = set->power_lvl;
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

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
	memcpy(val->value, &property, sizeof(property));
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
	memcpy(val->value, &property, sizeof(property));
}

static int sensor_data_get(struct bt_mesh_sensor_srv *srv,
			   struct bt_mesh_sensor *sensor,
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

static int sensor_data_get(struct bt_mesh_sensor_srv *srv,
			   struct bt_mesh_sensor *sensor,
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

static struct bt_mesh_dtt_srv dtt_srv = BT_MESH_DTT_SRV_INIT(NULL);

static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);

static struct bt_mesh_scheduler_srv scheduler_srv =
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv);

struct lightness_ctx {
	struct bt_mesh_lightness_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	uint32_t period;
	uint16_t current;
	uint16_t target;
};

struct light_temp_ctx {
	struct bt_mesh_light_temp_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	uint32_t period;
	struct bt_mesh_light_temp current;
	struct bt_mesh_light_temp target;
};

struct light_ctl_ctx {
	struct bt_mesh_light_ctl_srv srv;
	struct lightness_ctx ctl_lightness_ctx;
	struct light_temp_ctx light_temp_ctx;
};

static void
start_new_lightness_trans(uint32_t step_cnt,
			  const struct bt_mesh_model_transition *transition,
			  struct lightness_ctx *ctx)
{
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
}

static void lightness_status(struct lightness_ctx *ctx,
			     struct bt_mesh_lightness_status *rsp)
{
	rsp->current = bt_mesh_lightness_clamp(&ctx->srv, ctx->current);
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void light_ctl_status(struct light_ctl_ctx *ctx,
			     struct bt_mesh_light_ctl_status *rsp)
{
	rsp->current_light = ctx->ctl_lightness_ctx.current;
	rsp->current_temp = ctx->light_temp_ctx.current.temp;
	rsp->target_light = ctx->ctl_lightness_ctx.target;
	rsp->target_temp = ctx->light_temp_ctx.target.temp;
	rsp->remaining_time = MAX(ctx->ctl_lightness_ctx.remaining,
				  ctx->light_temp_ctx.remaining);
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

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
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
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

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

static void
start_new_light_temp_trans(uint32_t step_cnt,
			   const struct bt_mesh_model_transition *transition,
			   struct light_temp_ctx *ctx)
{
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
}

static void light_temp_status(struct light_temp_ctx *ctx,
			      struct bt_mesh_light_temp_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void periodic_light_temp_work(struct k_work *work)
{
	struct light_temp_ctx *ctx =
		CONTAINER_OF(work, struct light_temp_ctx, work);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctx, struct light_ctl_ctx, light_temp_ctx);
	struct lightness_ctx *lightness_ctx = &ctl_ctx->ctl_lightness_ctx;

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target.temp - ctx->current.temp) <= PWM_SIZE_STEP)) {
		ctx->current.temp = ctx->target.temp;
		ctx->current.delta_uv = ctx->target.delta_uv;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_light_temp_status status;

		light_temp_status(ctx, &status);
		bt_mesh_light_temp_srv_pub(&ctl_ctx->srv.temp_srv, NULL,
					   &status);

		if (lightness_ctx->remaining == 0) {
			struct bt_mesh_light_ctl_status ctl_status;

			light_ctl_status(ctl_ctx, &ctl_status);
			bt_mesh_light_ctl_pub(&ctl_ctx->srv, NULL, &ctl_status);
		}
		return;
	} else if (ctx->target.temp > ctx->current.temp) {
		ctx->current.temp += PWM_SIZE_STEP;
	} else {
		ctx->current.temp -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void light_temp_set(struct bt_mesh_light_temp_srv *srv,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_light_temp_set *set,
			   struct bt_mesh_light_temp_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, temp_srv);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctl_srv, struct light_ctl_ctx, srv);
	struct light_temp_ctx *l_ctx = &ctl_ctx->light_temp_ctx;
	uint32_t step_cnt;

	l_ctx->target.temp = set->params.temp;
	l_ctx->target.delta_uv = set->params.delta_uv;
	if (set->transition) {
		l_ctx->remaining =
			set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target.temp - l_ctx->current.temp) /
			   PWM_SIZE_STEP;
		start_new_light_temp_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current.temp = l_ctx->target.temp;
		l_ctx->current.delta_uv = l_ctx->target.delta_uv;
	}
	light_temp_status(l_ctx, rsp);

	struct bt_mesh_light_ctl_status ctl_status;

	light_ctl_status(ctl_ctx, &ctl_status);
	bt_mesh_light_ctl_pub(&ctl_ctx->srv, NULL, &ctl_status);
}

static void light_temp_get(struct bt_mesh_light_temp_srv *srv,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_light_temp_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, temp_srv);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctl_srv, struct light_ctl_ctx, srv);
	struct light_temp_ctx *l_ctx = &ctl_ctx->light_temp_ctx;

	light_temp_status(l_ctx, rsp);
}

static const struct bt_mesh_light_temp_srv_handlers light_temp_srv_handlers = {
	.set = light_temp_set,
	.get = light_temp_get,
};

static void periodic_light_ctl_lightness_work(struct k_work *work)
{
	struct lightness_ctx *ctx =
		CONTAINER_OF(work, struct lightness_ctx, work);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctx, struct light_ctl_ctx, ctl_lightness_ctx);
	struct light_temp_ctx *temp_ctx = &ctl_ctx->light_temp_ctx;

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_lightness_status status;

		lightness_status(ctx, &status);
		bt_mesh_lightness_srv_pub(&ctl_ctx->srv.lightness_srv, NULL,
					  &status);

		if (temp_ctx->remaining == 0) {
			struct bt_mesh_light_ctl_status ctl_status;

			light_ctl_status(ctl_ctx, &ctl_status);
			bt_mesh_light_ctl_pub(&ctl_ctx->srv, NULL, &ctl_status);
		}
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void light_ctl_lightness_set(struct bt_mesh_lightness_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_set *set,
				    struct bt_mesh_lightness_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, lightness_srv);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctl_srv, struct light_ctl_ctx, srv);
	struct lightness_ctx *l_ctx = &ctl_ctx->ctl_lightness_ctx;
	uint32_t step_cnt;

	l_ctx->target = set->lvl;
	if (set->transition) {
		l_ctx->remaining =
			set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_lightness_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}

	lightness_status(l_ctx, rsp);

	struct bt_mesh_light_ctl_status ctl_status;

	light_ctl_status(ctl_ctx, &ctl_status);
	bt_mesh_light_ctl_pub(&ctl_ctx->srv, NULL, &ctl_status);
}

static void light_ctl_lightness_get(struct bt_mesh_lightness_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, lightness_srv);
	struct light_ctl_ctx *ctl_ctx =
		CONTAINER_OF(ctl_srv, struct light_ctl_ctx, srv);
	struct lightness_ctx *l_ctx = &ctl_ctx->ctl_lightness_ctx;

	lightness_status(l_ctx, rsp);
}

static const struct bt_mesh_lightness_srv_handlers ctl_lightness_srv_handlers = {
	.light_set = light_ctl_lightness_set,
	.light_get = light_ctl_lightness_get,
};

static struct light_ctl_ctx light_ctl_ctx = {
	.srv = BT_MESH_LIGHT_CTL_SRV_INIT(&ctl_lightness_srv_handlers,
					  &light_temp_srv_handlers),
};

struct light_hue_ctx {
	struct bt_mesh_light_hue_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	uint32_t period;
	uint16_t current;
	uint16_t target;
};

struct light_sat_ctx {
	struct bt_mesh_light_sat_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	uint32_t period;
	uint16_t current;
	uint16_t target;
};

struct light_hsl_ctx {
	struct bt_mesh_light_hsl_srv srv;
	struct light_hue_ctx light_hue_ctx;
	struct light_sat_ctx light_sat_ctx;
	struct lightness_ctx *lightness_ctx;
};

struct light_xy_ctx {
	struct bt_mesh_light_xyl_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	uint32_t period;
	struct bt_mesh_light_xy current;
	struct bt_mesh_light_xy target;
	struct lightness_ctx *lightness_ctx;
};

struct light_xyl_hsl_ctx {
	struct light_xy_ctx xyl_ctx;
	struct light_hsl_ctx hsl_ctx;
	struct lightness_ctx lightness_ctx;
};

static void light_hsl_status(struct light_hsl_ctx *ctx,
			     struct bt_mesh_light_hsl_status *rsp)
{
	rsp->params.hue = ctx->light_hue_ctx.current;
	rsp->params.saturation = ctx->light_sat_ctx.current;
	rsp->params.lightness = ctx->lightness_ctx->current;
	rsp->remaining_time = MAX(MAX(ctx->lightness_ctx->remaining,
				      ctx->light_hue_ctx.remaining),
				  ctx->light_sat_ctx.remaining);
}

static void
start_new_hue_trans(uint32_t step_cnt,
		    const struct bt_mesh_model_transition *transition,
		    struct light_hue_ctx *ctx)
{
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
}

static void hue_status(struct light_hue_ctx *ctx,
		       struct bt_mesh_light_hue_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void periodic_light_hue_work(struct k_work *work)
{
	struct light_hue_ctx *ctx =
		CONTAINER_OF(work, struct light_hue_ctx, work);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(ctx, struct light_hsl_ctx, light_hue_ctx);
	struct light_sat_ctx *sat_ctx = &hsl_ctx->light_sat_ctx;
	struct lightness_ctx *lightness_ctx = hsl_ctx->lightness_ctx;

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_light_hue_status status;

		hue_status(ctx, &status);
		bt_mesh_light_hue_srv_pub(&hsl_ctx->srv.hue, NULL, &status);
		if (sat_ctx->remaining == 0 && lightness_ctx->remaining == 0) {
			struct bt_mesh_light_hsl_status hsl_status;

			light_hsl_status(hsl_ctx, &hsl_status);
			bt_mesh_light_hsl_srv_pub(&hsl_ctx->srv, NULL, &hsl_status);
		}
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void hue_set(struct bt_mesh_light_hue_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_light_hue *set,
		    struct bt_mesh_light_hue_status *rsp)
{
	struct bt_mesh_light_hsl_srv *hsl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_hsl_srv, hue);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(hsl_srv, struct light_hsl_ctx, srv);
	struct light_hue_ctx *l_ctx = &hsl_ctx->light_hue_ctx;
	uint32_t step_cnt;

	l_ctx->target = set->lvl;
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_hue_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}
	hue_status(l_ctx, rsp);
}

static void hue_get(struct bt_mesh_light_hue_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_hue_status *rsp)
{
	struct bt_mesh_light_hsl_srv *hsl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_hsl_srv, hue);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(hsl_srv, struct light_hsl_ctx, srv);
	struct light_hue_ctx *l_ctx = &hsl_ctx->light_hue_ctx;

	hue_status(l_ctx, rsp);
}

static const struct bt_mesh_light_hue_srv_handlers hue_cb = {
	.set = hue_set,
	.get = hue_get,
};

static void
start_new_sat_trans(uint32_t step_cnt,
		    const struct bt_mesh_model_transition *transition,
		    struct light_sat_ctx *ctx)
{
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
}

static void light_sat_status(struct light_sat_ctx *ctx,
			     struct bt_mesh_light_sat_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void periodic_light_sat_work(struct k_work *work)
{
	struct light_sat_ctx *ctx =
		CONTAINER_OF(work, struct light_sat_ctx, work);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(ctx, struct light_hsl_ctx, light_sat_ctx);
	struct light_hue_ctx *hue_ctx = &hsl_ctx->light_hue_ctx;
	struct lightness_ctx *lightness_ctx = hsl_ctx->lightness_ctx;

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_light_sat_status status;

		light_sat_status(ctx, &status);
		bt_mesh_light_sat_srv_pub(&hsl_ctx->srv.sat, NULL, &status);
		if (hue_ctx->remaining == 0 && lightness_ctx->remaining == 0) {
			struct bt_mesh_light_hsl_status hsl_status;

			light_hsl_status(hsl_ctx, &hsl_status);
			bt_mesh_light_hsl_srv_pub(&hsl_ctx->srv, NULL, &hsl_status);
		}
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void light_sat_set(struct bt_mesh_light_sat_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_sat *set,
			  struct bt_mesh_light_sat_status *rsp)
{
	struct bt_mesh_light_hsl_srv *hsl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_hsl_srv, sat);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(hsl_srv, struct light_hsl_ctx, srv);
	struct light_sat_ctx *l_ctx = &hsl_ctx->light_sat_ctx;
	uint32_t step_cnt;

	l_ctx->target = set->lvl;
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_sat_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}
	light_sat_status(l_ctx, rsp);
}

static void light_sat_get(struct bt_mesh_light_sat_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_sat_status *rsp)
{
	struct bt_mesh_light_hsl_srv *hsl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_hsl_srv, sat);
	struct light_hsl_ctx *hsl_ctx =
		CONTAINER_OF(hsl_srv, struct light_hsl_ctx, srv);
	struct light_sat_ctx *l_ctx = &hsl_ctx->light_sat_ctx;

	light_sat_status(l_ctx, rsp);
}

static const struct bt_mesh_light_sat_srv_handlers sat_cb = {
	.set = light_sat_set,
	.get = light_sat_get,
};

static void
start_new_light_xy_trans(uint32_t step_cnt,
			 const struct bt_mesh_model_transition *transition,
			 struct light_xy_ctx *ctx)
{
	ctx->period = (step_cnt ? transition->time / step_cnt : 0);
	k_work_reschedule(&ctx->work, K_MSEC(transition->delay));
}

static void light_xy_status(struct light_xy_ctx *ctx,
			    struct bt_mesh_light_xy_status *rsp)
{
	rsp->current = ctx->current;
	rsp->target = ctx->target;
	rsp->remaining_time = ctx->remaining;
}

static void xyl_get(struct bt_mesh_light_xyl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_xyl_status *status)
{
	struct bt_mesh_lightness_status lightness = { 0 };
	struct bt_mesh_light_xy_status xy = { 0 };

	srv->lightness_srv->handlers->light_get(srv->lightness_srv, ctx,
					       &lightness);
	srv->handlers->xy_get(srv, ctx, &xy);

	status->params.xy = xy.current;
	status->params.lightness = lightness.current;
	status->remaining_time =
		MAX(xy.remaining_time, lightness.remaining_time);
}

static void periodic_light_xy_work(struct k_work *work)
{
	struct light_xy_ctx *ctx =
		CONTAINER_OF(work, struct light_xy_ctx, work);
	struct light_xyl_hsl_ctx *xyl_hsl_ctx =
		CONTAINER_OF(ctx, struct light_xyl_hsl_ctx, xyl_ctx);

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target.x - ctx->current.x) <= PWM_SIZE_STEP) ||
	    (abs(ctx->target.y - ctx->current.y) <= PWM_SIZE_STEP)) {
		ctx->current.x = ctx->target.x;
		ctx->current.y = ctx->target.y;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_light_xyl_status xyl_status;
		struct bt_mesh_light_hsl_status hsl_status;

		xyl_get(&ctx->srv, NULL, &xyl_status);
		bt_mesh_light_xyl_srv_pub(&ctx->srv, NULL, &xyl_status);

		light_hsl_status(&xyl_hsl_ctx->hsl_ctx, &hsl_status);
		bt_mesh_light_hsl_srv_pub(&xyl_hsl_ctx->hsl_ctx.srv, NULL, &hsl_status);

		return;
	} else if ((ctx->target.x > ctx->current.x) &&
		   (ctx->target.y > ctx->current.y)) {
		ctx->current.x += PWM_SIZE_STEP;
		ctx->current.y += PWM_SIZE_STEP;
	} else {
		ctx->current.x -= PWM_SIZE_STEP;
		ctx->current.y -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void light_xy_set(struct bt_mesh_light_xyl_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_light_xy_set *set,
			 struct bt_mesh_light_xy_status *rsp)
{
	struct light_xy_ctx *l_ctx =
		CONTAINER_OF(srv, struct light_xy_ctx, srv);
	struct light_xyl_hsl_ctx *xyl_hsl_ctx =
		CONTAINER_OF(l_ctx, struct light_xyl_hsl_ctx, xyl_ctx);
	struct bt_mesh_light_hsl_status hsl_status;
	uint32_t step_cnt;

	l_ctx->target.x = set->params.x;
	l_ctx->target.y = set->params.y;
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt =
			abs(l_ctx->target.x - l_ctx->current.x) / PWM_SIZE_STEP;
		step_cnt =
			abs(l_ctx->target.y - l_ctx->current.y) / PWM_SIZE_STEP;
		start_new_light_xy_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current.x = l_ctx->target.x;
		l_ctx->current.y = l_ctx->target.y;
	}
	light_xy_status(l_ctx, rsp);

	light_hsl_status(&xyl_hsl_ctx->hsl_ctx, &hsl_status);
	bt_mesh_light_hsl_srv_pub(&xyl_hsl_ctx->hsl_ctx.srv, NULL, &hsl_status);
}
static void light_xy_get(struct bt_mesh_light_xyl_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_light_xy_status *rsp)
{
	struct light_xy_ctx *l_ctx =
		CONTAINER_OF(srv, struct light_xy_ctx, srv);

	light_xy_status(l_ctx, rsp);
}

static const struct bt_mesh_light_xyl_srv_handlers light_xyl_handlers = {
	.xy_set = light_xy_set,
	.xy_get = light_xy_get,
};

static void periodic_light_xyl_hsl_lightness_work(struct k_work *work)
{
	struct lightness_ctx *ctx =
		CONTAINER_OF(work, struct lightness_ctx, work);
	struct light_xyl_hsl_ctx *xyl_hsl_ctx =
		CONTAINER_OF(ctx, struct light_xyl_hsl_ctx, lightness_ctx);
	struct light_hue_ctx *hue_ctx = &xyl_hsl_ctx->hsl_ctx.light_hue_ctx;
	struct light_sat_ctx *sat_ctx = &xyl_hsl_ctx->hsl_ctx.light_sat_ctx;

	ctx->remaining -= ctx->period;

	if ((ctx->remaining <= ctx->period) ||
	    (abs(ctx->target - ctx->current) <= PWM_SIZE_STEP)) {
		ctx->current = ctx->target;
		ctx->remaining = 0;
		/* Publish the new value at the end of the transition */
		struct bt_mesh_lightness_status status;

		lightness_status(ctx, &status);
		bt_mesh_lightness_srv_pub(&xyl_hsl_ctx->lightness_ctx.srv, NULL, &status);

		if (hue_ctx->remaining == 0 && sat_ctx->remaining == 0) {
			struct bt_mesh_light_hsl_status hsl_status;

			light_hsl_status(&xyl_hsl_ctx->hsl_ctx, &hsl_status);
			bt_mesh_light_hsl_srv_pub(&xyl_hsl_ctx->hsl_ctx.srv, NULL, &hsl_status);
		}
		return;
	} else if (ctx->target > ctx->current) {
		ctx->current += PWM_SIZE_STEP;
	} else {
		ctx->current -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&ctx->work, K_MSEC(ctx->period));
}

static void light_xyl_hsl_lightness_set(struct bt_mesh_lightness_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_set *set,
				    struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx = CONTAINER_OF(srv, struct lightness_ctx, srv);
	struct light_xyl_hsl_ctx *xyl_hsl_ctx =
		CONTAINER_OF(l_ctx, struct light_xyl_hsl_ctx, lightness_ctx);
	struct bt_mesh_light_xyl_status xyl_status;
	struct bt_mesh_light_hsl_status hsl_status;
	uint32_t step_cnt;

	l_ctx->target = set->lvl;
	if (set->transition) {
		l_ctx->remaining = set->transition->time;
	} else {
		l_ctx->remaining = 0;
	}

	if (l_ctx->remaining) {
		step_cnt = abs(l_ctx->target - l_ctx->current) / PWM_SIZE_STEP;
		start_new_lightness_trans(step_cnt, set->transition, l_ctx);
	} else {
		l_ctx->current = l_ctx->target;
	}

	lightness_status(l_ctx, rsp);

	xyl_get(&xyl_hsl_ctx->xyl_ctx.srv, NULL, &xyl_status);
	bt_mesh_light_xyl_srv_pub(&xyl_hsl_ctx->xyl_ctx.srv, NULL, &xyl_status);

	light_hsl_status(&xyl_hsl_ctx->hsl_ctx, &hsl_status);
	bt_mesh_light_hsl_srv_pub(&xyl_hsl_ctx->hsl_ctx.srv, NULL, &hsl_status);
}

static void light_xyl_hsl_lightness_get(struct bt_mesh_lightness_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx = CONTAINER_OF(srv, struct lightness_ctx, srv);

	lightness_status(l_ctx, rsp);
}

static const struct bt_mesh_lightness_srv_handlers xyl_hsl_lightness_srv_handlers = {
	.light_set = light_xyl_hsl_lightness_set,
	.light_get = light_xyl_hsl_lightness_get,
};

static struct light_xyl_hsl_ctx xyl_hsl_ctx = {
	.lightness_ctx.srv = BT_MESH_LIGHTNESS_SRV_INIT(&xyl_hsl_lightness_srv_handlers),
	.xyl_ctx = {
		.srv = BT_MESH_LIGHT_XYL_SRV_INIT(&xyl_hsl_ctx.lightness_ctx.srv,
						  &light_xyl_handlers),
		.lightness_ctx = &xyl_hsl_ctx.lightness_ctx,
	},
	.hsl_ctx = {
		.srv = BT_MESH_LIGHT_HSL_SRV_INIT(&xyl_hsl_ctx.lightness_ctx.srv, &hue_cb, &sat_cb),
		.lightness_ctx = &xyl_hsl_ctx.lightness_ctx,
	}
};

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
	BT_MESH_ELEM(10,
		     BT_MESH_MODEL_LIST(
				 BT_MESH_MODEL_CFG_SRV,
				 BT_MESH_MODEL_CFG_CLI(&cfg_cli),
				 BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
				 BT_MESH_MODEL_HEALTH_CLI(&health_cli),
			     BT_MESH_MODEL_DTT_SRV(&dtt_srv),
			     BT_MESH_MODEL_SCHEDULER_SRV(&scheduler_srv)),
		     vnd_models),
	BT_MESH_ELEM(20,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_BATTERY_SRV(&bat_ctx.srv),
			     BT_MESH_MODEL_LOC_SRV(&loc_ctx.srv),
			     BT_MESH_MODEL_BATTERY_SRV(&bat_ctx.srv),
			     BT_MESH_MODEL_PROP_SRV_ADMIN(&prop_ctx.admin_srv),
			     BT_MESH_MODEL_PROP_SRV_CLIENT(&prop_ctx.cli_srv),
			     BT_MESH_MODEL_PROP_SRV_USER(&prop_ctx.usr_srv),
			     BT_MESH_MODEL_PROP_SRV_MFR(&prop_ctx.mfr_srv),
			     BT_MESH_MODEL_SENSOR_SRV(&sensor_srv),
			     BT_MESH_MODEL_TIME_SRV(&time_srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(30,
		     BT_MESH_MODEL_LIST(
				 BT_MESH_MODEL_PLVL_SRV(&lvl_ctx.srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(40,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHTNESS_SRV(&lightness_ctx.srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(41,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_CTRL_SRV(&light_ctrl_srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(50,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHTNESS_SRV(&xyl_hsl_ctx.lightness_ctx.srv),
			     BT_MESH_MODEL_LIGHT_XYL_SRV(&xyl_hsl_ctx.xyl_ctx.srv),
			     BT_MESH_MODEL_LIGHT_HSL_SRV(&xyl_hsl_ctx.hsl_ctx.srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(51,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_HUE_SRV(&xyl_hsl_ctx.hsl_ctx.srv.hue)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(52,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_SAT_SRV(&xyl_hsl_ctx.hsl_ctx.srv.sat)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(60,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_CTL_SRV(&light_ctl_ctx.srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(61,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_TEMP_SRV(&light_ctl_ctx.srv.temp_srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(70,
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
			     BT_MESH_MODEL_SCENE_CLI(&scene_cli),
			     BT_MESH_MODEL_LIGHT_XYL_CLI(&xyl_cli),
			     BT_MESH_MODEL_LIGHT_HSL_CLI(&light_hsl_cli),
			     BT_MESH_MODEL_SCHEDULER_CLI(&scheduler_cli)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&lvl_ctx.work, periodic_led_work);
	k_work_init_delayable(&lightness_ctx.work, periodic_led_lightness_work);
	k_work_init_delayable(&xyl_hsl_ctx.xyl_ctx.work, periodic_light_xy_work);
	k_work_init_delayable(&xyl_hsl_ctx.lightness_ctx.work,
			      periodic_light_xyl_hsl_lightness_work);
	k_work_init_delayable(&xyl_hsl_ctx.hsl_ctx.light_hue_ctx.work,
			      periodic_light_hue_work);
	k_work_init_delayable(&xyl_hsl_ctx.hsl_ctx.light_sat_ctx.work,
			      periodic_light_sat_work);
	k_work_init_delayable(&light_ctl_ctx.ctl_lightness_ctx.work,
			      periodic_light_ctl_lightness_work);
	k_work_init_delayable(&light_ctl_ctx.light_temp_ctx.work,
			      periodic_light_temp_work);
	return &comp;
}
