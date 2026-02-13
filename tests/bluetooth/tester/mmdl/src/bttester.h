/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>

#define BTP_MTU 1024
#define BTP_DATA_MAX_SIZE (BTP_MTU - sizeof(struct btp_hdr))

#define BTP_INDEX_NONE		0xff

#define BTP_SERVICE_ID_CORE	0
#define BTP_SERVICE_ID_GAP	1
#define BTP_SERVICE_ID_MESH	4
#define BTP_SERVICE_ID_MMDL	5

#define BTP_STATUS_SUCCESS	0x00
#define BTP_STATUS_FAILED	0x01
#define BTP_STATUS_UNKNOWN_CMD	0x02
#define BTP_STATUS_NOT_READY	0x03

struct btp_hdr {
	uint8_t  service;
	uint8_t  opcode;
	uint8_t  index;
	uint16_t len;
	uint8_t  data[];
} __packed;

#define BTP_STATUS			0x00
struct btp_status {
	uint8_t code;
} __packed;

/* Core Service */
#define CORE_READ_SUPPORTED_COMMANDS	0x01
struct core_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define CORE_READ_SUPPORTED_SERVICES	0x02
struct core_read_supported_services_rp {
	uint8_t data[0];
} __packed;

#define CORE_REGISTER_SERVICE		0x03
struct core_register_service_cmd {
	uint8_t id;
} __packed;

#define CORE_UNREGISTER_SERVICE		0x04
struct core_unregister_service_cmd {
	uint8_t id;
} __packed;

/* events */
#define CORE_EV_IUT_READY		0x80

/* GAP Service */
/* commands */

#define GAP_SETTINGS_POWERED		0
#define GAP_SETTINGS_CONNECTABLE	1
#define GAP_SETTINGS_FAST_CONNECTABLE	2
#define GAP_SETTINGS_DISCOVERABLE	3
#define GAP_SETTINGS_BONDABLE		4
#define GAP_SETTINGS_LINK_SEC_3		5
#define GAP_SETTINGS_SSP		6
#define GAP_SETTINGS_BREDR		7
#define GAP_SETTINGS_HS			8
#define GAP_SETTINGS_LE			9
#define GAP_SETTINGS_ADVERTISING	10
#define GAP_SETTINGS_SC			11
#define GAP_SETTINGS_DEBUG_KEYS		12
#define GAP_SETTINGS_PRIVACY		13
#define GAP_SETTINGS_CONTROLLER_CONFIG	14
#define GAP_SETTINGS_STATIC_ADDRESS	15

#define GAP_READ_CONTROLLER_INFO	0x03
struct gap_read_controller_info_rp {
	uint8_t  address[6];
	uint32_t supported_settings;
	uint32_t current_settings;
	uint8_t  cod[3];
	uint8_t  name[249];
	uint8_t  short_name[11];
} __packed;

static inline void tester_set_bit(uint8_t *addr, unsigned int bit)
{
	uint8_t *p = addr + (bit / 8U);

	*p |= BIT(bit % 8);
}

static inline uint8_t tester_test_bit(const uint8_t *addr, unsigned int bit)
{
	const uint8_t *p = addr + (bit / 8U);

	return *p & BIT(bit % 8);
}

/* MESH Service */
/* commands */
#define MESH_READ_SUPPORTED_COMMANDS	0x01
struct mesh_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define MESH_OUT_BLINK			BIT(0)
#define MESH_OUT_BEEP			BIT(1)
#define MESH_OUT_VIBRATE		BIT(2)
#define MESH_OUT_DISPLAY_NUMBER		BIT(3)
#define MESH_OUT_DISPLAY_STRING		BIT(4)

#define MESH_IN_PUSH			BIT(0)
#define MESH_IN_TWIST			BIT(1)
#define MESH_IN_ENTER_NUMBER		BIT(2)
#define MESH_IN_ENTER_STRING		BIT(3)

#define MESH_CONFIG_PROVISIONING	0x02
struct mesh_config_provisioning_cmd {
	uint8_t uuid[16];
	uint8_t static_auth[16];
	uint8_t out_size;
	uint16_t out_actions;
	uint8_t in_size;
	uint16_t in_actions;
} __packed;

#define MESH_PROVISION_NODE		0x03
struct mesh_provision_node_cmd {
	uint8_t net_key[16];
	uint16_t net_key_idx;
	uint8_t flags;
	uint32_t iv_index;
	uint32_t seq_num;
	uint16_t addr;
	uint8_t dev_key[16];
} __packed;

#define MESH_INIT			0x04
#define MESH_RESET			0x05
#define MESH_START			0x78

/* events */
#define MESH_EV_OUT_NUMBER_ACTION	0x80
struct mesh_out_number_action_ev {
	uint16_t action;
	uint32_t number;
} __packed;

#define MESH_EV_OUT_STRING_ACTION	0x81
struct mesh_out_string_action_ev {
	uint8_t string_len;
	uint8_t string[];
} __packed;

#define MESH_EV_IN_ACTION		0x82
struct mesh_in_action_ev {
	uint16_t action;
	uint8_t size;
} __packed;

#define MESH_EV_PROVISIONED		0x83

#define MESH_PROV_BEARER_PB_ADV		0x00
#define MESH_PROV_BEARER_PB_GATT	0x01
#define MESH_EV_PROV_LINK_OPEN		0x84
struct mesh_prov_link_open_ev {
	uint8_t bearer;
} __packed;

#define MESH_EV_PROV_LINK_CLOSED	0x85
struct mesh_prov_link_closed_ev {
	uint8_t bearer;
} __packed;

#define MESH_EV_NET_RECV		0x86
struct mesh_net_recv_ev {
	uint8_t ttl;
	uint8_t ctl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define MESH_EV_INVALID_BEARER		0x87
struct mesh_invalid_bearer_ev {
	uint8_t opcode;
} __packed;

#define MESH_EV_INCOMP_TIMER_EXP	0x88

/* MMDL Service */
struct mesh_model_transition {
	uint8_t time;
	uint8_t delay;
} __packed;

/* commands */
#define MMDL_READ_SUPPORTED_COMMANDS	0x01
struct mmdl_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define MMDL_GEN_ONOFF_GET	0x02

#define MMDL_GEN_ONOFF_SET	0x03
struct mesh_gen_onoff_set {
	uint8_t ack;
	uint8_t onoff;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_GEN_LVL_GET	0x04

#define MMDL_GEN_LVL_SET	0x05
struct mesh_gen_lvl_set {
	uint8_t ack;
	int16_t lvl;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_GEN_LVL_DELTA_SET	0x06
struct mesh_gen_lvl_delta_set {
	uint8_t ack;
	int32_t delta;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_GEN_LVL_MOVE_SET	0x07
struct mesh_gen_lvl_move_set {
	uint8_t ack;
	int16_t delta;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_GEN_DTT_GET	0x08

#define MMDL_GEN_DTT_SET	0x09
struct mesh_gen_dtt_set {
	uint8_t ack;
	uint8_t transition_time;
} __packed;

#define MMDL_GEN_PONOFF_GET	0x0a

#define MMDL_GEN_PONOFF_SET	0x0b
struct mesh_gen_ponoff_set {
	uint8_t ack;
	uint8_t on_power_up;
} __packed;

#define MMDL_GEN_PLVL_GET	0x0c

#define MMDL_GEN_PLVL_SET	0x0d
struct mesh_gen_plvl_set {
	uint8_t ack;
	uint16_t power_lvl;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_GEN_PLVL_LAST_GET	0x0e

#define MMDL_GEN_PLVL_DFLT_GET	0x0f

#define MMDL_GEN_PLVL_DFLT_SET	0x10
struct mesh_gen_plvl_dflt_set {
	uint8_t ack;
	uint16_t power_dflt;
} __packed;

#define MMDL_GEN_PLVL_RANGE_GET	0x11

#define MMDL_GEN_PLVL_RANGE_SET	0x12
struct mesh_gen_plvl_range_set {
	uint8_t ack;
	uint16_t range_min;
	uint16_t range_max;
} __packed;

#define MMDL_GEN_BATTERY_GET	0x13

#define MMDL_GEN_LOC_GLOBAL_GET	0x14

#define MMDL_GEN_LOC_LOCAL_GET	0x15

#define MMDL_GEN_LOC_GLOBAL_SET	0x16
struct mesh_gen_loc_global_set {
	uint8_t ack;
	uint32_t latitude;
	uint32_t longitude;
	uint16_t altitude;
} __packed;

#define MMDL_GEN_LOC_LOCAL_SET	0x17
struct mesh_gen_loc_local_set {
	uint8_t ack;
	uint16_t north;
	uint16_t east;
	uint16_t altitude;
	uint8_t floor;
	uint16_t loc_uncertainty;
} __packed;

#define MMDL_GEN_PROPS_GET	0x18
struct mesh_gen_props_get {
	uint8_t kind;
	uint16_t id;
} __packed;

#define MMDL_GEN_PROP_GET	0x19
struct mesh_gen_prop_get {
	uint8_t kind;
	uint16_t id;
} __packed;

#define MMDL_GEN_PROP_SET	0x1a
struct mesh_gen_prop_set {
	uint8_t ack;
	uint8_t kind;
	uint16_t id;
	uint8_t access;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define MMDL_SENSOR_DESC_GET	0x1b
struct mesh_sensor_desc_get {
	uint16_t id;
} __packed;

#define MMDL_SENSOR_GET	0x1c
struct mesh_sensor_get {
	uint16_t id;
} __packed;

#define MMDL_SENSOR_COLUMN_GET	0x1d
struct mesh_sensor_column_get {
	uint16_t id;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define MMDL_SENSOR_SERIES_GET	0x1e
struct mesh_sensor_series_get {
	uint16_t id;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define MMDL_SENSOR_CADENCE_GET	0x1f
struct mesh_sensor_cadence_get {
	uint16_t id;
} __packed;

#define MMDL_SENSOR_CADENCE_SET	0x20
struct mesh_sensor_cadence_set {
	uint8_t ack;
	uint16_t id;
	uint16_t len;
	uint8_t data[0];
} __packed;

#define MMDL_SENSOR_SETTINGS_GET	0x21
struct mesh_sensor_settings_get {
	uint16_t id;
} __packed;

#define MMDL_SENSOR_SETTING_GET	0x22
struct mesh_sensor_setting_get {
	uint16_t id;
	uint16_t setting_id;
} __packed;

#define MMDL_SENSOR_SETTING_SET	0x23
struct mesh_sensor_setting_set {
	uint8_t ack;
	uint16_t id;
	uint16_t setting_id;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define MMDL_TIME_GET	0x24
#define MMDL_TIME_SET	0x25
struct mesh_time_set {
	uint8_t tai[5];
	uint8_t subsecond;
	uint8_t uncertainty;
	uint16_t tai_utc_delta;
	uint8_t time_zone_offset;
} __packed;

#define MMDL_TIME_ROLE_GET	0x26
#define MMDL_TIME_ROLE_SET	0x27
struct mesh_time_role_set {
	uint8_t role;
} __packed;

#define MMDL_TIME_ZONE_GET	0x28
#define MMDL_TIME_ZONE_SET	0x29
struct mesh_time_zone_set {
	int16_t new_offset;
	uint64_t timestamp;
} __packed;

#define MMDL_TIME_TAI_UTC_DELTA_GET	0x2a
#define MMDL_TIME_TAI_UTC_DELTA_SET	0x2b
struct mesh_time_tai_utc_delta_set {
	int16_t delta_new;
	uint64_t timestamp;
} __packed;

#define MMDL_LIGHT_LIGHTNESS_GET	0x2c
#define MMDL_LIGHT_LIGHTNESS_SET	0x2d
struct mesh_light_lightness_set {
	uint8_t ack;
	uint16_t lightness;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_LIGHTNESS_LINEAR_GET	0x2e
#define MMDL_LIGHT_LIGHTNESS_LINEAR_SET	0x2f

#define MMDL_LIGHT_LIGHTNESS_LAST_GET	0x30
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_GET	0x31
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_SET	0x32
struct mesh_light_lightness_default_set {
	uint8_t ack;
	uint16_t dflt;
} __packed;

#define MMDL_LIGHT_LIGHTNESS_RANGE_GET	0x33
#define MMDL_LIGHT_LIGHTNESS_RANGE_SET	0x34
struct mesh_light_lightness_range_set {
	uint8_t ack;
	uint16_t min;
	uint16_t max;
} __packed;

#define MMDL_LIGHT_LC_MODE_GET	0x35
#define MMDL_LIGHT_LC_MODE_SET	0x36
struct mesh_light_lc_mode_set {
	uint8_t ack;
	uint8_t enabled;
} __packed;

#define MMDL_LIGHT_LC_OCCUPANCY_MODE_GET	0x37
#define MMDL_LIGHT_LC_OCCUPANCY_MODE_SET	0x38
struct mesh_light_lc_occupancy_mode_set {
	uint8_t ack;
	uint8_t enabled;
} __packed;

#define MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_GET	0x39
#define MMDL_LIGHT_LC_LIGHT_ONOFF_MODE_SET	0x3a
struct mesh_light_lc_light_onoff_mode_set {
	uint8_t ack;
	uint8_t onoff;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_LC_PROPERTY_GET	0x3b
struct mesh_light_lc_property_get {
	uint16_t id;
} __packed;

#define MMDL_LIGHT_LC_PROPERTY_SET	0x3c
struct mesh_light_lc_property_set {
	uint8_t ack;
	uint16_t id;
	uint16_t val;
} __packed;

#define MMDL_SENSOR_DATA_SET	0x3d
struct mesh_sensor_data_set_cmd {
	uint16_t prop_id;
	uint8_t len;
	uint8_t data[];
} __packed;

#define MMDL_LIGHT_CTL_STATES_GET	0x3e
#define MMDL_LIGHT_CTL_STATES_SET	0x3f
struct mesh_light_ctl_states_set {
	uint8_t ack;
	uint16_t light;
	uint16_t temp;
	int16_t delta_uv;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_CTL_TEMPERATURE_GET	0x40
#define MMDL_LIGHT_CTL_TEMPERATURE_SET	0x41
struct mesh_light_ctl_temperature_set {
	uint8_t ack;
	uint16_t temp;
	int16_t delta_uv;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_CTL_DEFAULT_GET	0x42
#define MMDL_LIGHT_CTL_DEFAULT_SET	0x43
struct mesh_light_ctl_default_set {
	uint8_t ack;
	uint16_t light;
	uint16_t temp;
	int16_t delta_uv;
} __packed;
#define MMDL_LIGHT_CTL_TEMPERATURE_RANGE_GET	0x44
#define MMDL_LIGHT_CTL_TEMPERATURE_RANGE_SET	0x45
struct mesh_light_ctl_temp_range_set {
	uint8_t ack;
	uint16_t min;
	uint16_t max;
} __packed;

#define MMDL_SCENE_GET	0x46
#define MMDL_SCENE_REGISTER_GET	0x47
#define MMDL_SCENE_STORE_PROCEDURE	0x48
struct mesh_scene_ctl_store_procedure {
	uint8_t ack;
	uint16_t scene;
} __packed;
#define MMDL_SCENE_RECALL	0x49
struct mesh_scene_ctl_recall {
	uint8_t ack;
	uint16_t scene;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_XYL_GET 0x4a
#define MMDL_LIGHT_XYL_SET 0x4b
struct mesh_light_xyl_set {
	uint8_t ack;
	uint16_t lightness;
	uint16_t x;
	uint16_t y;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_XYL_TARGET_GET 0x4c
#define MMDL_LIGHT_XYL_DEFAULT_GET 0x4d
#define MMDL_LIGHT_XYL_DEFAULT_SET 0x4e
struct mesh_light_xyl_default_set {
	uint8_t ack;
	uint16_t lightness;
	uint16_t x;
	uint16_t y;
} __packed;

#define MMDL_LIGHT_XYL_RANGE_GET 0x4f
#define MMDL_LIGHT_XYL_RANGE_SET 0x50
struct mesh_light_xyl_range_set {
	uint8_t ack;
	uint16_t min_x;
	uint16_t min_y;
	uint16_t max_x;
	uint16_t max_y;
} __packed;

#define MMDL_LIGHT_HSL_GET 0x51
#define MMDL_LIGHT_HSL_SET 0x52
struct mesh_light_hsl_set {
	uint8_t ack;
	uint16_t lightness;
	uint16_t hue;
	uint16_t saturation;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_HSL_TARGET_GET 0x53
#define MMDL_LIGHT_HSL_DEFAULT_GET 0x54
#define MMDL_LIGHT_HSL_DEFAULT_SET 0x55
struct mesh_light_hsl_default_set {
	uint8_t ack;
	uint16_t lightness;
	uint16_t hue;
	uint16_t saturation;
} __packed;

#define MMDL_LIGHT_HSL_RANGE_GET 0x56
#define MMDL_LIGHT_HSL_RANGE_SET 0x57
struct mesh_light_hsl_range_set {
	uint8_t ack;
	uint16_t hue_min;
	uint16_t saturation_min;
	uint16_t hue_max;
	uint16_t saturation_max;
} __packed;

#define MMDL_LIGHT_HSL_HUE_GET 0x58
#define MMDL_LIGHT_HSL_HUE_SET 0x59
struct mesh_light_hsl_hue_set {
	uint8_t ack;
	uint16_t hue;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_LIGHT_HSL_SATURATION_GET 0x5a
#define MMDL_LIGHT_HSL_SATURATION_SET 0x5b
struct mesh_light_hsl_saturation_set {
	uint8_t ack;
	uint16_t saturation;
	struct mesh_model_transition transition[0];
} __packed;

#define MMDL_SCHEDULER_GET 0x5c
#define MMDL_SCHEDULER_ACTION_GET 0x5d
struct mesh_scheduler_action_get {
	uint8_t index;
} __packed;

#define MMDL_SCHEDULER_ACTION_SET 0x5e
struct mesh_scheduler_action_set {
	uint8_t ack;
	uint8_t index;
	uint8_t year;
	uint16_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t day_of_week;
	uint8_t action;
	uint8_t transition_time;
	uint16_t scene_number;
} __packed;

void tester_init(void);
void tester_rsp(uint8_t service, uint8_t opcode, uint8_t index, uint8_t status);
void tester_send(uint8_t service, uint8_t opcode, uint8_t index, uint8_t *data,
		 size_t len);

uint8_t tester_init_gap(void);
uint8_t tester_unregister_gap(void);
void tester_handle_gap(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len);
uint8_t tester_init_gatt(void);
uint8_t tester_unregister_gatt(void);
void tester_handle_gatt(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len);

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
uint8_t tester_init_l2cap(void);
uint8_t tester_unregister_l2cap(void);
void tester_handle_l2cap(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len);
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

#if defined(CONFIG_BT_MESH)
uint8_t tester_init_mesh(void);
uint8_t tester_unregister_mesh(void);
void tester_handle_mesh(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len);
uint8_t tester_init_mmdl(void);
uint8_t tester_unregister_mmdl(void);
void tester_handle_mmdl(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len);
#endif /* CONFIG_BT_MESH */
