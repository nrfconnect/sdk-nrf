/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_FACTORY_RESET_H_
#define APP_FACTORY_RESET_H_

#include <zephyr/kernel.h>

/**
 * @defgroup fmdn_sample_factory_reset Locator Tag sample factory reset module
 * @brief Locator Tag sample factory reset module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Factory reset callback descriptor. */
struct app_factory_reset_callbacks {
	/** Callback used to allow the user to prepare for the factory reset. */
	void (*prepare)(void);

	/** Callback used to notify the user that the factory reset has been executed. */
	void (*executed)(void);
};

/** Register a factory reset callback descriptor.
 *
 *  @param _name Factory reset callbacks descriptor name.
 *  @param _prepare Callback to define actions before the factory reset.
 *  @param _executed Callback to define actions after the factory reset.
 */
#define APP_FACTORY_RESET_CALLBACKS_REGISTER(_name, _prepare, _executed)		\
	static const STRUCT_SECTION_ITERABLE(app_factory_reset_callbacks, _name) = {	\
		.prepare = _prepare,							\
		.executed = _executed,							\
	}

/** Schedule the factory reset action.
 *
 * @param delay Time to wait before the factory reset action is performed.
 */
void app_factory_reset_schedule(k_timeout_t delay);

/** Cancel the scheduled factory reset action. */
void app_factory_reset_cancel(void);

/** Initialize the factory reset module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_factory_reset_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_FACTORY_RESET_H_ */
