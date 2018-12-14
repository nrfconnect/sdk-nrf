/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef UI_OUT_H_
#define UI_OUT_H_

#include <misc/util.h>
#include <assert.h>
#include <dk_buttons_and_leds.h>

/**
 * @file
 * @defgroup  ui_out_lib User Interface output library for Nordic DKs
 * @{
 * @brief API for the User Interface output library.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define UI_OUT_LED_ASSIGN(_led) \
	BIT((_led) - 1)

#define UI_OUT_LED_ADV UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_ADV_STARTED_LED)
#define UI_OUT_LED_CONNECTED UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_CONNECTED_LED)
#define UI_OUT_LED_BONDING UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_BONDING_LED)
#define UI_OUT_LED_SCANNING UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_SCANNING_LED)

#define UI_OUT_LED_USER_LED1 UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_USER_LED1)
#define UI_OUT_LED_USER_LED2 UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_USER_LED2)
#define UI_OUT_LED_USER_LED3 UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_USER_LED3)
#define UI_OUT_LED_USER_LED4 UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_USER_LED4)

#define UI_OUT_LED_ALERT UI_OUT_LED_ASSIGN(CONFIG_UI_OUT_ALERT_LED)

/**@brief User state leds
 *
 *        Parameters of user state leds.
 */
struct ui_out_led {
	/** Leds mask. */
	u32_t led_mask;
	/** Leds on interval. */
	u32_t on_interval;
	/** Leds off interval. */
	u32_t off_interval;
	/** Leds blinky. */
	bool blinky;
};

/**@brief User state definition.
 *
 *        Parameters of user state event.
 */
struct ui_out_user_state {
	/** User state id. */
	u32_t id;
	/** User state name. */
	const char *name;
	/** User state log message. */
	const char *message;
	/** User state leds parameters. */
	const struct ui_out_led *led;
};

/**@brief Helper to define user state id.
 *
 * @param[in] _id User state id.
 */
#define UI_OUT_STATE_USER_STATE_ID(_id) (UI_OUT_STATE_USER_STATE_START + (_id))

/**@brief Helper to declare user led, it can be used to pass user
 *        led data to @ref UI_OUT_USER_STATE_DEFINE.
 *
 * @param[in] _led_mask Bitmask that will assign leds to event.
 * @param[in] _blinky If true, the diode will flash with the specified
 *                    interval, otherwise leds will be only turned on.
 * @param[in] _on_interval The time in milliseconds through which
 *                         the LEDs will be on.
 * @param[in] _off_interval The time in milliseconds through which
 *                          the LEDs will be off.
 */
#define UI_OUT_USER_LED_DEFINE(_led_mask, _blinky, _on_interval, _off_interval) \
	(&(struct ui_out_led) {                                                 \
		.led_mask = _led_mask,                                          \
		.blinky = _blinky,                                              \
		.on_interval = _on_interval,                                    \
		.off_interval = _off_interval,                                  \
	})

/**@brief Declare user state, which can be send to user interface library.
 *
 *        User events are fully configurable. The user's states are
 *        fully configurable,  which means that you can freely define
 *        the diodes and their behavior as well as the information.
 *
 * @warning State id has to be greater than @ref UI_OUT_STATE_USER_STATE_START.
 *
 * @param[in] _name User state event name.
 * @param[in] _id User state id.
 * @param[in] _led User leds parameters @ref UI_OUT_USER_LED_DEFINE.
 * @param[in] _log_message Log message. If NULL, the log will not be displayed.
 */
#define UI_OUT_USER_STATE_DEFINE(_name, _id, _led, _log_message)          \
	const struct ui_out_user_state CONCAT(ui_user_state_, _id) __used \
	__attribute__((__section__("ui_out_user"))) = {                   \
		.id = UI_OUT_STATE_USER_STATE_ID(_id),                    \
		.name = _name,                                            \
		.message = _log_message,                                  \
		.led = _led,                                              \
	}


/**@brief Common event for user interface output library
 */
enum {
	UI_OUT_STATE_IDLE,
	UI_OUT_STATE_BLE_ADVERTISING,
	UI_OUT_STATE_BLE_SCANNING,
	UI_OUT_STATE_BLE_ADVERTISING_AND_SCANNING,
	UI_OUT_STATE_BLE_CONNECTED,
	UI_OUT_STATE_BLE_BONDING,
	UI_OUT_STATE_FATAL_ERROR,
	UI_OUT_STATE_ALERT_0,
	UI_OUT_STATE_ALERT_1,
	UI_OUT_STATE_ALERT_2,
	UI_OUT_STATE_ALERT_3,
	UI_OUT_STATE_ALERT_OFF,
	UI_OUT_STATE_USER_STATE_START,
};

/**@brief Initialization of user interface output library
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ui_out_init(void);

/**@brief Send state which will be indicated to user interface output library,
 *        states are indicated by output defined in library configuration.
 *
 *        This also can indicate user created state.
 *
 * @param[in] state_id State that will be indicated. To indicate
 *                     user state use @ref UI_OUT_STATE_USER_STATE_ID.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void ui_out_state_indicate(u32_t state_id);

/**@brief Get state id by its name.
 *
 * @param[in] name User defined state name.
 * @param[out] id State id.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ui_out_user_get_state_id(const char *name, u32_t *id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*UI_OUT_H_ */
