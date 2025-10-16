/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#if CONFIG_OTDOA_SHELL_COMMANDS
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>

#include "otdoa_al/otdoa_api.h"
#include "otdoa_al/otdoa_nordic_at.h"
#include "nrf_modem_at.h"
#include "otdoa_http.h"

/**
 *
 * otdoa_shell_info_handler() - Handler for "info" command
 *
 * Displays the following:
 *  - ECGI of camped on cell
 *  - CTRL FSM state
 *  - NordicAT FSM state
 *  - Software version
 *  - ICCID
 */
static int otdoa_shell_info_handler(const struct shell *shell, size_t argc, char **argv)
{
	const char *version;
	char iccid[256];
	int rc;
	otdoa_xmonitor_params_t params;

	shell_print(shell, "Nordic OTDOA Application");
	/* get the ECGI */
	rc = otdoa_nordic_at_get_xmonitor(&params);
	if (rc != OTDOA_API_SUCCESS
		&& rc != OTDOA_EVENT_FAIL_NO_DLEARFCN && rc != OTDOA_EVENT_FAIL_NO_PCI) {
		shell_error(shell, "Failed to get ECGI: %d", rc);
	} else {
		shell_print(shell, "          ECGI: %"PRIu32, params.ecgi);
		shell_print(shell, "           MCC: %"PRIu16, params.mcc);
		shell_print(shell, "           MNC: %"PRIu16, params.mnc);
		if (params.dlearfcn == UNKNOWN_UBSA_DLEARFCN) {
			shell_print(shell, "      DLEARFCN: N/A");
		} else {
			shell_print(shell, "      DLEARFCN: %"PRIu32, params.dlearfcn);
		}
		if (params.pci == UNKNOWN_UBSA_PCI) {
			shell_print(shell, "           PCI: N/A");
		} else {
			shell_print(shell, "           PCI: %"PRIu16, params.pci);
		}
	}

	/* get the software version */
	version = otdoa_api_get_version();
	shell_print(shell, "       Version: %s", version);
#ifdef APP_MODEM_TRACE_ENABLED
	shell_print(shell, "         Trace: ENABLED");
#else
	shell_print(shell, "         Trace: DISABLED");
#endif

	rc = otdoa_nordic_at_get_modem_version(iccid, sizeof(iccid));
	if (rc) {
		shell_error(shell, "Failed to get Modem Version");
	} else {
		shell_print(shell, "   MFW Version: %s", iccid);
	}

	const char *pszIMEI = otdoa_get_imei_string();

	if (NULL == pszIMEI) {
		pszIMEI = "unknown";
	}
	shell_print(shell, "          IMEI: %s", pszIMEI);

	/* get the ICCID */
	rc = nrf_modem_at_cmd(iccid, sizeof(iccid), "AT%%XICCID");
	if (rc) {
		shell_error(shell, "Failed to get ICCID: %d", rc);
	} else {
		/* find the end of the ICCID and end the string there */
		*strchr(iccid, '\n') = 0;
		shell_print(shell, "       %s", iccid);
	}

	return 0;
}

/*
 * otdoa_shell_get_config_handler() - Handler for get_config command
 *
 * Takes no parameters
 */
static int otdoa_shell_get_config_handler(const struct shell *shell, size_t argc, char **argv)
{

	shell_print(shell, "Getting config file\n");

	int iRet = otdoa_api_cfg_download();

	if (iRet != 0) {
		shell_error(shell, "Failure in gettinig config file");
	}

	return 0;
}

extern int otdoa_http_send_test_jwt(void);
static void otdoa_shell_jwt_handler(const struct shell *shell, size_t argc, char **argv)
{
	otdoa_http_send_test_jwt();
}

/* Override the Serving Cell & DLEARFCN */
extern uint32_t u32OverrideServCellECGI;
extern uint32_t u32OverrideDLEARFCN;
static int otdoa_shell_override_handler(const struct shell *shell, size_t argc, char **argv)
{
	otdoa_xmonitor_params_t params;

	otdoa_nordic_at_get_xmonitor(&params);
	shell_print(shell, "Current : INFO ECGI=%u, DLEARFCN=%u\n", params.ecgi, params.dlearfcn);

	uint32_t u32ServCellECGI = 0; /* Zero means don't override */

	if (argc >= 2) {
		u32ServCellECGI = strtoul(argv[1], NULL, 0);
		u32OverrideServCellECGI = u32ServCellECGI;
		if (u32ServCellECGI) {
			u32OverrideDLEARFCN = params.dlearfcn;
		}
	}
	if (argc > 2) {
		u32OverrideDLEARFCN = strtoul(argv[2], NULL, 0);
	}

	shell_print(shell, "Serving Cell Override ECGI=%u DLEARFCN=%u\n", u32OverrideServCellECGI,
		    u32OverrideDLEARFCN);
	return 0;
}

/*
 * otdoa_shell_get_ubsa_handler() - Handler for get_ubsa command.
 *
 *  This command has the following (optional) string parameters:
 *  1. Serving cell ECGI
 *  2. Radius
 *  3. NumCells
 */
static int otdoa_shell_get_ubsa_handler(const struct shell *shell, size_t argc, char **argv)
{
	otdoa_xmonitor_params_t params;

	/* use the real values as defaults */
	otdoa_nordic_at_get_xmonitor(&params);

	/* Default to 100000 since v0.2 of server interprets this as meters */
	uint32_t u32Radius = 100000;
	uint32_t u32NumCells = 1000;

	if (u32OverrideDLEARFCN > 0) {
		params.dlearfcn = u32OverrideDLEARFCN;
	}

	if (argc >= 2) {
		params.ecgi = strtoul(argv[1], NULL, 0);
	}

	if (argc >= 3) {
		u32Radius = strtoul(argv[2], NULL, 0);
		if (u32Radius == 0) {
			shell_error(shell, "Failed to convert radius (%s)\n", argv[2]);
		}
	}

	if (argc >= 4) {
		u32NumCells = strtoul(argv[3], NULL, 0);
		if (u32NumCells == 0) {
			shell_error(shell, "Failed to convert number of cells (%s)\n", argv[3]);
		}
	}

	if (argc >= 5) {
		params.mcc = strtoul(argv[4], NULL, 0);
		if (params.mcc == 0) {
			shell_error(shell, "Failed to convert MCC (%s)\n", argv[4]);
		}
	}

	if (argc >= 6) {
		params.mnc = strtoul(argv[5], NULL, 0);
		if (params.mnc == 0) {
			shell_error(shell, "Failed to convert MNC (%s)\n", argv[5]);
		}
	}

	shell_print(shell,
		    "Getting uBSA (ECGI: %u (0x%08x) DLEARFCN: %u  Radius: %u  Num Cells: %u)\n",
		    params.ecgi, params.ecgi, params.dlearfcn, u32Radius, u32NumCells);

	otdoa_api_ubsa_dl_req_t dl_req = {0};

	dl_req.ecgi = params.ecgi;
	dl_req.dlearfcn = params.dlearfcn;
	dl_req.ubsa_radius_meters = u32Radius;
	dl_req.max_cells = u32NumCells;
	dl_req.mcc = params.mcc;
	dl_req.mnc = params.mnc;
	dl_req.pci = params.pci;
	/* Note: we reset the cell blacklist when user requests a uBSA DL */
	int err = otdoa_api_ubsa_download(&dl_req, true);

	if (err != OTDOA_API_SUCCESS) {
		shell_error(shell, "uBSA download failed with return %d\n", err);
	}
	return 0;
}

static void otdoa_shell_provision_handler(const struct shell *shell, size_t argc, char **argv)
{
	if (argc >= 2) {
		otdoa_api_provision(argv[1]);
	} else {
		shell_error(shell, "Key not provided.");
	}
}

/*
 * otdoa_shell_reset_handler() - Soft reset the device
 */
static int otdoa_shell_reset_handler(const struct shell *shell, size_t argc, char **argv)
{

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	NVIC_SystemReset();
	return 0;
}

/*
 * otdoa_shell_loc_handler() - Handler for "loc" command.
 *
 * This command has the following string parameters:
 *  1. Num PRS occasions
 *  2. Capture Flags
 */
static int otdoa_shell_loc_handler(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t u32Length = 32;
	uint32_t u32CaptureFlags = 0;

	/* NB: The first arg is the "test" subcommand
	 * so argv[0] = "test", argv[1] = report interval, ...
	 */
	if (argc < 1) {
		return -1;
	}

	if (argc >= 2) {
		u32Length = strtoul(argv[1], NULL, 0);
	}
	if (argc >= 3) {
		u32CaptureFlags = strtoul(argv[2], NULL, 0);
	}

	uint32_t timeout_msec = CONFIG_OTDOA_DEFAULT_TIMEOUT_MS;
	otdoa_api_session_params_t params = {0};

	params.session_length = u32Length;
	params.capture_flags = u32CaptureFlags;
	params.timeout = timeout_msec;

	int err = otdoa_api_start_session(&params);

	if (err != OTDOA_API_SUCCESS) {
		shell_error(shell, "otdoa_api_start_session() failed with return %d\n", err);
	}

	return 0;
}

SHELL_SUBCMD_SET_CREATE(otdoa_cmds, (phywi));

SHELL_SUBCMD_ADD((phywi), info, &otdoa_cmds, " Show current OTDOA info", otdoa_shell_info_handler,
		 0, 0);
SHELL_SUBCMD_ADD((phywi), get_config, &otdoa_cmds, " Download a config file",
		 otdoa_shell_get_config_handler, 0, 0);
SHELL_SUBCMD_ADD((phywi), jwt, &otdoa_cmds, " Test generate JWT token", otdoa_shell_jwt_handler, 0,
		 0);
SHELL_SUBCMD_ADD((phywi), get_ubsa, &otdoa_cmds,
		 " Download a uBSA (ECGI, Radius, NumCells, MCC, MNC)",
		 otdoa_shell_get_ubsa_handler, 0, 6);
SHELL_SUBCMD_ADD((phywi), loc, &otdoa_cmds, " Perform a location estimate (length,flags)",
		 otdoa_shell_loc_handler, 0, 2);
SHELL_SUBCMD_ADD((phywi), provision, &otdoa_cmds, " Provision a key to use for JWT generation",
		 otdoa_shell_provision_handler, 0, 1);
SHELL_SUBCMD_ADD((phywi), reset, &otdoa_cmds, " Soft reset the device", otdoa_shell_reset_handler,
		 0, 0);
SHELL_SUBCMD_ADD((phywi), ecgi, &otdoa_cmds,
		 " Override the serving cell ECGI - 0 to reset, empty to display",
		 otdoa_shell_override_handler, 0, 2);

SHELL_CMD_REGISTER(phywi, &otdoa_cmds, "PHY Wireless OTDOA Commands", NULL);

#endif
