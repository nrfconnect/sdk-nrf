/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Server shell: "dect rpc init" initializes nRF RPC when client is connected
 * over UART. Use when CONFIG_NRF_RPC_INIT=n so server can boot without client.
 */

#if defined(CONFIG_DECT_NR_RPC_SERVER_SHELL_RPC_INIT)

#include "dect_rpc_common.h"
#include <nrf_rpc.h>
#include <nrf_rpc_errno.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dect_rpc_server_shell, CONFIG_NET_DECT_RPC_LOG_LEVEL);

static void dect_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	ARG_UNUSED(report);
}

/*
 * Server is the initiator: it sends the group-init. The client (follower) must
 * run "dect rpc init" first and be waiting; then run this. If the client was
 * not ready, this will time out after 15s; run "dect rpc init" on the client
 * first and try again.
 */
static int cmd_dect_rpc_init(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static bool inited;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (inited) {
		shell_info(shell, "RPC already initialized");
		return 0;
	}

	shell_info(shell, "Sending group init... (client must have run 'dect rpc init' first)");
	err = nrf_rpc_init(dect_rpc_err_handler);
	if (err != 0) {
		shell_error(shell, "nrf_rpc_init failed: %d (run \"dect rpc init\" on client first,"
			    " wait until it blocks, then run this again)", err);
		return err;
	}
	/* After a previous failed init the library can return 0 without the group being bound. */
	if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
		shell_error(shell, "RPC group not bound. Run \"dect rpc init\" on client first, "
			    "then run this again (reboot server if retry).");
		return -NRF_EAGAIN;
	}
	inited = true;
	shell_info(shell, "RPC initialized");
	return 0;
}

/* Use distinct command name; server already has "dect" from DECT L2 shell. */
SHELL_CMD_REGISTER(dect_rpc_init, NULL, "Initialize DECT RPC when client is connected over UART",
		   cmd_dect_rpc_init);

#endif /* CONFIG_DECT_NR_RPC_SERVER_SHELL_RPC_INIT */
