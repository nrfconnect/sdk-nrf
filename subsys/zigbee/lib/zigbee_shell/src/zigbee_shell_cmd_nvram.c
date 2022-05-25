/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_shell.h>
#include "zigbee_shell_utils.h"


#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
static zb_bool_t nvram_enabled = ZB_TRUE;

/**@brief Enable Zigbee NVRAM
 *
 * @code
 * nvram enable
 * @endcode
 *
 */
static int cmd_zb_nvram_enable(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

	/* Keep Zigbee NVRAM after device reboot or power-off. */
	zb_set_nvram_erase_at_start(ZB_FALSE);
	nvram_enabled = ZB_TRUE;
	zb_shell_print_done(shell, ZB_FALSE);

	return 0;
}

/**@brief Disable Zigbee NVRAM
 *
 * @note  If disabled, the Zigbee NVRAM pages are used, but they will be erased
 *        during the stack startup sequence, so the device will behave as if
 *        it has stored the NVRAM data in volatile memory.
 *
 * @code
 * nvram disable
 * @endcode
 *
 */
static int cmd_zb_nvram_disable(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

	/* Erase Zigbee NVRAM after device reboot or power-off. */
	zb_set_nvram_erase_at_start(ZB_TRUE);
	nvram_enabled = ZB_FALSE;
	zb_shell_print_done(shell, ZB_FALSE);

	return 0;
}

zb_bool_t zb_shell_nvram_enabled(void)
{
	return nvram_enabled;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_nvram,
	SHELL_CMD_ARG(enable, NULL, "Enable Zigbee NVRAM.",
		      cmd_zb_nvram_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable Zigbee NVRAM.",
		      cmd_zb_nvram_disable, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nvram, &sub_nvram, "Zigbee NVRAM manipulation.",
		   NULL);
#endif
