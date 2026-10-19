/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Client shell: "dect <subcmd> [args...]" runs the DECT NR+ L2 shell on the server
 * over DECT NR+ RPC (DECT_RPC_CMD_SHELL). If the server's local shell is in use, the
 * server returns BUSY. "dect status" also available via dedicated DECT NR+ STATUS RPC.
 * "dect sync" is available via dedicated DECT NR+ SYNC RPC.
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include "dect_rpc_client_net.h"
#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc.h>

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#include <string.h>

static void in6_addr_to_str(const uint8_t *addr, char *buf, size_t buf_len)
{
	if (!net_addr_ntop(NET_AF_INET6, addr, buf, buf_len)) {
		buf[0] = '\0';
	}
}

/* Run DECT NR+ L2 shell command on server: dect run <subcmd> [args...]. Sync pattern (cmd_rsp). */
static int cmd_dect_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t i;
	size_t n_args;
	size_t req_size = 8;
	int err;
	uint32_t status;

	/* Subcommand handler receives argv[0]=run, argv[1]=subcmd, argv[2..]=args */
	if (argc < 2) {
		shell_error(shell, "Usage: dect run <subcmd> [args...] "
			    "(e.g. dect run status, dect run nw_join)");
		return -EINVAL;
	}
	n_args = argc - 1; /* subcmd + optional args to send to server */
	if (n_args > DECT_RPC_SHELL_MAX_ARGC) {
		shell_error(shell, "Too many args (max %d)", DECT_RPC_SHELL_MAX_ARGC);
		return -EINVAL;
	}
	for (i = 1; i < argc; i++) {
		req_size += 4 + strlen(argv[i]) + 1;
	}

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, req_size);
	nrf_rpc_encode_uint(&ctx, (uint32_t)n_args);
	for (i = 1; i < argc; i++) {
		nrf_rpc_encode_buffer(&ctx, (const uint8_t *)argv[i], strlen(argv[i]) + 1);
	}

	err = nrf_rpc_cbor_cmd_rsp(&dect_rpc_group, DECT_RPC_CMD_SHELL, &ctx);
	if (err < 0) {
		shell_error(shell, "RPC send failed: %d", err);
		return -EIO;
	}

	if (!nrf_rpc_decode_valid(&ctx)) {
		shell_error(shell, "Invalid shell response");
		return -EIO;
	}
	status = nrf_rpc_decode_uint(&ctx);
	/* Output is flushed per line via DECT NR+ RPC_CMD_SHELL_LINE; response is status only. */
	if (status != 0) {
		shell_warn(shell, "DECT NR+ Server returned status %u", (unsigned int)status);
	}

	if (!nrf_rpc_decoding_done_and_check(&dect_rpc_group, &ctx)) {
		shell_error(shell, "DECT NR+ Shell response decode error");
		return -EIO;
	}
	return 0;
}

static void print_dect_status_shell(const struct shell *shell, const struct dect_status_info *st)
{
	char addr_str[NET_INET6_ADDRSTRLEN];
	int i;

	shell_print(shell, "DECT NR+ status (from DECT NR+ RPC server):");
	shell_print(shell, "  Modem activated:              %s",
		      st->mdm_activated ? "yes" : "no");
	shell_print(shell, "  Cluster running:              %s",
		      st->cluster_running ? "yes" : "no");
	shell_print(shell, "  Cluster channel:              %u", st->cluster_channel);
	shell_print(shell, "  Network beacon running:       %s",
		      st->nw_beacon_running ? "yes" : "no");

	if (st->parent_count > 0) {
		shell_print(shell, "  Associations:");
		shell_print(shell, "    Parent long RD ID:              %u (0x%08x)",
			    st->parent_associations[0].long_rd_id,
			    st->parent_associations[0].long_rd_id);
		in6_addr_to_str((const uint8_t *)&st->parent_associations[0].local_ipv6_addr,
				addr_str, sizeof(addr_str));
		shell_print(shell, "      Local IPv6 address:           %s", addr_str);
		if (st->parent_associations[0].global_ipv6_addr_set) {
			in6_addr_to_str(
				(const uint8_t *)&st->parent_associations[0].global_ipv6_addr,
				addr_str, sizeof(addr_str));
			shell_print(shell, "      Global IPv6 address:          %s", addr_str);
		}
	}

	if (st->child_count > 0 && st->parent_count == 0) {
		shell_print(shell, "  Associations:");
	}
	for (i = 0; i < st->child_count; i++) {
		shell_print(shell, "    Child #%d long RD ID:       %u (0x%08x)", i + 1,
			    st->child_associations[i].long_rd_id,
			    st->child_associations[i].long_rd_id);
		in6_addr_to_str((const uint8_t *)&st->child_associations[i].local_ipv6_addr,
				addr_str, sizeof(addr_str));
		shell_print(shell, "      Local IPv6 address:       %s", addr_str);
		if (st->child_associations[i].global_ipv6_addr_set) {
			in6_addr_to_str(
				(const uint8_t *)&st->child_associations[i].global_ipv6_addr,
				addr_str, sizeof(addr_str));
			shell_print(shell, "      Global IPv6 address:      %s", addr_str);
		}
	}

	if (st->br_global_ipv6_addr_prefix_set) {
		in6_addr_to_str((const uint8_t *)&st->br_global_ipv6_addr_prefix, addr_str,
				sizeof(addr_str));
		shell_print(shell, "  Border router global IPv6 prefix/%u: %s",
			    (unsigned int)(st->br_global_ipv6_addr_prefix_len * 8), addr_str);
		shell_print(shell, "    Connection to Internet should be available.");
	} else {
		shell_print(shell, "  Border router global IPv6 address: not set");
	}

	if (st->fw_version_str[0] != '\0') {
		shell_print(shell, "  Modem FW version:             %s", st->fw_version_str);
	}
}

/* Request sequence so each DECT NR+ status has unique payload => unique CRC. */
static uint32_t dect_rpc_req_seq;

/* Dedicated status RPC: ref-style synchronous send+response (nrf_rpc_cbor_cmd_rsp). */
static int cmd_dect_status(const struct shell *shell, size_t argc, char **argv)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct dect_status_info status;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 8);
	dect_rpc_status_req_encode(&ctx, dect_rpc_req_seq++);

	err = nrf_rpc_cbor_cmd_rsp(&dect_rpc_group, DECT_RPC_CMD_IF_STATUS, &ctx);
	if (err < 0) {
		shell_error(shell, "RPC send failed");
		return -EIO;
	}

	if (!dect_rpc_status_rsp_decode(&ctx, &status)) {
		shell_error(shell, "Invalid status response");
		return -EIO;
	}

	print_dect_status_shell(shell, &status);

	if (!nrf_rpc_decoding_done_and_check(&dect_rpc_group, &ctx)) {
		shell_error(shell, "Status response decode error");
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_DECT_NR_RPC_CLIENT_SHELL_RPC_INIT)
static void dect_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	ARG_UNUSED(report);
	/* Logging is done by nRF RPC; avoid k_oops so shell init can return error. */
}

/*
 * Initialize nRF RPC when server is connected over UART.
 * You must run this on the DECT NR+ CLIENT first; it will block. Then run "dect_rpc_init"
 * on the server. If you run the server first, the server will time out; run this
 * on the client first and try again.
 */
static int cmd_dect_rpc_init(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static bool inited;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (inited) {
		shell_info(shell, "RPC already initialized");
		dect_rpc_client_notify_rpc_init_done();
		return 0;
	}

	shell_info(shell, "Waiting for DECT NR+ server... Run 'dect_rpc_init' on the server now.");
	err = nrf_rpc_init(dect_rpc_err_handler);
	if (err != 0) {
		shell_error(shell, "nrf_rpc_init failed: %d (is DECT NR+ server connected? Run "
			    "dect_rpc_init on server first, then retry; if retry, reboot client)",
			    err);
		return err;
	}

	/* After a previous failed init the library can return 0 without the group being bound. */
	if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
		shell_error(shell, "DECT NR+ RPC group not bound (DECT NR+ server not ready). "
			"Reboot DECT NR+ client, "
			"start DECT NR+ server and run dect_rpc_init on DECT NR+ server, "
			"then run this again.");
		return -EAGAIN;
	}

	inited = true;
	dect_rpc_client_notify_rpc_init_done();
	shell_info(shell, "DECT NR+ RPC initialized; interface will be brought up");
	return 0;
}

static int cmd_dect_rpc_ping(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (dect_rpc_client_ping(K_MSEC(2000)) != 0) {
		shell_error(shell, "DECT NR+ RPC UART test: failed (timeout)");
		return -ETIMEDOUT;
	}
	shell_info(shell, "DECT NR+ RPC UART test: OK");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dect_rpc,
	SHELL_CMD(init, NULL, "Initialize DECT NR+ RPC when DECT NR+ server is connected",
		cmd_dect_rpc_init),
	SHELL_CMD(ping, NULL, "Test DECT NR+ RPC UART both ways (ping DECT NR+ server)",
		cmd_dect_rpc_ping),
	SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_DECT_NR_RPC_CLIENT_SHELL_RPC_INIT */

#if defined(CONFIG_DECT_NR_RPC_NET_IF)
static int cmd_dect_sync(const struct shell *shell, size_t argc, char **argv)
{
	struct net_if *iface;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	if (!iface) {
		shell_error(shell, "No DECT NR+ RPC interface");
		return -ENODEV;
	}
	if (dect_rpc_client_sync_addrs(iface) != 0) {
		shell_error(shell, "Sync addresses from DECT NR+ server failed "
				   "(timeout or DECT NR+ RPC error)");
		return -EIO;
	}
	shell_info(shell, "Synced addresses/prefixes from server");
	return 0;
}
#endif /* CONFIG_DECT_NR_RPC_NET_IF */

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
static struct net_if *dect_rpc_shell_iface(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
}

static int cmd_dect_connect(const struct shell *shell, size_t argc, char **argv)
{
	struct net_if *iface;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	iface = dect_rpc_shell_iface();
	if (!iface) {
		shell_error(shell, "No DECT NR+ RPC interface");
		return -ENODEV;
	}

	ret = conn_mgr_if_connect(iface);
	if (ret < 0) {
		shell_error(shell, "dect connect failed: %d", ret);
		return ret;
	}

	shell_info(shell, "Connect initiated on server (via RPC)");
	return 0;
}

static int cmd_dect_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	struct net_if *iface;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	iface = dect_rpc_shell_iface();
	if (!iface) {
		shell_error(shell, "No DECT NR+ RPC interface");
		return -ENODEV;
	}

	ret = conn_mgr_if_disconnect(iface);
	if (ret < 0) {
		shell_error(shell, "dect disconnect failed: %d", ret);
		return ret;
	}

	shell_info(shell, "Disconnect initiated on server (via RPC)");
	return 0;
}
#endif /* CONFIG_DECT_NR_RPC_CONN_MGR */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dect,
#if defined(CONFIG_DECT_NR_RPC_CLIENT_SHELL_RPC_INIT)
	SHELL_CMD(rpc, &sub_dect_rpc, "DECT NR+ RPC commands (init when DECT NR+ server connected)",
		NULL),
#endif
	SHELL_CMD(status, NULL, "DECT status from DECT NR+ server (dedicated DECT NR+ RPC)",
		cmd_dect_status),
#if defined(CONFIG_DECT_NR_RPC_NET_IF)
	SHELL_CMD(sync, NULL, "Sync IPv6 addresses/prefixes from DECT NR+ server.",
		  cmd_dect_sync),
#endif
#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
	SHELL_CMD(connect, NULL, "Start DECT association on server (RPC)", cmd_dect_connect),
	SHELL_CMD(disconnect, NULL, "Stop DECT association on server (RPC)", cmd_dect_disconnect),
#endif
	SHELL_CMD_ARG(run, NULL, "Run DECT L2 shell on server\n"
		     "dect run <subcmd> [args...]  e.g. dect run status, dect run nw_join",
		     cmd_dect_shell, 2, DECT_RPC_SHELL_MAX_ARGC + 1),
	SHELL_SUBCMD_SET_END
);

#if defined(CONFIG_DECT_NR_RPC_CLIENT_SHELL_RPC_INIT)
#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
#define DECT_SHELL_HELP "DECT NR+ RPC client (connect | disconnect | rpc init | status | sync | run ...)"
#else
#define DECT_SHELL_HELP "DECT NR+ RPC client (rpc init | status | sync | run ...)"
#endif
#elif defined(CONFIG_DECT_NR_RPC_CONN_MGR)
#define DECT_SHELL_HELP "DECT NR+ RPC client (connect | disconnect | status | sync | run ...)"
#else
#define DECT_SHELL_HELP "DECT NR+ RPC client (status | sync | run ...)"
#endif
SHELL_CMD_REGISTER(dect, &sub_dect, DECT_SHELL_HELP, NULL);
