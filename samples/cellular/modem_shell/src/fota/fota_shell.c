/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "fota.h"
#include "mosh_print.h"

static const char fota_server_eu[] = "nrf-test-eu.s3.amazonaws.com";
static const char fota_server_usa[] = "nrf-test-us.s3.amazonaws.com";
static const char fota_server_jpn[] = "nrf-test-jpn.s3.amazonaws.com";
static const char fota_server_au[] = "nrf-test-au.s3.amazonaws.com";

static int cmd_fota_download(const struct shell *shell, size_t argc,
			     char **argv)
{
	int err;
	const char *fota_server;

	if (strcmp(argv[1], "eu") == 0) {
		fota_server = fota_server_eu;
	} else if (strcmp(argv[1], "us") == 0) {
		fota_server = fota_server_usa;
	} else if (strcmp(argv[1], "jpn") == 0) {
		fota_server = fota_server_jpn;
	} else if (strcmp(argv[1], "au") == 0) {
		fota_server = fota_server_au;
	} else {
		mosh_error("FOTA: Unknown server: %s", argv[1]);
		return -EINVAL;
	}

	mosh_print("FOTA: Starting download...");

	err = fota_start(fota_server, argv[2]);

	if (err) {
		mosh_error("Failed to start FOTA download, error %d",
			    err);
		return err;
	}

	mosh_print("FOTA: Download started");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_fota,
	SHELL_CMD_ARG(
		download, NULL,
		"<server> <filename>\nDownload and install a FOTA update. Available servers are \"eu\", \"us\", \"jpn\" and \"au\".",
		cmd_fota_download, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fota, &sub_fota, "Commands for FOTA update.", mosh_print_help_shell);
