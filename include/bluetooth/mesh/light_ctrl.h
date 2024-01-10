/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_light_ctrl Light Lightness Control common API
 *  @{
 *  @brief Common types for the Light Lightness Control models.
 */

#ifndef BT_MESH_LIGHT_CTRL_H__
#define BT_MESH_LIGHT_CTRL_H__

#include <bluetooth/mesh/properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Light Lightness Control properties */
enum bt_mesh_light_ctrl_prop {
	/** Target ambient illuminance in the On state.
	 *
	 *  - Unit: Lux
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
	 *  - Range: 0 to 167772.13
	 */
	BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON =
		BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_ON,

	/** Target ambient illuminance in the Prolong state.
	 *
	 *  - Unit: Lux
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
	 *  - Range: 0 to 167772.13
	 */
	BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG =
		BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_PROLONG,

	/** Target ambient illuminance in the Standby state.
	 *
	 *  - Unit: Lux
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
	 *  - Range: 0 to 167772.13
	 */
	BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY =
		BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_STANDBY,

	/** Lightness in the On state.
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 16 bit unsigned scalar (Resolution: 1)
	 *  - Range: 0 to 65535
	 */
	BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON =
		BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_ON,

	/** Lightness in the Prolong state.
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 16 bit unsigned scalar (Resolution: 1)
	 *  - Range: 0 to 65535
	 */
	BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG =
		BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_PROLONG,

	/** Lightness in the Standby state.
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 16 bit unsigned scalar (Resolution: 1)
	 *  - Range: 0 to 65535
	 */
	BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY =
		BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_STANDBY,

	/** Regulator Accuracy.
	 *
	 *  - Unit: Percent
	 *  - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
	 *  - Range: 0 to 100.0
	 */
	BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY =
		BT_MESH_PROP_ID_LIGHT_CTRL_REG_ACCURACY,

	/** Fade time for fading to the Prolong state.
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_PROLONG =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE,

	/** Fade time for fading to the On state.
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_ON =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_ON,

	/** Fade time for fading to the Standby state (auto mode).
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_AUTO =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_STANDBY_AUTO,

	/** Fade time for fading to the Standby state (manual mode).
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_MANUAL =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_STANDBY_MANUAL,

	/** Delay from Occupancy sensor reports until the light turns on.
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_OCCUPANCY_DELAY =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_OCCUPANCY_DELAY,

	/** Time to stay in the Prolong state.
	 *
	 *  *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_PROLONG =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_PROLONG,

	/** Time to stay in the On state.
	 *
	 *  - Unit: Seconds
	 *  - Encoding: 24 bit unsigned scalar (Resolution: 1 ms)
	 *  - Range: 0 to 16777.214
	 */
	BT_MESH_LIGHT_CTRL_PROP_TIME_ON =
		BT_MESH_PROP_ID_LIGHT_CTRL_TIME_RUN_ON,
};

/** Light Control Regulator Coefficients */
enum bt_mesh_light_ctrl_coeff {
	/** Regulator downwards integral coefficient (Kid).
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 32 bit floating point.
	 *  - Range: 0 to 1000.0
	 */
	BT_MESH_LIGHT_CTRL_COEFF_KID = BT_MESH_PROP_ID_LIGHT_CTRL_REG_KID,

	/** Regulator upwards integral coefficient (Kiu).
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 32 bit floating point.
	 *  - Range: 0 to 1000.0
	 */
	BT_MESH_LIGHT_CTRL_COEFF_KIU = BT_MESH_PROP_ID_LIGHT_CTRL_REG_KIU,

	/** Regulator downwards proportional coefficient (Kpd).
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 32 bit floating point.
	 *  - Range: 0 to 1000.0
	 */
	BT_MESH_LIGHT_CTRL_COEFF_KPD = BT_MESH_PROP_ID_LIGHT_CTRL_REG_KPD,

	/** Regulator upwards proportional coeffient (Kpu).
	 *
	 *  - Unit: Unitless
	 *  - Encoding: 32 bit floating point.
	 *  - Range: 0 to 1000.0
	 */
	BT_MESH_LIGHT_CTRL_COEFF_KPU = BT_MESH_PROP_ID_LIGHT_CTRL_REG_KPU,
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LIGHT_CTRL_OP_MODE_GET BT_MESH_MODEL_OP_2(0x82, 0x91)
#define BT_MESH_LIGHT_CTRL_OP_MODE_SET BT_MESH_MODEL_OP_2(0x82, 0x92)
#define BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x93)
#define BT_MESH_LIGHT_CTRL_OP_MODE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x94)
#define BT_MESH_LIGHT_CTRL_OP_OM_GET BT_MESH_MODEL_OP_2(0x82, 0x95)
#define BT_MESH_LIGHT_CTRL_OP_OM_SET BT_MESH_MODEL_OP_2(0x82, 0x96)
#define BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x97)
#define BT_MESH_LIGHT_CTRL_OP_OM_STATUS BT_MESH_MODEL_OP_2(0x82, 0x98)
#define BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET BT_MESH_MODEL_OP_2(0x82, 0x99)
#define BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET BT_MESH_MODEL_OP_2(0x82, 0x9A)
#define BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET_UNACK                            \
	BT_MESH_MODEL_OP_2(0x82, 0x9B)
#define BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x9C)
#define BT_MESH_LIGHT_CTRL_OP_PROP_GET BT_MESH_MODEL_OP_2(0x82, 0x9D)
#define BT_MESH_LIGHT_CTRL_OP_PROP_SET BT_MESH_MODEL_OP_1(0x62)
#define BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK BT_MESH_MODEL_OP_1(0x63)
#define BT_MESH_LIGHT_CTRL_OP_PROP_STATUS BT_MESH_MODEL_OP_1(0x64)

#define BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_GET 0
#define BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_SET 1
#define BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_STATUS 1
#define BT_MESH_LIGHT_CTRL_MSG_LEN_OM_GET 0
#define BT_MESH_LIGHT_CTRL_MSG_LEN_OM_SET 1
#define BT_MESH_LIGHT_CTRL_MSG_LEN_OM_STATUS 1
#define BT_MESH_LIGHT_CTRL_MSG_LEN_LIGHT_ONOFF_GET 0
#define BT_MESH_LIGHT_CTRL_MSG_MINLEN_LIGHT_ONOFF_SET 2
#define BT_MESH_LIGHT_CTRL_MSG_MINLEN_LIGHT_ONOFF_STATUS 1
#define BT_MESH_LIGHT_CTRL_MSG_LEN_PROP_GET 2
#define BT_MESH_LIGHT_CTRL_MSG_MINLEN_PROP_SET 2
#define BT_MESH_LIGHT_CTRL_MSG_MINLEN_PROP_STATUS 2

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
#define BT_MESH_LIGHT_CTRL_SRV_REG_CFG_INIT                                    \
	{                                                                      \
		.ki.up = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KIU,                  \
		.ki.down = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KID,                  \
		.kp.up = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KPU,                  \
		.kp.down = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KPD,                  \
		.accuracy = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_ACCURACY,        \
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
#define BT_MESH_LIGHT_CTRL_SRV_LUX_INIT                                        \
	.lux = {                                                               \
		{ CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_STANDBY },             \
		{ CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON },                  \
		{ CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_PROLONG }              \
	}
#else
#define BT_MESH_LIGHT_CTRL_SRV_LUX_INIT                                        \
	.centilux = {                                                          \
		CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_STANDBY,                 \
		CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON,                      \
		CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_PROLONG                  \
	}
#endif
#else
#define BT_MESH_LIGHT_CTRL_SRV_LUX_INIT
#endif

#define BT_MESH_LIGHT_CTRL_SRV_CFG_INIT                                        \
	{                                                                      \
		.occupancy_delay =                                             \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_OCCUPANCY_DELAY,         \
		.fade_on =                                                     \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_ON,            \
		.on = (MSEC_PER_SEC * CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_ON),  \
		.fade_prolong =                                                \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_PROLONG,       \
		.prolong = (MSEC_PER_SEC *                                     \
			    CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_PROLONG),       \
		.fade_standby_auto =                                           \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_STANDBY_AUTO,  \
		.fade_standby_manual =                                         \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_STANDBY_MANUAL,\
		.light = {                                                     \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_STANDBY,             \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_ON,                  \
			CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_PROLONG,             \
		},                                                             \
		BT_MESH_LIGHT_CTRL_SRV_LUX_INIT                                \
	}
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTRL_H__ */

/** @} */
