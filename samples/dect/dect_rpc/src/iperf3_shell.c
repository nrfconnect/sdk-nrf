/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <zephyr/posix/sys/select.h>
#include <iperf_api.h>

static int cmd_iperf3(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);

	(void)iperf_main(argc, argv, NULL, 0, NULL);
	return 0;
}

SHELL_CMD_REGISTER(iperf3, NULL, "iPerf3 (try: iperf3 --manual)", cmd_iperf3);
