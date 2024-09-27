/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <net/download_client.h>

LOG_MODULE_DECLARE(download_client);

static char host[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE];
static char file[CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE];

static int callback(const struct download_client_evt *event);

static char dlc_buf[2048];
static struct download_client downloader;
static struct download_client_cfg config = {
	.callback = callback,
	.buf = dlc_buf,
	.buf_size = sizeof(dlc_buf),
};

static int sec_tag_list[1];
static struct download_client_host_cfg host_config = {
	.sec_tag_list = sec_tag_list,
	.hostname =  host,
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
	case DOWNLOAD_CLIENT_EVT_CLOSED:
		shell_print(shell_instance, "socket closed");
		break;
	}
	return 0;
}

static int cmd_dc_init(const struct shell *shell, size_t argc, char **argv)
{
	return download_client_init(&downloader, &config);
}

static int cmd_dc_deinit(const struct shell *shell, size_t argc, char **argv)
{
	return download_client_deinit(&downloader);
}

static int cmd_dc_config(const struct shell *shell, size_t argc, char **argv)
{
	shell_warn(shell, "usage: dc host_config <pdn_id>|<sec_tag>\n");
	return 0;
}

static int cmd_dc_set_pdn_id(const struct shell *shell, size_t argc,
			     char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc set pdn <pdn_id>\n");
		return -EINVAL;
	}

	host_config.pdn_id = atoi(argv[1]);

	shell_print(shell, "PDN ID set: %d\n", host_config.pdn_id);
	return 0;
}

static int cmd_dc_set_sec_tag(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc set sec_tag <sec_tag>\n");
		return -EINVAL;
	}

	sec_tag_list[0] = atoi(argv[1]);
	host_config.sec_tag_count = 1;

	shell_print(shell, "Security tag set: %d\n", host_config.sec_tag_list[0]);
	return 0;
}

static int cmd_dc_set_host(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	shell_instance = shell;

	if (argc != 2) {
		shell_warn(shell, "usage: dc connect <host>|<url>");
		return -EINVAL;
	}

	memcpy(host, argv[1], MIN(strlen(argv[1]) + 1, sizeof(host)));

	return 0;
}

static int cmd_dc_download(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc != 2) {
		shell_warn(shell, "usage: dc download <url>");
		return -ENOEXEC;
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

static int cmd_dc_get(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	size_t from = 0;
	char *f = file;

	if (argc < 2) {
		shell_warn(shell, "usage: dc get <url> [<file>|NULL] [offset]");
		return -EINVAL;
	}

	shell_instance = shell;

	memcpy(host, argv[1], MIN(strlen(argv[1]) + 1, sizeof(host)));
	if (argc > 2) {
		memcpy(file, argv[2], MIN(strlen(argv[2]) + 1, sizeof(file)));
		if (strstr(file, "NULL")) {
			f = NULL;
		}
	} else {
		f = NULL;
	}

	if (argc > 3) {
		errno = 0;
		from = strtol(argv[3], NULL, 0);
		if (errno == ERANGE) {
			shell_warn(shell, "invalid offset");
			return -EINVAL;
		}
	}


	err = download_client_get(&downloader, &host_config, f, from);

	if (err) {
		shell_warn(shell, "download_client_get() failed, err %d",
			   err);
		return -ENOEXEC;
	} else {
		shell_print(shell, "Downloading");
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(set_options,
	SHELL_CMD(hostname, NULL, "Set a target host", cmd_dc_set_host),
	SHELL_CMD(sec_tag, NULL, "Set security tag", cmd_dc_set_sec_tag),
	SHELL_CMD(pdn_id, NULL, "Set PDN ID", cmd_dc_set_pdn_id),
	SHELL_CMD(range_override, NULL, "Set a target host", cmd_dc_set_range_override),
	SHELL_CMD(native_tls, NULL, "Set a target host", cmd_dc_set_tls_hostname),
	SHELL_CMD(close_when_done, NULL, "Set a target host", cmd_dc_set_tls_hostname),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(download_options,
	SHELL_CMD(start, NULL, "Start downloading file", cmd_dc_dl_start),
	SHELL_CMD(stop, NULL, "Stop download", cmd_dc_dl_stop),
	SHELL_CMD(fget, NULL, "Get file", cmd_dc_dl_get),
	SHELL_CMD(file_size, NULL, "Get file size", cmd_dc_dl_f_size),
	SHELL_CMD(downloaded_size, NULL, "Retrieve number of bytes downloaded", cmd_dc_dl_size),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dc,
	SHELL_CMD(init, NULL, "Initialize download client", cmd_dc_init),
	SHELL_CMD(deinit, NULL, "Deinitialize download client", cmd_dc_deinit),
	SHELL_CMD(set, &set_options, "Set configuration option", cmd_dc_set),
	SHELL_CMD(download, &download_options, "Download options", cmd_dc_download),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dc, &sub_dc, "Download client", NULL);
