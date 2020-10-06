/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <shell/shell.h>
#include <net/download_client.h>

LOG_MODULE_DECLARE(download_client);

static char apn[32];
static char host[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE];
static char file[CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE];

static struct download_client downloader;
static struct download_client_cfg config = {
	.apn = apn,
	.sec_tag = -1,
};

static const struct shell *shell_instance;

static int callback(const struct download_client_evt *event)
{
	static size_t downloaded;
	static size_t file_size;

	if (downloaded == 0) {
		download_client_file_size_get(&downloader, &file_size);
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		downloaded += event->fragment.len;
		if (file_size) {
			shell_fprintf(shell_instance, SHELL_NORMAL,
				      "\r[ %d%% ] ",
				      (downloaded * 100) / file_size);
		} else {
			shell_fprintf(shell_instance, SHELL_NORMAL,
				      "\r[ %d bytes ] ", downloaded);
		}
		break;
	case DOWNLOAD_CLIENT_EVT_DONE:
		shell_print(shell_instance, "done (%d bytes)", downloaded);
		downloaded = 0;
		break;
	case DOWNLOAD_CLIENT_EVT_ERROR:
		shell_error(shell_instance, "error %d during download",
			    event->error);
		downloaded = 0;
		break;
	}
	return 0;
}

static int download_shell_init(const struct device *d)
{
	return download_client_init(&downloader, callback);
}

static int cmd_dc_config(const struct shell *shell, size_t argc, char **argv)
{
	shell_warn(shell, "usage: dc config <apn>|<sec_tag>\n");
	return 0;
}

static int cmd_dc_config_apn(const struct shell *shell, size_t argc,
			     char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc config apn <apn_name>\n");
		return 0;
	}

	memcpy(apn, argv[1], MIN(strlen(argv[1]) + 1, sizeof(apn)));

	shell_print(shell, "APN set: %s\n", apn);
	return 0;
}

static int cmd_dc_config_sec_tag(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc config sec_tag <sec_tag>\n");
		return 0;
	}

	config.sec_tag = atoi(argv[1]);

	shell_print(shell, "Security tag set: %d\n", config.sec_tag);
	return 0;
}

static int cmd_dc_connect(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	shell_instance = shell;

	if (argc != 2) {
		shell_warn(shell, "usage: dc connect <host>|<url>");
		return 0;
	}

	memcpy(host, argv[1], MIN(strlen(argv[1]) + 1, sizeof(host)));

	err = download_client_connect(&downloader, host, &config);
	if (err) {
		shell_warn(shell, "download_client_connect() failed, err %d",
			   err);
	} else {
		shell_print(shell, "Connected");
	}

	return 0;
}

static int cmd_dc_pause(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "usage: dc pause");
		return 0;
	}

	download_client_pause(&downloader);
	shell_print(shell, "Paused");

	return 0;
}

static int cmd_dc_resume(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "usage: dc resume");
		return 0;
	}

	download_client_resume(&downloader);
	shell_print(shell, "Resuming");

	return 0;
}

static int cmd_dc_download(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc != 2) {
		shell_warn(shell, "usage: dc download <url>");
		return 0;
	}

	memcpy(file, argv[1], MIN(strlen(argv[1]) + 1, sizeof(file)));

	err = download_client_start(&downloader, file, 0);
	if (err) {
		shell_warn(shell, "download_client_start() failed, err %d",
			   err);
	} else {
		shell_print(shell, "Downloading");
	}
	return 0;
}

static int cmd_dc_disconnect(const struct shell *shell, size_t argc,
			     char **argv)
{
	return download_client_disconnect(&downloader);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config,
	SHELL_CMD(apn, NULL, "Set APN", cmd_dc_config_apn),
	SHELL_CMD(sec_tag, NULL, "Set security tag", cmd_dc_config_sec_tag),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dc,
	SHELL_CMD(config, &sub_config, "Configure", cmd_dc_config),
	SHELL_CMD(connect, NULL, "Connect to a host", cmd_dc_connect),
	SHELL_CMD(disconnect, NULL, "Disconnect from a host",
		  cmd_dc_disconnect),
	SHELL_CMD(download, NULL, "Download a file", cmd_dc_download),
	SHELL_CMD(pause, NULL, "Pause download", cmd_dc_pause),
	SHELL_CMD(resume, NULL, "Resume download", cmd_dc_resume),
	SHELL_SUBCMD_SET_END
);

SYS_INIT(download_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

SHELL_CMD_REGISTER(dc, &sub_dc, "Download client", NULL);
