/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_SHELL_H__
#define ZIGBEE_SHELL_H__

#include <zboss_api.h>


/** @file zigbee_shell.h
 *
 * @defgroup zigbee_shell Zigbee shell library.
 * @{
 * @brief  Library for enabling and configuring Zigbee shell library.
 *
 * @details Provides a set of functions for enabling and configuring Zigbee shell library
 * and zigbee shell endpoint handler function.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function to get the Endpoint number used by the shell.
 *
 * @return Number of Endpoint used by the shell.
 */
zb_uint8_t zb_shell_get_endpoint(void);

/**@brief Sets the Endpoint number used by the shell.
 */
void zb_shell_set_endpoint(zb_uint8_t ep);

/**@brief Configures shell endpoint by setting number of endpoint to be used
 *        by the shell and registers shell endpoint handler for chosen endpoint.
 */
void zb_shell_configure_endpoint(void);

/**@brief Function for intercepting every frame coming to the endpoint,
 *        so the frame may be processed before it is processed by the Zigbee
 *        stack.
 *
 * @param bufid     Reference to the ZBOSS buffer.
 *
 * @retval ZB_TRUE  Frame has already been processed and Zigbee stack can skip
 *                  processing this frame.
 * @retval ZB_FALSE Zigbee stack must process this frame.
 */
zb_uint8_t zb_shell_ep_handler(zb_bufid_t bufid);

/**@brief Function for setting the state of the debug mode of the shell.
 *
 * @param debug    Turns the debug mode on (ZB_TRUE) or off (ZB_FALSE).
 */
void zb_shell_debug_set(zb_bool_t debug);

/**@brief Function for getting the state of the debug mode of the shell.
 *
 * @retval ZB_TRUE  Debug mode is turned on.
 * @retval ZB_FALSE Debug mode is turned off.
 */
zb_bool_t zb_shell_debug_get(void);

#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
/**@brief Function for getting the state of the Zigbee NVRAM.
 *
 * @retval ZB_TRUE  Zigbee NVRAM is enabled.
 * @retval ZB_FALSE Zigbee NVRAM is disabled.
 */
zb_bool_t zb_shell_nvram_enabled(void);
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */

#ifdef __cplusplus
}
#endif

/**@} */

#endif /* ZIGBEE_SHELL_H__ */
