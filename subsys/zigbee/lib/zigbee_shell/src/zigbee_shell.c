/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>

#include <zb_nrf_platform.h>
#include <zigbee/zigbee_shell.h>


/* Endpoint used by the Zigbee shell. */
static zb_uint8_t shell_ep;

/* Zigbee shell debug mode indicator. */
static zb_bool_t debug_mode = ZB_FALSE;

LOG_MODULE_REGISTER(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

/**@brief Returns the Endpoint number used by the shell.
 */
zb_uint8_t zb_shell_get_endpoint(void)
{
	return shell_ep;
}

/**@brief Sets the Endpoint number used by the shell.
 */
void zb_shell_set_endpoint(zb_uint8_t ep)
{
	shell_ep = ep;
}

/**@brief Configures shell endpoint by setting number of endpoint to be used
 * by the shell and for chosen endpoint registers shell endpoint handler.
 */
void zb_shell_configure_endpoint(void)
{
	zb_zcl_globals_t *zcl_ctx = zb_zcl_get_ctx();
	zb_af_endpoint_desc_t *shell_ep_desc;

	/* Verify that ZCL device ctx is present at the device as for example,
	 * network coordinator sample doesn't have one by default.
	 */
	if (zcl_ctx->device_ctx == NULL) {
		LOG_ERR("Register ZCL ctx at Zigbee device to use Zigbee shell ZCL commands.");
		return;
	}

	/* Set endpoint number to use from KConfig. */
#ifdef CONFIG_ZIGBEE_SHELL_ENDPOINT
	zb_shell_set_endpoint(CONFIG_ZIGBEE_SHELL_ENDPOINT);
#endif
	/* Verify that endpoint used by shell is registered. */
	shell_ep_desc = zb_af_get_endpoint_desc(zb_shell_get_endpoint());
	if (!shell_ep_desc) {
		LOG_ERR("Zigbee shell endpoint: %d is not registered.",
			zb_shell_get_endpoint());
		return;
	}

	/* Check that no endpoint device handler is set for shell endpoint
	 * and set handler used by shell.
	 */
	if (shell_ep_desc->device_handler) {
		LOG_ERR("Can not set device handler for shell endpoint - handler already set.");
	} else {
		ZB_AF_SET_ENDPOINT_HANDLER(zb_shell_get_endpoint(),
					   zb_shell_ep_handler);
	}
}

/**@brief Sets the debug mode.
 */
void zb_shell_debug_set(zb_bool_t debug)
{
	debug_mode = debug;
}

/**@brief Gets the debug mode.
 */
zb_bool_t zb_shell_debug_get(void)
{
	return debug_mode;
}
