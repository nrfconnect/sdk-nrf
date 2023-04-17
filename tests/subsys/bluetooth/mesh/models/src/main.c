/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>

/* Instantiate all models, using their INIT macros if available. */
#if !defined CONFIG_EMDS
static struct bt_mesh_lvl_srv lvl_srv = BT_MESH_LVL_SRV_INIT(NULL);
static struct bt_mesh_ponoff_srv ponoff_srv =
	BT_MESH_PONOFF_SRV_INIT(NULL, NULL, NULL);
static struct bt_mesh_light_ctrl_srv light_ctrl_srv =
	BT_MESH_LIGHT_CTRL_SRV_INIT(NULL);
static struct bt_mesh_light_ctl_srv light_ctl_srv =
	BT_MESH_LIGHT_CTL_SRV_INIT(NULL, NULL);
static struct bt_mesh_onoff_srv onoff_srv = BT_MESH_ONOFF_SRV_INIT(NULL);
static struct bt_mesh_onoff_cli onoff_cli = BT_MESH_ONOFF_CLI_INIT(NULL);
static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);
static struct bt_mesh_time_cli time_cli = BT_MESH_TIME_CLI_INIT(NULL);
static struct bt_mesh_lvl_cli lvl_cli = BT_MESH_LVL_CLI_INIT(NULL);
static struct bt_mesh_dtt_srv dtt_srv = BT_MESH_DTT_SRV_INIT(NULL);
static struct bt_mesh_dtt_cli dtt_cli = BT_MESH_DTT_CLI_INIT(NULL);
static struct bt_mesh_ponoff_cli ponoff_cli = BT_MESH_PONOFF_CLI_INIT(NULL);
static struct bt_mesh_plvl_cli plvl_cli = BT_MESH_PLVL_CLI_INIT(NULL);
static struct bt_mesh_battery_srv battery_srv = BT_MESH_BATTERY_SRV_INIT(NULL);
static struct bt_mesh_battery_cli battery_cli = BT_MESH_BATTERY_CLI_INIT(NULL);
static struct bt_mesh_loc_srv loc_srv = BT_MESH_LOC_SRV_INIT(NULL);
static struct bt_mesh_loc_cli loc_cli = BT_MESH_LOC_CLI_INIT(NULL);
static struct bt_mesh_prop_srv prop_srv = BT_MESH_PROP_SRV_USER_INIT();
static struct bt_mesh_prop_cli prop_cli = BT_MESH_PROP_CLI_INIT(NULL, NULL);
static struct bt_mesh_lightness_cli lightness_cli =
	BT_MESH_LIGHTNESS_CLI_INIT(NULL);
static struct bt_mesh_light_ctrl_cli light_ctrl_cli =
	BT_MESH_LIGHT_CTRL_CLI_INIT(NULL);
static struct bt_mesh_light_ctl_cli light_ctl_cli =
	BT_MESH_LIGHT_CTL_CLI_INIT(NULL);
static struct bt_mesh_sensor_cli sensor_cli = BT_MESH_SENSOR_CLI_INIT(NULL);
static struct bt_mesh_scene_cli scene_cli;
static struct bt_mesh_light_xyl_cli xyl_cli;
static struct bt_mesh_light_hsl_cli light_hsl_cli;
static struct bt_mesh_light_hsl_srv light_hsl_srv =
	BT_MESH_LIGHT_HSL_SRV_INIT(NULL, NULL, NULL);
static struct bt_mesh_scheduler_cli scheduler_cli;
/* Settings dependent models: */
#ifdef CONFIG_BT_SETTINGS
static struct bt_mesh_scene_srv scene_srv;
static struct bt_mesh_scheduler_srv scheduler_srv =
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv);
#endif
#endif /* !defined CONFIG_EMDS */
static struct bt_mesh_plvl_srv plvl_srv = BT_MESH_PLVL_SRV_INIT(NULL);
static struct bt_mesh_light_hue_srv light_hue_srv =
	BT_MESH_LIGHT_HUE_SRV_INIT(NULL);
static struct bt_mesh_light_sat_srv light_sat_srv =
	BT_MESH_LIGHT_SAT_SRV_INIT(NULL);
static struct bt_mesh_light_xyl_srv xyl_srv =
	BT_MESH_LIGHT_XYL_SRV_INIT(NULL, NULL);
static struct bt_mesh_light_temp_srv light_temp_srv =
	BT_MESH_LIGHT_TEMP_SRV_INIT(NULL);
static struct bt_mesh_lightness_srv lightness_srv =
	BT_MESH_LIGHTNESS_SRV_INIT(NULL);

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, BT_MESH_MODEL_LIST(
#if !defined CONFIG_EMDS
#ifdef CONFIG_BT_SETTINGS
		BT_MESH_MODEL_SCENE_SRV(&scene_srv),
		BT_MESH_MODEL_SCHEDULER_SRV(&scheduler_srv),
#endif
		BT_MESH_MODEL_LVL_SRV(&lvl_srv),
		BT_MESH_MODEL_PONOFF_SRV(&ponoff_srv),
		BT_MESH_MODEL_LIGHT_CTRL_SRV(&light_ctrl_srv),
		BT_MESH_MODEL_LIGHT_CTL_SRV(&light_ctl_srv),
		BT_MESH_MODEL_ONOFF_SRV(&onoff_srv),
		BT_MESH_MODEL_ONOFF_CLI(&onoff_cli),
		BT_MESH_MODEL_TIME_SRV(&time_srv),
		BT_MESH_MODEL_TIME_CLI(&time_cli),
		BT_MESH_MODEL_LVL_CLI(&lvl_cli),
		BT_MESH_MODEL_DTT_SRV(&dtt_srv),
		BT_MESH_MODEL_DTT_CLI(&dtt_cli),
		BT_MESH_MODEL_PONOFF_CLI(&ponoff_cli),
		BT_MESH_MODEL_PLVL_CLI(&plvl_cli),
		BT_MESH_MODEL_BATTERY_SRV(&battery_srv),
		BT_MESH_MODEL_BATTERY_CLI(&battery_cli),
		BT_MESH_MODEL_LOC_SRV(&loc_srv),
		BT_MESH_MODEL_LOC_CLI(&loc_cli),
		BT_MESH_MODEL_PROP_SRV_USER(&prop_srv),
		BT_MESH_MODEL_PROP_CLI(&prop_cli),
		BT_MESH_MODEL_LIGHTNESS_CLI(&lightness_cli),
		BT_MESH_MODEL_LIGHT_CTRL_CLI(&light_ctrl_cli),
		BT_MESH_MODEL_LIGHT_CTL_CLI(&light_ctl_cli),
		BT_MESH_MODEL_SENSOR_CLI(&sensor_cli),
		BT_MESH_MODEL_SCENE_CLI(&scene_cli),
		BT_MESH_MODEL_LIGHT_XYL_CLI(&xyl_cli),
		BT_MESH_MODEL_LIGHT_HSL_CLI(&light_hsl_cli),
		BT_MESH_MODEL_LIGHT_HSL_SRV(&light_hsl_srv),
		BT_MESH_MODEL_SCHEDULER_CLI(&scheduler_cli),
#endif /* !defined CONFIG_EMDS */
		BT_MESH_MODEL_PLVL_SRV(&plvl_srv),
		BT_MESH_MODEL_LIGHT_HUE_SRV(&light_hue_srv),
		BT_MESH_MODEL_LIGHT_SAT_SRV(&light_sat_srv),
		BT_MESH_MODEL_LIGHT_TEMP_SRV(&light_temp_srv),
		BT_MESH_MODEL_LIGHT_XYL_SRV(&xyl_srv),
		BT_MESH_MODEL_LIGHTNESS_SRV(&lightness_srv)
	), BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

int main(void)
{
	bt_mesh_init(NULL, &comp);

	return 0;
}
