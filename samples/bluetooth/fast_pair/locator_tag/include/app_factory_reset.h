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

/** Callback type used to notify the user that the factory reset has been executed. */
typedef void (*app_factory_reset_executed_cb)(void);

/** Schedule the factory reset action.
 *
 * @param delay Time to wait before the factory reset action is performed.
 * @param cb    Callback to define the prepare action before the factory reset.
 */
void app_factory_reset_schedule(k_timeout_t delay,
				app_factory_reset_executed_cb cb);

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
