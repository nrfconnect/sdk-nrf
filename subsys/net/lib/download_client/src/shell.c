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

static int dl_callback(const struct download_client_evt *event);

static char dlc_buf[2048];
static struct download_client downloader;
static struct download_client_cfg config = {
	.callback = dl_callback,
	.buf = dlc_buf,
	.buf_size = sizeof(dlc_buf),
};

static int sec_tag_list[1];
static struct download_client_host_cfg host_config = {
	.sec_tag_list = sec_tag_list,
};

static char uri[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE +
		CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE + 1];
static bool in_progress;

static const struct shell *shell_instance;

static int dl_callback(const struct download_client_evt *event)
{
	static size_t downloaded;
	static size_t file_size;

	if (downloaded == 0) {
		//download_client_file_size_get(&downloader, &file_size);
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
		in_progress = false;
		break;
	case DOWNLOAD_CLIENT_EVT_CLOSED:
		shell_print(shell_instance, "download client closed");
		in_progress = false;
	case DOWNLOAD_CLIENT_EVT_DEINITIALIZED:
		shell_print(shell_instance, "client deinitialized");
		in_progress = false;
		break;
	}

	return 0;
}

static int cmd_dc_init(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	shell_instance = shell;

	err = download_client_init(&downloader, &config);
	if (err) {
		shell_warn(shell, "Download client init failed %d\n", err);
	}

	shell_print(shell, "Download client initialized\n");

	return err;
}

static int cmd_dc_deinit(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = download_client_deinit(&downloader);
	if (err) {
	shell_warn(shell, "Download client deinit failed %d\n", err);
	}

	return err;
}

/*static int cmd_host_config(const struct shell *shell, size_t argc, char **argv)
{
	shell_warn(shell,
		   "usage: dc host_config <pdn_id>|<sec_tag>|<native_tls>|<keep_connection>"\
		   "<range_override>\n");
	return 0;
}
*/

static int cmd_host_config_pdn_id(const struct shell *shell, size_t argc,
			     char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc host_config pdn <pdn_id>\n");
		return -EINVAL;
	}

	host_config.pdn_id = atoi(argv[1]);

	shell_print(shell, "PDN ID set: %d\n", host_config.pdn_id);
	return 0;
}

static int cmd_host_config_sec_tag(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc host_config sec_tag <sec_tag>\n");
		return -EINVAL;
	}

	sec_tag_list[0] = atoi(argv[1]);
	host_config.sec_tag_count = 1;

	shell_print(shell, "Security tag set: %d\n", host_config.sec_tag_list[0]);
	return 0;
}

static int cmd_host_config_native_tls(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc host_config native_tls <0/1>\n");
		return -EINVAL;
	}

	host_config.set_native_tls = atoi(argv[1]);

	shell_print(shell, "Native tls %s\n", host_config.set_native_tls ? "enabled" : "disabled");

	return 0;
}

static int cmd_host_config_keep_connection(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "usage: dc host_config keep_connection <0/1>\n");
		return -EINVAL;
	}

	host_config.keep_connection = atoi(argv[1]);

	shell_print(shell, "Keep connection %s\n", host_config.keep_connection ? "enabled" : "disabled");

	return 0;
}

/*
static int cmd_download(const struct shell *shell, size_t argc, char **argv)
{
	shell_warn(shell,
		   "usage: dc download <get>|<start>|<stop>|<file_size>|<progress>"\
		   "<range_override>\n");
	return 0;
}
*/

static int cmd_download_start(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	shell_instance = shell;

	if (argc != 2) {
		shell_warn(shell, "usage: dc download start <url>");
		return -ENOEXEC;
	}

	if (in_progress) {
		return -EALREADY;
	}

	strncpy(uri, argv[1], sizeof(uri));
	uri[sizeof(uri) - 1] = '\0';

	err = download_client_start(&downloader, &host_config, uri, atoi(argv[2]));
	if (err) {
		shell_warn(shell, "download_client_start() failed, err %d",
			   err);
	} else {
		in_progress = true;
		shell_print(shell, "Started download of %s from byte %d", argv[1], atoi(argv[2]));
	}
	return 0;
}

static int cmd_download_stop(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = download_client_stop(&downloader);
	if (err) {
		shell_warn(shell, "download_client_stop() failed, err %d",
			   err);
	} else {
		shell_print(shell, "Download stopped");
	}
	return 0;
}

static int cmd_download_file_size_get(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	size_t fs;

	err = download_client_file_size_get(&downloader, &fs);
	if (err) {
		shell_warn(shell, "download_client_start() failed, err %d",
			   err);
	} else {
		shell_print(shell, "File size: %d", fs);
	}
	return 0;
}

static int cmd_download_progress_get(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	size_t fs;

	err = download_client_downloaded_size_get(&downloader, &fs);
	if (err) {
		shell_warn(shell, "download_client_start() failed, err %d",
			   err);
	} else {
		shell_print(shell, "Downloaded: %d", fs);
	}
	return 0;
}


static int cmd_download_get(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	size_t from = 0;

	shell_instance = shell;

	if (argc < 2 || argc > 3) {
		shell_warn(shell, "usage: dc get <uri> [offset]");
		return -EINVAL;
	}

	if (argc == 3) {
		from = atoi(argv[2]);
	}

	if (in_progress) {
		return -EALREADY;
	}

	strncpy(uri, argv[1], sizeof(uri));
	uri[sizeof(uri) - 1] = '\0';


	err = download_client_get(&downloader, &host_config, uri, from);

	if (err) {
		shell_warn(shell, "download_client_get() failed, err %d",
			   err);
		return -ENOEXEC;
	} else {
		in_progress = true;
		shell_print(shell, "Downloading");
	}
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(host_config_options,
	SHELL_CMD(sec_tag, NULL, "Set security tag", cmd_host_config_sec_tag),
	SHELL_CMD(pdn_id, NULL, "Set PDN ID", cmd_host_config_pdn_id),
	SHELL_CMD(sec_tag, NULL, "Set security tag", cmd_host_config_sec_tag),
	SHELL_CMD(native_tls, NULL, "Enable native TLS", cmd_host_config_native_tls),
	SHELL_CMD(keep_connection, NULL, "Keep host connection", cmd_host_config_keep_connection),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(download_options,
	SHELL_CMD(get, NULL, "Get file", cmd_download_get),
	SHELL_CMD(start, NULL, "Start download", cmd_download_start),
	SHELL_CMD(stop, NULL, "Stop download", cmd_download_stop),
	SHELL_CMD(file_size, NULL, "Get file size", cmd_download_file_size_get),
	SHELL_CMD(progress, NULL, "Get bytes downloaded", cmd_download_progress_get),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dl,
	SHELL_CMD(init, NULL, "Initialize download client", cmd_dc_init),
	SHELL_CMD(deinit, NULL, "Deinitialize download client", cmd_dc_deinit),
	SHELL_CMD(host_config, &host_config_options, "Set configuration option", NULL),
	SHELL_CMD(download, &download_options, "Download options", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dl, &sub_dl, "Download client", NULL);
