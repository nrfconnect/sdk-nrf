/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_FMDN_H_
#define BT_FAST_PAIR_FMDN_H_

#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

/**
 * @defgroup bt_fast_pair_fmdn Fast Pair FMDN API
 * @brief Fast Pair FMDN API
 *
 *  It is required to use the Fast Pair FMDN API in the cooperative thread context
 *  (for example, system workqueue thread). Following this requirement guarantees
 *  a proper synchronization between the user operations and the module operations.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Ringing activity source types. */
enum bt_fast_pair_fmdn_ring_src {
	/** Ringing source type originating from the Bluetooth Fast Pair service that
	 *  is defined in the FMDN Accessory specification (Beacon Actions operations).
	 */
	BT_FAST_PAIR_FMDN_RING_SRC_FMDN_BT_GATT,

	/** Ringing source type originating from the Bluetooth accessory non-owner
	 *  service that is defined in the DULT specification.
	 *  Used only when the CONFIG_BT_FAST_PAIR_FMDN_DULT is enabled.
	 */
	BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT,
};

/** Ringing component identifiers. */
enum bt_fast_pair_fmdn_ring_comp {
	/** Identifier of the right component. */
	BT_FAST_PAIR_FMDN_RING_COMP_RIGHT = BIT(0),

	/** Identifier of the left component. */
	BT_FAST_PAIR_FMDN_RING_COMP_LEFT = BIT(1),

	/** Identifier of the case component. */
	BT_FAST_PAIR_FMDN_RING_COMP_CASE = BIT(2),
};

/** Ringing component bitmask with no active component (typically used to
 *  stop the ongoing ringing action).
 */
#define BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE (0x00)

/** Special value of the ringing component bitmask to request ringing
 *  from all available components on the device. This value can only
 *  appear in the callback request and cannot be used in the application
 *  response passed to the @ref bt_fast_pair_fmdn_ring_state_update API.
 */
#define BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL (0xFF)

/** Ringing volume. */
enum bt_fast_pair_fmdn_ring_volume {
	/** Default level of the ringing volume. */
	BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT = 0x00,

	/** Low level of the ringing volume. */
	BT_FAST_PAIR_FMDN_RING_VOLUME_LOW = 0x01,

	/** Medium level of the ringing volume. */
	BT_FAST_PAIR_FMDN_RING_VOLUME_MEDIUM = 0x02,

	/** High level of the ringing volume. */
	BT_FAST_PAIR_FMDN_RING_VOLUME_HIGH = 0x03,
};

/** Number of milliseconds per decisecond. */
#define BT_FAST_PAIR_FMDN_RING_MSEC_PER_DSEC (100U)

/** @brief Convert the ringing timeout value from deciseconds to milliseconds.
 *
 * Deciseconds are the native time unit used by the FMDN extension.
 *
 * @param _timeout_ds The ringing timeout value in deciseconds.
 */
#define BT_FAST_PAIR_FMDN_RING_TIMEOUT_DS_TO_MS(_timeout_ds) \
	((_timeout_ds) * BT_FAST_PAIR_FMDN_RING_MSEC_PER_DSEC)

/** @brief Convert the ringing timeout value from milliseconds to deciseconds.
 *
 * Deciseconds are the native time unit used by the FMDN extension.
 *
 * @param _timeout_ms The ringing timeout value in milliseconds.
 */
#define BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(_timeout_ms) \
	((_timeout_ms) / BT_FAST_PAIR_FMDN_RING_MSEC_PER_DSEC)

/** Ringing request parameters. */
struct bt_fast_pair_fmdn_ring_req_param {
	/** Bitmask with the active ringing components that is composed of
	 *  the @ref bt_fast_pair_fmdn_ring_comp identifiers.
	 */
	uint8_t active_comp_bm;

	/** Ringing timeout in deciseconds. */
	uint16_t timeout;

	/** Ringing volume using @ref bt_fast_pair_fmdn_ring_volume values. */
	enum bt_fast_pair_fmdn_ring_volume volume;
};

/** Ringing callback structure. */
struct bt_fast_pair_fmdn_ring_cb {
	/** @brief Request the user to start the ringing action.
	 *
	 *  This callback is called to start the ringing action. The FMDN
	 *  module requests this action in response to the command from the
	 *  connected peer. Eventually, the action times out, which is
	 *  indicated by the @ref timeout_expired callback. This action can
	 *  also be cancelled by the connected peer (see the @ref stop_request
	 *  callback) or stopped manually by the user action (see the
	 *  @ref bt_fast_pair_fmdn_ring_state_update API and the
	 *  @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_UI_STOPPED trigger).
	 *
	 *  The input parameters determine how the ringing actions should
	 *  be executed. See @ref bt_fast_pair_fmdn_ring_req_param for more
	 *  details.
	 *
	 *  If the action is accepted for at least one requested component, you
	 *  shall indicate it using the @ref bt_fast_pair_fmdn_ring_state_update API
	 *  and set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED as a trigger
	 *  for the ringing state change. If all components are out of range, you
	 *  shall set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED as a trigger.
	 *
	 *  This callback can be called again when the ringing action has already
	 *  started. In this case, you shall update the ringing activity to match
	 *  the newest set of parameters.
	 *
	 *  If you cannot start the ringing action on all requested components
	 *  (for example, one of them is out of range), you shall still declare
	 *  success for this request. Once an unavailable component becomes reachable,
	 *  you can start the ringing action on it and indicate it using the
	 *  @ref bt_fast_pair_fmdn_ring_state_update API.
	 *
	 *  This callback is executed in the cooperative thread context. You
	 *  can learn about the exact thread context by analyzing the
	 *  CONFIG_BT_RECV_CONTEXT configuration choice. By default, this
	 *  callback is executed in the Bluetooth-specific workqueue thread
	 *  (CONFIG_BT_RECV_WORKQ_BT).
	 *
	 *  @param src   Source of the ringing activity.
	 *  @param param Requested ringing parameters.
	 */
	void (*start_request)(enum bt_fast_pair_fmdn_ring_src src,
			      const struct bt_fast_pair_fmdn_ring_req_param *param);

	/** @brief Request the user to stop the ringing action on timeout.
	 *
	 *  This callback is called to stop the ongoing ringing action. The
	 *  FMDN module requests this action when the ringing timeout expires.
	 *
	 *  If the action is accepted for at least one requested component, you
	 *  shall indicate it using the @ref bt_fast_pair_fmdn_ring_state_update API
	 *  and set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_TIMEOUT_STOPPED as
	 *  a trigger for the ringing state change. If all components are out of
	 *  range, you shall set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED
	 *  as a trigger.
	 *
	 *  This callback is called when at least one ringing component is active.
	 *  The timeout is not stopped unless the API user reports that ringing is
	 *  stopped for all of the components.
	 *
	 *  If you cannot stop the ringing action on all requested components
	 *  (for example, one of them is out of range), you shall still declare
	 *  success for this request. Once an unavailable component becomes reachable,
	 *  you can stop the ringing action on it and indicate it using the
	 *  @ref bt_fast_pair_fmdn_ring_state_update API.
	 *
	 *  This callback is executed in the cooperative thread context - in
	 *  the system workqueue thread.
	 *
	 *  @param src Source of the ringing activity.
	 */
	void (*timeout_expired)(enum bt_fast_pair_fmdn_ring_src src);

	/** @brief Request the user to stop the ringing action on GATT request.
	 *
	 *  This callback is called to stop the ongoing ringing action. The
	 *  FMDN module requests this action in response to the command
	 *  from the connected peer.
	 *
	 *  If the action is accepted for at least one requested component, you
	 *  shall indicate it using the @ref bt_fast_pair_fmdn_ring_state_update API
	 *  and set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_GATT_STOPPED as
	 *  a trigger for the ringing state change. If all components are out of
	 *  range, you shall set the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED
	 *  as a trigger.
	 *
	 *  If you cannot stop the ringing action on all requested components
	 *  (for example, one of them is out of range), you shall still declare
	 *  success for this request. Once an unavailable component becomes reachable,
	 *  you can stop the ringing action on it and indicate it using the
	 *  @ref bt_fast_pair_fmdn_ring_state_update API.
	 *
	 *  This callback is executed in the cooperative thread context. You
	 *  can learn about the exact thread context by analyzing the
	 *  CONFIG_BT_RECV_CONTEXT configuration choice. By default, this
	 *  callback is executed in the Bluetooth-specific workqueue thread
	 *  (CONFIG_BT_RECV_WORKQ_BT).
	 *
	 *  @param src Source of the ringing activity.
	 */
	void (*stop_request)(enum bt_fast_pair_fmdn_ring_src src);
};

/** @brief Register the ringing callbacks in the FMDN module.
 *
 *  This function registers the ringing callbacks. If you declare at least one
 *  ringing component using the CONFIG_BT_FAST_PAIR_FMDN_RING_COMP Kconfig
 *  choice option, you shall call this API before you enable Fast Pair with the
 *  @ref bt_fast_pair_enable function. Otherwise, the enable operation fails.
 *
 *  You can call this function only in the disabled state of the FMDN module
 *  (see @ref bt_fast_pair_is_ready function).
 *
 *  @param cb Ringing callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_ring_cb_register(const struct bt_fast_pair_fmdn_ring_cb *cb);

/** Trigger for the new ringing state. */
enum bt_fast_pair_fmdn_ring_trigger {
	/** Ringing action started. */
	BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED = 0x00,

	/** Ringing action failed (all requested components are out of range). */
	BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED = 0x01,

	/** Ringing action stopped due to the timeout. */
	BT_FAST_PAIR_FMDN_RING_TRIGGER_TIMEOUT_STOPPED = 0x02,

	/** Ringing action stopped due to the UI action (e.g button press, touch sense). */
	BT_FAST_PAIR_FMDN_RING_TRIGGER_UI_STOPPED = 0x03,

	/** Ringing action stopped due to the GATT request. */
	BT_FAST_PAIR_FMDN_RING_TRIGGER_GATT_STOPPED = 0x04,
};

/** Ringing state parameters. */
struct bt_fast_pair_fmdn_ring_state_param {
	/** Trigger for the new ringing state. */
	enum bt_fast_pair_fmdn_ring_trigger trigger;

	/** Bitmask with the active ringing components that is composed of
	 *  the @ref bt_fast_pair_fmdn_ring_comp identifiers.
	 */
	uint8_t active_comp_bm;

	/** Ringing timeout in deciseconds.
	 *  Relevant only for the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED trigger
	 *  Set to zero to preserve the existing timeout.
	 */
	uint16_t timeout;
};

/** @brief Update the ringing state in the FMDN module.
 *
 *  This function updates the ringing state in the FMDN module and notifies
 *  connected peers about the change. The application user is responsible
 *  for setting the correct state that reflects the actual state of their
 *  ringing components.
 *
 *  You shall use this API to respond to the callbacks defined by the
 *  @ref bt_fast_pair_fmdn_ring_cb structure. It is also possible to use
 *  this function to change the ringing state asynchronously. For example,
 *  you shall call this API when the user stops the ringing action manually
 *  (see the @ref BT_FAST_PAIR_FMDN_RING_TRIGGER_UI_STOPPED trigger). In
 *  other cases, you may need to update the ringing state asynchronously to
 *  recover from the failure. For example, you can indicate the state change
 *  once you stop the ringing action on a previously unavailable component.
 *
 *  @param src   Source of the ringing activity that triggered state update.
 *  @param param Ringing state parameters.
 *               See the @ref bt_fast_pair_fmdn_ring_state_param for
 *               the detailed description.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_ring_state_update(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_state_param *param);

/** Unknown battery level. */
#define BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE (0xFF)

/** @brief Set the current battery level.
 *
 *  This function sets the current battery level. It is recommended to
 *  initialize the battery level with this API before you enable Fast Pair
 *  with the @ref bt_fast_pair_enable API.
 *
 *  By default, the @ref BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE value is used,
 *  which means that battery levels do not show in the advertising payload.
 *  If you do not want to support the battery level indication, you should
 *  ignore this API and never call it in their application.
 *
 *  However, if the CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT Kconfig is enabled,
 *  you must initialize battery level with this API before you enable Fast Pair
 *  with the @ref bt_fast_pair_enable API. This requirement is necessary as the
 *  DULT battery mechanism does not support unknown battery levels. As a result,
 *  you must not call this API with the @ref BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE
 *  value in this configuration variant.
 *
 *  To keep the Android battery level indications accurate, you should set the
 *  battery level to the new value with the help of this API as soon as the device
 *  battery level changes.
 *
 *  The exact mapping of the battery percentage to the battery level as defined by the
 *  FMDN Accessory specification in the advertising payload is implementation-specific.
 *  The mapping configuration is controlled by the following Kconfig options:
 *  CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR and
 *  CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR.
 *
 *  @param percentage_level Battery level as a percentage [0-100%] or
 *                          @ref BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE value if the
 *                          battery level is unknown.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_battery_level_set(uint8_t percentage_level);

/** Helper to declare FMDN advertising parameters inline.
 *
 * @param _int_min     Minimum advertising interval.
 * @param _int_max     Maximum advertising interval.
 */
#define BT_FAST_PAIR_FMDN_ADV_PARAM_INIT(_int_min, _int_max) \
{                                                            \
	.interval_min = (_int_min),                          \
	.interval_max = (_int_max),                          \
}

/** Default value of FMDN advertising parameters. */
#define BT_FAST_PAIR_FMDN_ADV_PARAM_DEFAULT                  \
	((struct bt_fast_pair_fmdn_adv_param[]) {            \
		BT_FAST_PAIR_FMDN_ADV_PARAM_INIT(            \
			0x0C80, /* 2s min interval */        \
			0x0C80  /* 2s max interval */        \
	)})

/** FMDN advertising parameters. */
struct bt_fast_pair_fmdn_adv_param {
	/** Minimum Advertising Interval (N * 0.625 milliseconds).
	 *  Range: 0x0020 to 0x4000.
	 */
	uint32_t interval_min;

	/** Maximum Advertising Interval (N * 0.625 milliseconds).
	 *  Range: 0x0020 to 0x4000.
	 */
	uint32_t interval_max;
};

/** @brief Set the FMDN advertising parameters.
 *
 *  This function sets the advertising parameters. It is recommended to
 *  initialize the advertising parameters with this API before you enable
 *  Fast Pair with the @ref bt_fast_pair_enable API. Otherwise, the default
 *  value @ref BT_FAST_PAIR_FMDN_ADV_PARAM_DEFAULT is used for advertising.
 *
 *  You can use this function to dynamically update advertising parameters
 *  during an ongoing FMDN advertising.
 *
 *  The API user is responsible for adjusting this configuration to their
 *  application requirements. The advertising intervals parameters from
 *  this API have additional constraints when you enable the Fast
 *  Pair advertising together with FMDN advertising (see respective
 *  specifications for details). This function does not validate if
 *  these constraints are met.
 *
 *  @param adv_param Configuration parameters for FMDN advertising.
 *                   See the @ref bt_fast_pair_fmdn_adv_param for the
 *                   detailed description.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_adv_param_set(
	const struct bt_fast_pair_fmdn_adv_param *adv_param);

/** @brief Set the Bluetooth identity for the FMDN extension.
 *
 *  This function sets the Bluetooth identity for the FMDN extension.
 *  This identity shall be created with the bt_id_create function
 *  that is available in the Bluetooth API. You can change the
 *  identity with this API only in the disabled state of the Fast Pair module
 *  before you enable it with the @ref bt_fast_pair_enable API. If you do not
 *  explicitly set the identity with this API call, the default identity,
 *  BT_ID_DEFAULT, is used.
 *
 *  @param id Bluetooth identity for the FMDN extension.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_id_set(uint8_t id);

/** Information callback structure. */
struct bt_fast_pair_fmdn_info_cb {
	/** @brief Indicate that the peer was notified about the clock value.
	 *
	 *  This callback is called to indicate that the authenticated peer
	 *  was notified about the accessory clock. It can be used to signal
	 *  the synchronization point between the devices. After this callback,
	 *  it may no longer be necessary to use the Fast Pair not discoverable
	 *  advertising as a mechanism to synchronize devices after a long clock
	 *  drift.
	 *
	 *  This callback is executed in the cooperative thread context. You
	 *  can learn about the exact thread context by analyzing the
	 *  CONFIG_BT_RECV_CONTEXT configuration choice. By default, this
	 *  callback is executed in the Bluetooth-specific workqueue thread
	 *  (CONFIG_BT_RECV_WORKQ_BT).
	 */
	void (*clock_synced)(void);

	/** @brief Indicate provisioning state changes.
	 *
	 *  This callback is called to indicate that the FMDN accessory has been
	 *  successfully provisioned or unprovisioned.
	 *
	 *  This callback also reports the initial provisioning state when the
	 *  user enables Fast Pair with the @ref bt_fast_pair_enable API.
	 *
	 *  The first callback is executed in the workqueue context after the
	 *  @ref bt_fast_pair_enable function call. Subsequent callbacks are
	 *  also executed in the cooperative thread context. You can learn about
	 *  the exact thread context by analyzing the CONFIG_BT_RECV_CONTEXT
	 *  configuration choice. By default, this callback is executed in the
	 *  Bluetooth-specific workqueue thread (CONFIG_BT_RECV_WORKQ_BT).
	 *
	 *  @param provisioned true if the accessory has been successfully provisioned.
	 *                     false if the accessory has been successfully unprovisioned.
	 */
	void (*provisioning_state_changed)(bool provisioned);

	/** Internally used field for list handling. */
	sys_snode_t node;
};

/** @brief Register the information callbacks in the FMDN module.
 *
 *  This function registers the information callbacks. You can call this function only
 *  in the disabled state of the FMDN module (see @ref bt_fast_pair_is_ready function).
 *  This API for callback registration is optional and does not have to be used. You can
 *  register multiple instances of information callbacks.
 *
 *  @param cb Information callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_info_cb_register(struct bt_fast_pair_fmdn_info_cb *cb);

/** Read modes. */
enum bt_fast_pair_fmdn_read_mode {
	/** EIK recovery read mode. */
	BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY,

	/** Identification read mode.
	 *  Used only when the CONFIG_BT_FAST_PAIR_FMDN_DULT is enabled.
	 */
	BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID,
};

/** Read mode callback structure. */
struct bt_fast_pair_fmdn_read_mode_cb {
	/** @brief Read mode exited.
	 *
	 *  This callback is called to indicate that the read mode has been exited.
	 *  Read mode can be entered by calling the @ref bt_fast_pair_fmdn_read_mode_enter API.
	 *
	 *  @param mode Read mode.
	 */
	void (*exited)(enum bt_fast_pair_fmdn_read_mode mode);
};

/** @brief Register the read mode callbacks in the FMDN module.
 *
 *  This function registers the read mode callbacks.
 *
 *  You can call this function only in the disabled state of the FMDN module
 *  (see @ref bt_fast_pair_is_ready function).
 *
 *  @param cb Read mode callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_read_mode_cb_register(
	const struct bt_fast_pair_fmdn_read_mode_cb *cb);

/** @brief Enter read mode.
 *
 *  This function can only be called if Fast Pair was previously enabled with the
 *  @ref bt_fast_pair_enable API.
 *
 *  @param mode Read mode.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fmdn_read_mode_enter(enum bt_fast_pair_fmdn_read_mode mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_FMDN_H_ */
