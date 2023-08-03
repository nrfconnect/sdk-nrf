/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <ctype.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <app_event_manager.h>
#include <date_time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_smp_client_shell, CONFIG_NRF_MCUMGR_SMP_CLIENT_LOG_LEVEL);

#include <mcumgr_smp_client.h>
#include <zephyr/shell/shell.h>

static bool active_download;

static int hash_to_string(char *hash_string, size_t string_size, uint8_t *hash)
{
	char *ptr = hash_string;
	int buf_size = string_size;
	int len = 0;

	for (int i = 0; i < IMG_MGMT_DATA_SHA_LEN; i++) {
		len += snprintk(ptr + len, buf_size - len, "%x", hash[i]);
		if (len >= string_size) {
			return -1;
		}
	}
	hash_string[len] = 0;

	return 0;
}

static void fota_download_shell_callback(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	/* These two cases return immediately */
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		return;
	default:
		return;

	/* Following cases mark end of FOTA download */
	case FOTA_DOWNLOAD_EVT_CANCELLED:
		LOG_ERR("FOTA_DOWNLOAD_EVT_CANCELLED");
		active_download = false;
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA_DOWNLOAD_EVT_ERROR: %d", evt->cause);
		active_download = false;
		break;

	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download finished");
		active_download = false;
		break;
	}
}

static void print_image_list(const struct shell *sh, struct mcumgr_image_state *image_list)
{
	struct mcumgr_image_data *list;
	char hash_string[(IMG_MGMT_DATA_SHA_LEN * 2) + 1];

	list = image_list->image_list;
	for (int i = 0; i < image_list->image_list_length; i++) {
		if (list->flags.active) {
			shell_print(sh, "Primary Image(%d) slot(%d)", list->img_num,
				    list->slot_num);
		} else {
			shell_print(sh, "Secondary Image(%d) slot(%d)", list->img_num,
				    list->slot_num);
		}

		shell_print(sh, "       Version: %s", list->version);
		shell_print(sh, "       Bootable(%d) Pending(%d) Confirmed(%d)",
			    list->flags.bootable, list->flags.pending, list->flags.confirmed);
		if (hash_to_string(hash_string, sizeof(hash_string), list->hash) == 0) {
			shell_print(sh, "       Hash: %s", hash_string);
		}

		list++;
	}
}

static int cmd_download(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 1) {
		shell_error(sh, "No arguments provided");
		shell_help(sh);
		return -EINVAL;
	} else if (active_download) {
		shell_error(sh, "Download started already");
		return -EBUSY;
	}

	active_download = true;
	ret = mcumgr_smp_client_download_start(argv[1], -1, fota_download_shell_callback);

	if (ret < 0) {
		active_download = false;
		shell_error(sh, "Can't do write, request failed (err %d)", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_download_cancel(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = mcumgr_smp_client_download_cancel();

	if (ret < 0) {
		shell_error(sh, "Can't cancel download, request failed (err %d)", ret);
		return ret;
	}
	shell_print(sh, "Cancel OK");

	return 0;
}

static int cmd_update(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	struct mcumgr_image_state image_list;

	ret = mcumgr_smp_client_update();

	if (ret < 0) {
		shell_error(sh, "Can't perform update, request failed (err %d)", ret);
		return ret;
	}
	shell_print(sh, "Update OK");

	if (mcumgr_smp_client_read_list(&image_list) == 0) {
		print_image_list(sh, &image_list);
	}

	return 0;
}

static int cmd_erase(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = mcumgr_smp_client_erase();

	if (ret < 0) {
		shell_error(sh, "Can't perform erase, request failed (err %d)", ret);
		return ret;
	}

	shell_print(sh, "Erase OK");

	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = mcumgr_smp_client_reset();

	if (ret < 0) {
		shell_error(sh, "Can't perform reset operation, request failed (err %d)", ret);
		return -ENOEXEC;
	}

	shell_print(sh, "Reset OK");

	return 0;
}

static int cmd_read_image_list(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	struct mcumgr_image_state image_list;

	ret = dfu_target_smp_image_list_get(&image_list);

	if (ret < 0) {
		shell_error(sh, "Can't perform image list read, request failed (err %d)",
			    ret);
		return -ENOEXEC;
	}
	print_image_list(sh, &image_list);

	return 0;
}

static int cmd_confirm_image(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = mcumgr_smp_client_confirm_image();

	if (ret != 0) {
		shell_error(sh, "Can't perform image confirmation, request failed (err %d)", ret);
		return -ENOEXEC;
	}

	return 0;
}

#define MCUMGR_HELP_CMD			"MCUmgr client commands"
#define MCUMGR_HELP_DOWNLOAD		"Start download/upload image from PATH"
#define MCUMGR_HELP_DOWNLOAD_CANCEL	"Cancel download/upload process"
#define MCUMGR_HELP_ERASE		"ERASE secondary image and reset device"
#define MCUMGR_HELP_RESET		"Reset device"
#define MCUMGR_HELP_SCHEDULE		"Set Test flag to image"
#define MCUMGR_HELP_CONFIRM		"Confirm swapped image"
#define MCUMGR_HELP_READ		"Read image list"

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_mcumgr, SHELL_CMD_ARG(download, NULL, MCUMGR_HELP_DOWNLOAD, cmd_download, 1, 1),
	SHELL_CMD_ARG(download_cancel, NULL, MCUMGR_HELP_DOWNLOAD_CANCEL, cmd_download_cancel, 1,
		      0),
	SHELL_CMD_ARG(update, NULL, MCUMGR_HELP_SCHEDULE, cmd_update, 1, 0),
	SHELL_CMD_ARG(confirm, NULL, MCUMGR_HELP_CONFIRM, cmd_confirm_image, 1, 0),
	SHELL_CMD_ARG(erase, NULL, MCUMGR_HELP_ERASE, cmd_erase, 1, 0),
	SHELL_CMD_ARG(reset, NULL, MCUMGR_HELP_RESET, cmd_reset, 1, 0),
	SHELL_CMD_ARG(read, NULL, MCUMGR_HELP_READ, cmd_read_image_list, 1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_ARG_REGISTER(mcumgr, &sub_mcumgr, MCUMGR_HELP_CMD, NULL, 1, 0);
