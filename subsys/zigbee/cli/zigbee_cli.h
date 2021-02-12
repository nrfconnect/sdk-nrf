/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_CLI_H__
#define ZIGBEE_CLI_H__

#include <stdint.h>

#include <zboss_api.h>
#include <zigbee/zigbee_app_utils.h>
#include "zigbee_cli_utils.h"
#include "zigbee_cli_ctx_mgr.h"


/**@brief Function to get the Endpoint number used by the CLI.
 *
 * @return Number of Endpoint used by the CLI.
 */
zb_uint8_t zb_cli_get_endpoint(void);

/**@brief Sets the Endpoint number used by the CLI.
 */
void zb_cli_set_endpoint(zb_uint8_t ep);

/**@brief Configures CLI endpoint by setting number of endpoint to be used
 *        by the CLI and registers CLI endpoint handler for chosen endpoint.
 */
void zb_cli_configure_endpoint(void);

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
zb_uint8_t zb_cli_ep_handler(zb_bufid_t bufid);

/**@brief Function for setting the state of the debug mode of the CLI.
 *
 * @param debug    Turns the debug mode on (ZB_TRUE) or off (ZB_FALSE).
 */
void zb_cli_debug_set(zb_bool_t debug);

/**@brief Function for getting the state of the debug mode of the CLI.
 *
 * @retval ZB_TRUE  Debug mode is turned on.
 * @retval ZB_FALSE Debug mode is turned off.
 */
zb_bool_t zb_cli_debug_get(void);

#endif /* ZIGBEE_CLI_H__ */
