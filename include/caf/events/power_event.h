/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _POWER_EVENT_H_
#define _POWER_EVENT_H_

/**
 * @file
 * @defgroup caf_power_event CAF Power Event
 * @{
 * @brief CAF Power Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Power down event.
 *
 * The power down event is submitted to inform the application modules that they should be suspended
 * to reduce the power consumption. The power down event can also be submitted to suspend
 * the modules after an application module reports fatal error submitting @ref module_state_event
 * with state set to MODULE_STATE_ERROR.
 *
 * An application module that handles the power down event should submit @ref module_state_event
 * with state set to MODULE_STATE_OFF or MODULE_STATE_STANDBY. The MODULE_STATE_STANDBY should
 * be submitted by a module that is capable of submitting @ref wake_up_event to wake up the system
 * on external event. Otherwise MODULE_STATE_OFF should be reported. If the module is unable to
 * suspend at the moment, it should consume the received power down events until it is ready to
 * suspend itself. The module should also delay submitting the @ref module_state_event until it is
 * ready to suspend itself.
 *
 * The module may handle the power down event differently if it is related to fatal error.
 * In general, an application module should suspend itself, but some of the modules may still need
 * to be active after the fatal error. For example, a module that controls hardware LEDs may be used
 * to display LED effect related to the fatal error.
 *
 * Only the module that controls the power management in the application can submit the power down
 * event. It is also the final subscriber for the power down event. When it receives the power down
 * event, it is ensured that all other application modules that handle the power down event are
 * already suspended. Then it can continue the power down procedure.
 *
 * @note An application module that handles power down event must also handle @ref wake_up_event.
 *       Otherwise the module will never be woken up after suspending.
 */
struct power_down_event {
	/** Event header. */
	struct app_event_header header;

	/** Information if the power down was triggered by a fatal error. */
	bool error;
};


/** @brief Wake up event.
 *
 * The wake up event is submitted to trigger exiting the suspended state.
 *
 * The wake up event can be submitted by any application module after the application is suspended.
 * The event can be submitted for example on button press or sensor trigger.
 *
 * Every application module that handles wake up event should exit suspended state, turn on
 * functionalities disabled on @ref power_down_event and submit @ref module_state_event with state
 * set to MODULE_STATE_READY. If the module is already woken up, it should ignore the event.
 * The module must not consume the wake up event to ensure that other listeners will be informed.
 *
 * The module that controls power management of the application should be an only early subscriber
 * for the wake up event. This module can consume the wake up events to prevent waking up the
 * application modules after fatal error.
 *
 * @note For more information about suspending modules see @ref power_down_event.
 */
struct wake_up_event {
	/** Event header. */
	struct app_event_header header;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

APP_EVENT_TYPE_DECLARE(power_down_event);
APP_EVENT_TYPE_DECLARE(wake_up_event);

#ifdef __cplusplus
}
#endif

#endif /* _POWER_EVENT_H_ */
