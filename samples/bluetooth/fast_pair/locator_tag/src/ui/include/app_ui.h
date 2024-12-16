/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_UI_H_
#define APP_UI_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_ui Locator Tag sample UI module
 * @brief Locator Tag sample UI module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Application states passed to the UI module.
 *
 *  These states are independent of each other and the application can be
 *  in several of them at the same time.
 */
enum app_ui_state {
	APP_UI_STATE_APP_RUNNING,
	APP_UI_STATE_RINGING,
	APP_UI_STATE_RECOVERY_MODE,
	APP_UI_STATE_ID_MODE,
	APP_UI_STATE_PROVISIONED,
	APP_UI_STATE_FP_ADV,
	APP_UI_STATE_DFU_MODE,
	APP_UI_STATE_MOTION_DETECTOR_ACTIVE,

	APP_UI_STATE_COUNT,
};

/** Indicate the current application state to the UI module.
 *
 * The UI module reacts to the current application state by properly driving the
 * dependent peripherals, such as LEDs or speakers.
 * This function should be implemented depending on the chosen platform and peripherals.
 *
 * @param state Current application state to be handled by the UI module
 * @param active Marks if the application enters ( @ref true ) or leaves ( @ref false )
 *               the current state.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_ui_state_change_indicate(enum app_ui_state state, bool active);

/** @brief Available UI module requests to be handled by the application. */
enum app_ui_request {
	APP_UI_REQUEST_RINGING_STOP,
	APP_UI_REQUEST_RECOVERY_MODE_ENTER,
	APP_UI_REQUEST_ID_MODE_ENTER,
	APP_UI_REQUEST_SIMULATED_BATTERY_CHANGE,
	APP_UI_REQUEST_FP_ADV_PAIRING_MODE_CHANGE,
	APP_UI_REQUEST_FACTORY_RESET,
	APP_UI_REQUEST_DFU_MODE_ENTER,
	APP_UI_REQUEST_MOTION_INDICATE,

	APP_UI_REQUEST_COUNT,
};

/** @brief Listener for the UI module requests. */
struct app_ui_request_listener {
	/** UI module request handler.
	 *
	 * @param request UI module generated request to be handled by the application.
	 */
	void (*handler)(enum app_ui_request request);
};

/** Register a listener for the UI module requests.
 *
 * @param _name UI module request listener name.
 * @param _handler UI module request handler.
 */
#define APP_UI_REQUEST_LISTENER_REGISTER(_name, _handler)				\
	BUILD_ASSERT(_handler != NULL);							\
	static const STRUCT_SECTION_ITERABLE(app_ui_request_listener, _name) = {	\
		.handler = _handler,							\
	}

/** Initialize the UI module.
 *
 * This function should be implemented depending on the chosen platform and peripherals.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_ui_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_UI_H_ */
