/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @defgroup bt_mesh_properties Bluetooth Mesh Properties
 * @{
 * @brief Device properties definitions.
 */
#ifndef BT_MESH_PROPERTIES_H__
#define BT_MESH_PROPERTIES_H__

#include <bluetooth/mesh.h>

/**
 * @defgroup bt_mesh_property_ids Property IDs
 * All available Mesh Property IDs. See the Bluetooth Mesh Device Properties
 * Specification for details.
 * @{
 */
/** Prohibited. */
#define BT_MESH_PROP_ID_PROHIBITED 0x0000
/** Average ambient temperature in a period of day. */
#define BT_MESH_PROP_ID_AVG_AMB_TEMP_IN_A_PERIOD_OF_DAY 0x0001
/** Average input current. */
#define BT_MESH_PROP_ID_AVG_INPUT_CURRENT 0x0002
/** Average input voltage. */
#define BT_MESH_PROP_ID_AVG_INPUT_VOLTAGE 0x0003
/** Average output current. */
#define BT_MESH_PROP_ID_AVG_OUTPUT_CURRENT 0x0004
/** Average output voltage. */
#define BT_MESH_PROP_ID_AVG_OUTPUT_VOLTAGE 0x0005
/** Center beam intensity at full power. */
#define BT_MESH_PROP_ID_CENTER_BEAM_INTENSITY_AT_FULL_POWER 0x0006
/** Chromaticity tolerance. */
#define BT_MESH_PROP_ID_CHROMATICITY_TOLERANCE 0x0007
/** Color rendering index R9. */
#define BT_MESH_PROP_ID_COL_RENDERING_INDEX_R9 0x0008
/** Color rendering index RA. */
#define BT_MESH_PROP_ID_COL_RENDERING_INDEX_RA 0x0009
/** Device appearance. */
#define BT_MESH_PROP_ID_DEV_APPEARANCE 0x000A
/** Device country of origin. */
#define BT_MESH_PROP_ID_DEV_COUNTRY_OF_ORIGIN 0x000B
/** Device date of manufacture. */
#define BT_MESH_PROP_ID_DEV_DATE_OF_MANUFACTURE 0x000C
/** Device energy use since turn on. */
#define BT_MESH_PROP_ID_DEV_ENERGY_USE_SINCE_TURN_ON 0x000D
/** Device firmware revision. */
#define BT_MESH_PROP_ID_DEV_FW_REVISION 0x000E
/** Device global trade item number. */
#define BT_MESH_PROP_ID_DEV_GLOBAL_TRADE_ITEM_NUM 0x000F
/** Device hardware revision. */
#define BT_MESH_PROP_ID_DEV_HW_REVISION 0x0010
/** Device manufacturer name. */
#define BT_MESH_PROP_ID_DEV_MFR_NAME 0x0011
/** Device model number. */
#define BT_MESH_PROP_ID_DEV_MODEL_NUM 0x0012
/** Device operating temperature range specification. */
#define BT_MESH_PROP_ID_DEV_OP_TEMP_RANGE_SPEC 0x0013
/** Device operating temperature statistical values. */
#define BT_MESH_PROP_ID_DEV_OP_TEMP_STAT_VALUES 0x0014
/** Device over temperature event statistics. */
#define BT_MESH_PROP_ID_DEV_OVER_TEMP_EVT_STAT 0x0015
/** Device power range specification. */
#define BT_MESH_PROP_ID_DEV_POWER_RANGE_SPEC 0x0016
/** Device runtime since turn on. */
#define BT_MESH_PROP_ID_DEV_RUNTIME_SINCE_TURN_ON 0x0017
/** Device runtime warranty. */
#define BT_MESH_PROP_ID_DEV_RUNTIME_WARRANTY 0x0018
/** Device serial number. */
#define BT_MESH_PROP_ID_DEV_SERIAL_NUM 0x0019
/** Device software revision. */
#define BT_MESH_PROP_ID_DEV_SW_REVISION 0x001A
/** Device under temperature event statistics. */
#define BT_MESH_PROP_ID_DEV_UNDER_TEMP_EVT_STAT 0x001B
/** Indoor ambient temperature statistical values. */
#define BT_MESH_PROP_ID_INDOOR_AMB_TEMP_STAT_VALUES 0x001C
/** Initial CIE-1931 chromaticity coordinates. */
#define BT_MESH_PROP_ID_INITIAL_CIE_1931_CHROMATICITY_COORDS 0x001D
/** Initial correlated color temperature. */
#define BT_MESH_PROP_ID_INITIAL_CORRELATED_COL_TEMP 0x001E
/** Initial luminous flux. */
#define BT_MESH_PROP_ID_INITIAL_LUMINOUS_FLUX 0x001F
/** Initial planckian distance. */
#define BT_MESH_PROP_ID_INITIAL_PLANCKIAN_DISTANCE 0x0020
/** Input current range specification. */
#define BT_MESH_PROP_ID_INPUT_CURRENT_RANGE_SPEC 0x0021
/** Input current statistics. */
#define BT_MESH_PROP_ID_INPUT_CURRENT_STAT 0x0022
/** Input over current event statistics. */
#define BT_MESH_PROP_ID_INPUT_OVER_CURRENT_EVT_STAT 0x0023
/** Input over ripple voltage event statistics. */
#define BT_MESH_PROP_ID_INPUT_OVER_RIPPLE_VOLTAGE_EVT_STAT 0x0024
/** Input over voltage event statistics. */
#define BT_MESH_PROP_ID_INPUT_OVER_VOLTAGE_EVT_STAT 0x0025
/** Input under current event statistics. */
#define BT_MESH_PROP_ID_INPUT_UNDER_CURRENT_EVT_STAT 0x0026
/** Input under voltage event statistics. */
#define BT_MESH_PROP_ID_INPUT_UNDER_VOLTAGE_EVT_STAT 0x0027
/** Input voltage range specification. */
#define BT_MESH_PROP_ID_INPUT_VOLTAGE_RANGE_SPEC 0x0028
/** Input voltage ripple specification. */
#define BT_MESH_PROP_ID_INPUT_VOLTAGE_RIPPLE_SPEC 0x0029
/** Input voltage statistics. */
#define BT_MESH_PROP_ID_INPUT_VOLTAGE_STAT 0x002A
/** Light control ambient luxlevel on. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_ON 0x002B
/** Light control ambient luxlevel prolong. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_PROLONG 0x002C
/** Light control ambient luxlevel standby. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_AMB_LUXLEVEL_STANDBY 0x002D
/** Light control lightness on. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_ON 0x002E
/** Light control lightness prolong. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_PROLONG 0x002F
/** Light control lightness standby. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_LIGHTNESS_STANDBY 0x0030
/** Light control regulator accuracy. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_REG_ACCURACY 0x0031
/** Light control regulator kid. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_REG_KID 0x0032
/** Light control regulator kiu. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_REG_KIU 0x0033
/** Light control regulator kpd. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_REG_KPD 0x0034
/** Light control regulator kpu. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_REG_KPU 0x0035
/** Light control time fade. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE 0x0036
/** Light control time fade on. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_ON 0x0037
/** Light control time fade standby auto. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_STANDBY_AUTO 0x0038
/** Light control time fade standby manual. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_FADE_STANDBY_MANUAL 0x0039
/** Light control time occupancy delay. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_OCCUPANCY_DELAY 0x003A
/** Light control time prolong. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_PROLONG 0x003B
/** Light control time run on. */
#define BT_MESH_PROP_ID_LIGHT_CTRL_TIME_RUN_ON 0x003C
/** Lumen maintenance factor. */
#define BT_MESH_PROP_ID_LUMEN_MAINTENANCE_FACTOR 0x003D
/** Luminous efficacy. */
#define BT_MESH_PROP_ID_LUMINOUS_EFFICACY 0x003E
/** Luminous energy since turn on. */
#define BT_MESH_PROP_ID_LUMINOUS_ENERGY_SINCE_TURN_ON 0x003F
/** Luminous exposure. */
#define BT_MESH_PROP_ID_LUMINOUS_EXPOSURE 0x0040
/** Luminous flux range. */
#define BT_MESH_PROP_ID_LUMINOUS_FLUX_RANGE 0x0041
/** Motion sensed. */
#define BT_MESH_PROP_ID_MOTION_SENSED 0x0042
/** Motion threshold. */
#define BT_MESH_PROP_ID_MOTION_THRESHOLD 0x0043
/** Open circuit event statistics. */
#define BT_MESH_PROP_ID_OPEN_CIRCUIT_EVT_STAT 0x0044
/** Outdoor statistical values. */
#define BT_MESH_PROP_ID_OUTDOOR_STAT_VALUES 0x0045
/** Output current range. */
#define BT_MESH_PROP_ID_OUTPUT_CURRENT_RANGE 0x0046
/** Output current statistics. */
#define BT_MESH_PROP_ID_OUTPUT_CURRENT_STAT 0x0047
/** Output ripple voltage specification. */
#define BT_MESH_PROP_ID_OUTPUT_RIPPLE_VOLTAGE_SPEC 0x0048
/** Output voltage range. */
#define BT_MESH_PROP_ID_OUTPUT_VOLTAGE_RANGE 0x0049
/** Output voltage statistics. */
#define BT_MESH_PROP_ID_OUTPUT_VOLTAGE_STAT 0x004A
/** Over output ripple voltage event statistics. */
#define BT_MESH_PROP_ID_OVER_OUTPUT_RIPPLE_VOLTAGE_EVT_STAT 0x004B
/** People count. */
#define BT_MESH_PROP_ID_PEOPLE_COUNT 0x004C
/** Presence detected. */
#define BT_MESH_PROP_ID_PRESENCE_DETECTED 0x004D
/** Present ambient light level. */
#define BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL 0x004E
/** Present ambient temperature. */
#define BT_MESH_PROP_ID_PRESENT_AMB_TEMP 0x004F
/** Present CIE-1931 chromaticity coordinates. */
#define BT_MESH_PROP_ID_PRESENT_CIE_1931_CHROMATICITY_COORDS 0x0050
/** Present correlated color temperature. */
#define BT_MESH_PROP_ID_PRESENT_CORRELATED_COL_TEMP 0x0051
/** Present device input power. */
#define BT_MESH_PROP_ID_PRESENT_DEV_INPUT_POWER 0x0052
/** Present device operating efficiency. */
#define BT_MESH_PROP_ID_PRESENT_DEV_OP_EFFICIENCY 0x0053
/** Present device operating temperature. */
#define BT_MESH_PROP_ID_PRESENT_DEV_OP_TEMP 0x0054
/** Present illuminance. */
#define BT_MESH_PROP_ID_PRESENT_ILLUMINANCE 0x0055
/** Present indoor ambient temperature. */
#define BT_MESH_PROP_ID_PRESENT_INDOOR_AMB_TEMP 0x0056
/** Present input current. */
#define BT_MESH_PROP_ID_PRESENT_INPUT_CURRENT 0x0057
/** Present input ripple voltage. */
#define BT_MESH_PROP_ID_PRESENT_INPUT_RIPPLE_VOLTAGE 0x0058
/** Present input voltage. */
#define BT_MESH_PROP_ID_PRESENT_INPUT_VOLTAGE 0x0059
/** Present luminous flux. */
#define BT_MESH_PROP_ID_PRESENT_LUMINOUS_FLUX 0x005A
/** Present outdoor ambient temperature. */
#define BT_MESH_PROP_ID_PRESENT_OUTDOOR_AMB_TEMP 0x005B
/** Present output current. */
#define BT_MESH_PROP_ID_PRESENT_OUTPUT_CURRENT 0x005C
/** Present output voltage. */
#define BT_MESH_PROP_ID_PRESENT_OUTPUT_VOLTAGE 0x005D
/** Present planckian distance. */
#define BT_MESH_PROP_ID_PRESENT_PLANCKIAN_DISTANCE 0x005E
/** Present relative output ripple voltage. */
#define BT_MESH_PROP_ID_PRESENT_REL_OUTPUT_RIPPLE_VOLTAGE 0x005F
/** Relative device energy use in a period of day. */
#define BT_MESH_PROP_ID_REL_DEV_ENERGY_USE_IN_A_PERIOD_OF_DAY 0x0060
/** Relative device runtime in a generic level range. */
#define BT_MESH_PROP_ID_REL_DEV_RUNTIME_IN_A_GENERIC_LEVEL_RANGE 0x0061
/** Relative exposure time in an illuminance range. */
#define BT_MESH_PROP_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE 0x0062
/** Relative runtime in a correlated color temperature range. */
#define BT_MESH_PROP_ID_REL_RUNTIME_IN_A_CORRELATED_COL_TEMP_RANGE 0x0063
/** Relative runtime in a device operating temperature range. */
#define BT_MESH_PROP_ID_REL_RUNTIME_IN_A_DEV_OP_TEMP_RANGE 0x0064
/** Relative runtime in an input current range. */
#define BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_CURRENT_RANGE 0x0065
/** Relative runtime in an input voltage range. */
#define BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_VOLTAGE_RANGE 0x0066
/** Short circuit event statistics. */
#define BT_MESH_PROP_ID_SHORT_CIRCUIT_EVT_STAT 0x0067
/** Time since motion sensed. */
#define BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED 0x0068
/** Time since presence detected. */
#define BT_MESH_PROP_ID_TIME_SINCE_PRESENCE_DETECTED 0x0069
/** Total device energy use. */
#define BT_MESH_PROP_ID_TOT_DEV_ENERGY_USE 0x006A
/** Total device off on cycles. */
#define BT_MESH_PROP_ID_TOT_DEV_OFF_ON_CYCLES 0x006B
/** Total device power on cycles. */
#define BT_MESH_PROP_ID_TOT_DEV_POWER_ON_CYCLES 0x006C
/** Total device power on time. */
#define BT_MESH_PROP_ID_TOT_DEV_POWER_ON_TIME 0x006D
/** Total device runtime. */
#define BT_MESH_PROP_ID_TOT_DEV_RUNTIME 0x006E
/** Total light exposure time. */
#define BT_MESH_PROP_ID_TOT_LIGHT_EXPOSURE_TIME 0x006F
/** Total luminous energy. */
#define BT_MESH_PROP_ID_TOT_LUMINOUS_ENERGY 0x0070
/** Desired ambient temperature. */
#define BT_MESH_PROP_ID_DESIRED_AMB_TEMP 0x0071
/** @} */

#endif /* BT_MESH_PROPERTIES_H__ */

/** @} */
