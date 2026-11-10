/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
#include <bootutil/boot_request.h>

static int cmd_boot_request_serial_recovery(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = boot_request_init();
	if (rc == 0) {
		rc = boot_request_enter_recovery();
	}

	if (rc != 0) {
		shell_error(sh, "Failed to request serial recovery: %d", rc);
		return rc;
	}

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(boot_request_cmds,
	SHELL_CMD_ARG(serial_recovery, NULL, "Enter MCUboot serial recovery on next boot",
		      cmd_boot_request_serial_recovery, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(boot_request, &boot_request_cmds, "Bootloader request commands", NULL);
