/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_UI_PRIV_H_
#define APP_UI_PRIV_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_ui Locator Tag sample UI module private
 * @brief Locator Tag sample UI module private API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Broadcast the UI module request to all registered listeners.
 *
 * This function should be called on the asynchronous events that happened,
 * for example, if there should be some action after a button press, this information
 * should be broadcasted to all the listeners as one of them might implement it.
 *
 * @param request UI module generated request to be handled by the application.
 */
void app_ui_request_broadcast(enum app_ui_request request);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_UI_PRIV_H_ */
