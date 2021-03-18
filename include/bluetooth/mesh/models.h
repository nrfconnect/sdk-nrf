/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_MODELS_H__
#define BT_MESH_MODELS_H__

#include <bluetooth/mesh.h>

#include <bluetooth/mesh/model_types.h>

/* Foundation models */
#include <bluetooth/mesh/cfg_cli.h>
#include <bluetooth/mesh/cfg_srv.h>
#include <bluetooth/mesh/health_cli.h>
#include <bluetooth/mesh/health_srv.h>

/* Generic models */
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/gen_onoff_cli.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#include <bluetooth/mesh/gen_lvl_cli.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include <bluetooth/mesh/gen_dtt_cli.h>
#include <bluetooth/mesh/gen_ponoff_srv.h>
#include <bluetooth/mesh/gen_ponoff_cli.h>
#include <bluetooth/mesh/gen_plvl_srv.h>
#include <bluetooth/mesh/gen_plvl_cli.h>
#include <bluetooth/mesh/gen_battery_srv.h>
#include <bluetooth/mesh/gen_battery_cli.h>
#include <bluetooth/mesh/gen_loc_srv.h>
#include <bluetooth/mesh/gen_loc_cli.h>
#include <bluetooth/mesh/gen_prop_srv.h>
#include <bluetooth/mesh/gen_prop_cli.h>

/* Sensor models */
#include <bluetooth/mesh/sensor_types.h>
#include <bluetooth/mesh/sensor_srv.h>
#include <bluetooth/mesh/sensor_cli.h>

/* Lighting models */
#include <bluetooth/mesh/lightness_srv.h>
#include <bluetooth/mesh/lightness_cli.h>
#include <bluetooth/mesh/light_ctrl_srv.h>
#include <bluetooth/mesh/light_ctrl_cli.h>
#include <bluetooth/mesh/light_ctl_cli.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/light_ctl_srv.h>
#include <bluetooth/mesh/light_xyl_cli.h>
#include <bluetooth/mesh/light_xyl_srv.h>
#include <bluetooth/mesh/light_hsl_cli.h>
#include <bluetooth/mesh/light_hsl_srv.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hue_srv.h>

/* Time and Scene models */
#include <bluetooth/mesh/time_cli.h>
#include <bluetooth/mesh/time_srv.h>
#include <bluetooth/mesh/scene_cli.h>
#include <bluetooth/mesh/scene_srv.h>
#include <bluetooth/mesh/scheduler_cli.h>
#include <bluetooth/mesh/scheduler_srv.h>

/** @brief Check whether the model publishes to a unicast address.
 *
 * @param[in] model Model to check
 *
 * @return true if the model publishes to a unicast address, false otherwise.
 */
bool bt_mesh_model_pub_is_unicast(const struct bt_mesh_model *model);

/** Shorthand macro for defining a model list directly in the element. */
#define BT_MESH_MODEL_LIST(...) ((struct bt_mesh_model[]){ __VA_ARGS__ })

#endif /* BT_MESH_MODELS_H__ */
