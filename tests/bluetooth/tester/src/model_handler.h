/* model_handler.h - Bluetooth Mesh Models handler header */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MODEL_HANDLER_H__
#define MODEL_HANDLER_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUR_FAULTS_MAX 4
#define HEALTH_TEST_ID 0x00

const struct bt_mesh_comp *model_handler_init(void);

extern struct bt_mesh_onoff_cli onoff_cli;
extern struct bt_mesh_lvl_cli lvl_cli;
extern struct bt_mesh_dtt_cli dtt_cli;
extern struct bt_mesh_ponoff_cli ponoff_cli;
extern struct bt_mesh_plvl_cli plvl_cli;
extern struct bt_mesh_battery_cli battery_cli;
extern struct bt_mesh_loc_cli loc_cli;
extern struct bt_mesh_prop_cli prop_cli;
extern struct bt_mesh_sensor_cli sensor_cli;
extern struct bt_mesh_time_cli time_cli;
extern struct bt_mesh_lightness_cli lightness_cli;
extern struct bt_mesh_light_ctrl_cli light_ctrl_cli;
extern struct bt_mesh_light_ctl_cli light_ctl_cli;
extern struct bt_mesh_scene_cli scene_cli;
extern struct bt_mesh_light_xyl_cli xyl_cli;
extern struct bt_mesh_light_hsl_cli light_hsl_cli;
extern struct bt_mesh_scheduler_cli scheduler_cli;

struct model_data {
	const struct bt_mesh_model *model;
	uint16_t addr;
	uint16_t appkey_idx;
};

/* Model send data */
#define MODEL_BOUNDS_MAX 100

extern struct model_data model_bound[MODEL_BOUNDS_MAX];

struct net_ctx {
	uint16_t local;
	uint16_t dst;
	uint16_t net_idx;
};

extern struct net_ctx net;
void sensor_data_set(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_HANDLER_H__ */
