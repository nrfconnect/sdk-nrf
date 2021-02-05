/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_light_ctrl_srv Light Lightness Control Server
 *  @ingroup bt_mesh_light_ctrl
 *  @{
 *  @brief Light Lightness Control Server model API
 */

#ifndef BT_MESH_LIGHT_CTRL_SRV_H__
#define BT_MESH_LIGHT_CTRL_SRV_H__

#include <bluetooth/mesh.h>
#include <bluetooth/mesh/light_ctrl.h>
#include <bluetooth/mesh/sensor.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/lightness_srv.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_ctrl_srv;

/** @def BT_MESH_LIGHT_CTRL_SRV_INIT
 *
 *  @brief Initialization parameters for @ref bt_mesh_light_ctrl_srv.
 *
 *  @param[in] _lightness_srv Pointer to the @ref bt_mesh_lightness_srv this
 *                            server controls.
 */
#define BT_MESH_LIGHT_CTRL_SRV_INIT(_lightness_srv)                            \
	{                                                                      \
		.cfg = BT_MESH_LIGHT_CTRL_SRV_CFG_INIT,                        \
		.onoff = BT_MESH_ONOFF_SRV_INIT(                               \
			&_bt_mesh_light_ctrl_srv_onoff),                       \
		.lightness = _lightness_srv,                                   \
		BT_MESH_LIGHT_CTRL_SRV_REG_INIT                                \
	}

/** @def BT_MESH_MODEL_LIGHT_CTRL_SRV
 *
 *  @brief Light Lightness model entry.
 *
 *  @param[in] _srv Pointer to a @ref bt_mesh_light_ctrl_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_CTRL_SRV(_srv)                                     \
	BT_MESH_MODEL_ONOFF_SRV(&(_srv)->onoff),                               \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LC_SRV,                        \
			 _bt_mesh_light_ctrl_srv_op, &(_srv)->pub,             \
			 BT_MESH_MODEL_USER_DATA(                              \
				 struct bt_mesh_light_ctrl_srv, _srv),         \
			 &_bt_mesh_light_ctrl_srv_cb),                         \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LC_SETUPSRV,                   \
			 _bt_mesh_light_ctrl_setup_srv_op,                     \
			 &(_srv)->setup_pub,                                   \
			 BT_MESH_MODEL_USER_DATA(                              \
				 struct bt_mesh_light_ctrl_srv, _srv),         \
			 &_bt_mesh_light_ctrl_setup_srv_cb)

/** Light Lightness Control Server state */
enum bt_mesh_light_ctrl_srv_state {
	/** Standby state */
	LIGHT_CTRL_STATE_STANDBY,
	/** On state */
	LIGHT_CTRL_STATE_ON,
	/** Prolong state */
	LIGHT_CTRL_STATE_PROLONG,

	/** The number of states. */
	LIGHT_CTRL_STATE_COUNT,
};

/** Light Lightness Control Server configuration. */
struct bt_mesh_light_ctrl_srv_cfg {
	/** Delay from occupancy detected until light turns on. */
	uint32_t occupancy_delay;
	/** Transition time to On state. */
	uint32_t fade_on;
	/** Time in On state. */
	uint32_t on;
	/** Transition time to Prolong state. */
	uint32_t fade_prolong;
	/** Time in Prolong state. */
	uint32_t prolong;
	/** Transition time to Standby state (in auto mode). */
	uint32_t fade_standby_auto;
	/** Transition time to Standby state (in manual mode). */
	uint32_t fade_standby_manual;
	/** State-wise light levels */
	uint16_t light[LIGHT_CTRL_STATE_COUNT];
};

/** Illumination regulator configuration */
struct bt_mesh_light_ctrl_srv_reg_cfg {
	/** Target illuminance values */
	struct sensor_value lux[LIGHT_CTRL_STATE_COUNT];
	/** Regulator upwards integral coefficient */
	float kiu;
	/** Regulator downwards integral coefficient */
	float kid;
	/** Regulator upwards propotional coefficient */
	float kpu;
	/** Regulator downwards propotional coefficient */
	float kpd;
	/** Regulator dead zone (in percent) */
	uint8_t accuracy;
};

/** Illumination regulator */
struct bt_mesh_light_ctrl_srv_reg {
	/** Regulator step timer */
	struct k_delayed_work timer;
	/** Internal integral sum. */
	float i;
	/** Previous output */
	uint16_t prev;
	/** Regulator configuration */
	struct bt_mesh_light_ctrl_srv_reg_cfg cfg;
};

/** @brief Light Lightness Control Server instance.
 *
 *  Should be initialized with @ref BT_MESH_LIGHT_CTRL_SRV_INIT.
 */
struct bt_mesh_light_ctrl_srv {
	/** Current control state */
	enum bt_mesh_light_ctrl_srv_state state;
	/** Internal flag field */
	atomic_t flags;
	/** Parameters for the start of current state */
	struct {
		/** Initial light level */
		uint16_t initial_light;
		/** Initial illumination level */
		struct sensor_value initial_lux;
		/** Fade duration */
		uint32_t duration;
	} fade;
	/** Present ambient illumination */
	struct sensor_value ambient_lux;
	/** State timer */
	struct k_delayed_work timer;

#if CONFIG_BT_SETTINGS
	/** Storage timer */
	struct k_delayed_work store_timer;
#endif
	/** Timer for delayed action */
	struct k_delayed_work action_delay;
	/** Configuration parameters */
	struct bt_mesh_light_ctrl_srv_cfg cfg;
	/** Publish parameters */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS, 3)];
	/** Setup model publish parameters */
	struct bt_mesh_model_pub setup_pub;
	/* Publication buffer */
	struct net_buf_simple setup_pub_buf;
	/* Publication data */
	uint8_t setup_pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)];

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	/** Illuminance regulator */
	struct bt_mesh_light_ctrl_srv_reg reg;
#endif

	/** Lightness server instance */
	struct bt_mesh_lightness_srv *lightness;
	/** Extended Generic OnOff server */
	struct bt_mesh_onoff_srv onoff;
	/** Transaction ID tracking context */
	struct bt_mesh_tid_ctx tid;
	/** Composition data server model instance */
	struct bt_mesh_model *model;
	/** Composition data setup server model instance */
	struct bt_mesh_model *setup_srv;
	/** Scene entry */
	struct bt_mesh_scene_entry scene;
};

/** @brief Turn the light on.
 *
 *  Instructs the controlled Lightness Server to turn the light on. If the light
 *  was already on, the dimming timeout is reset. If the light was in the
 *  Prolong state, it's moved back into the On state.
 *
 *  @param[in] srv        Light Lightness Control Server instance.
 *
 *  @retval 0      The Light Lightness Control Server was successfully turned
 *                 on.
 *  @retval -EBUSY The Light Lightness Control Server is disabled.
 */
int bt_mesh_light_ctrl_srv_on(struct bt_mesh_light_ctrl_srv *srv);

/** @brief Manually turn the light off.
 *
 *  Instructs the controlled Lightness Server to turn the light off (Standby
 *  state). Calling this function temporarily disables occupancy sensor
 *  triggering (referred to as "manual mode" in the documentation). The server
 *  will remain in manual mode until the manual mode timer expires, see
 *  @option{CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_MANUAL}.
 *
 *  @param[in] srv        Light Lightness Control Server instance.
 *
 *  @retval 0      The Light Lightness Control Server was successfully turned
 *                 off.
 *  @retval -EBUSY The Light Lightness Control Server is disabled.
 */
int bt_mesh_light_ctrl_srv_off(struct bt_mesh_light_ctrl_srv *srv);

/** @brief Enable the Light Lightness Control Server.
 *
 *  The Server must be enabled to take control of the Lightness Server.
 *
 *  @param[in] srv Light Lightness Control Server instance.
 *
 *  @retval 0         The Light Lightness Control Server was successfully
 *                    enabled.
 *  @retval -EALREADY The Light Lightness Control Server was already enabled.
 */
int bt_mesh_light_ctrl_srv_enable(struct bt_mesh_light_ctrl_srv *srv);

/** @brief Disable the Light Lightness Control Server.
 *
 *  The server must be enabled to take control of the Lightness Server.
 *  Disabling the server disengages the control over the Lightness Server,
 *  which will start running as an independent model.
 *
 *  @param[in] srv Light Lightness Control Server instance.
 *
 *  @retval 0         The Light Lightness Control Server was successfully
 *                    enabled.
 *  @retval -EALREADY The Light Lightness Control Server was already enabled.
 */
int bt_mesh_light_ctrl_srv_disable(struct bt_mesh_light_ctrl_srv *srv);

/** @brief Check if the Light Lightness Control Server has turned the light on.
 *
 *  @param[in] srv Light Lightness Control Server instance.
 *
 *  @return true if the Lightness Server's light is on because of its binding
 *          with the Light Lightness Control Server, false otherwise.
 */
bool bt_mesh_light_ctrl_srv_is_on(struct bt_mesh_light_ctrl_srv *srv);

/** @brief Publish the current OnOff state.
 *
 *  @param[in] srv Light Lightness Control Server instance.
 *  @param[in] ctx Message context, or NULL to publish with the configured
 *                 parameters.
 *
 *  @return 0              Successfully published the current Light state.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_srv_pub(struct bt_mesh_light_ctrl_srv *srv,
			       struct bt_mesh_msg_ctx *ctx);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_light_ctrl_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_light_ctrl_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_ctrl_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_light_ctrl_setup_srv_op[];
extern const struct bt_mesh_onoff_srv_handlers _bt_mesh_light_ctrl_srv_onoff;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTRL_SRV_H__ */

/** @} */
