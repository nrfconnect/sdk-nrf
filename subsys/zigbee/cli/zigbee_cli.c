/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <init.h>
#include <shell/shell.h>

#include <zb_nrf_platform.h>
#include "zigbee_cli.h"

/* Endpoint used by the Zigbee CLI. */
static zb_uint8_t cli_ep;

/* Zigbee CLI debug mode indicator. */
static zb_bool_t debug_mode = ZB_FALSE;

LOG_MODULE_REGISTER(cli, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

/**@brief Returns the Endpoint number used by the CLI.
 */
zb_uint8_t zb_cli_get_endpoint(void)
{
	return cli_ep;
}

/**@brief Sets the Endpoint number used by the CLI.
 */
void zb_cli_set_endpoint(zb_uint8_t ep)
{
	cli_ep = ep;
}

/**@brief Configures CLI endpoint by setting number of endpoint to be used
 * by the CLI and for chosen endpoint registers CLI endpoint handler.
 */
void zb_cli_configure_endpoint(void)
{
	zb_zcl_globals_t *zcl_ctx = zb_zcl_get_ctx();
	zb_af_endpoint_desc_t *cli_ep_desc;

	/* Verify that ZCL device ctx is present at the device as for example,
	 * network coordinator sample doesn't have one by default.
	 */
	if (zcl_ctx->device_ctx == NULL) {
		LOG_ERR("Register ZCL ctx at Zigbee device to use CLI ZCL commands.");
		return;
	}

	/* Set endpoint number to use from KConfig. */
#ifdef CONFIG_ZIGBEE_SHELL_ENDPOINT
	zb_cli_set_endpoint(CONFIG_ZIGBEE_SHELL_ENDPOINT);
#endif
	/* Verify that endpoint used by CLI is registered. */
	cli_ep_desc = zb_af_get_endpoint_desc(zb_cli_get_endpoint());
	if (!cli_ep_desc) {
		LOG_ERR("CLI endpoint: %d is not registered.",
			zb_cli_get_endpoint());
		return;
	}

	/* Check that no endpoint device handler is set for CLI endpoint
	 * and set handler used by CLI.
	 */
	if (cli_ep_desc->device_handler) {
		LOG_ERR("Can not set device handler for CLI endpoint - handler already set.");
	} else {
		ZB_AF_SET_ENDPOINT_HANDLER(zb_cli_get_endpoint(),
					   zb_cli_ep_handler);
	}
}

/**@brief Sets the debug mode.
 */
void zb_cli_debug_set(zb_bool_t debug)
{
	debug_mode = debug;
}

/**@brief Gets the debug mode.
 */
zb_bool_t zb_cli_debug_get(void)
{
	return debug_mode;
}
