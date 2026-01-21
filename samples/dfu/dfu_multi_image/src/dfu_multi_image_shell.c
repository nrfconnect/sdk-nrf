/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_multi_image.h>
#include <zephyr/dfu/mcuboot.h>

#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#include <pm_config.h>
#endif

#include "dfu_multi_image_sample_common.h"

#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#define DFU_MULTI_IMAGE_HELPER_ADDRESS PM_DFU_MULTI_IMAGE_HELPER_ADDRESS
#define DFU_MULTI_IMAGE_HELPER_SIZE PM_DFU_MULTI_IMAGE_HELPER_SIZE
#else
#define DFU_MULTI_IMAGE_HELPER_CONTAINER_ADDRESS \
	DT_REG_ADDR(DT_GPARENT(DT_ALIAS(dfu_multi_image_helper)))
#define DFU_MULTI_IMAGE_HELPER_OFFSET DT_REG_ADDR(DT_ALIAS(dfu_multi_image_helper))
#define DFU_MULTI_IMAGE_HELPER_ADDRESS ((uint32_t) DFU_MULTI_IMAGE_HELPER_CONTAINER_ADDRESS + \
				   DFU_MULTI_IMAGE_HELPER_OFFSET)
#define DFU_MULTI_IMAGE_HELPER_SIZE DT_REG_SIZE(DT_ALIAS(dfu_multi_image_helper))
#endif

static int cmd_dfu_multi_image_write(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int read_offset;
	int write_offset;
	int chunk_size;

	if (argc != 4) {
		shell_error(shell, "Usage: dfu_multi_image write <read_offset> <write_offset> "
				   "<chunk_size>");
		return -EINVAL;
	}

	read_offset = shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <read_offset> argument");
		return -EINVAL;
	}
	write_offset = shell_strtol(argv[2], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <write_offset> argument");
		return -EINVAL;
	}
	chunk_size = shell_strtol(argv[3], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <chunk_size> argument");
		return -EINVAL;
	}

	if (read_offset < 0 || write_offset < 0 || chunk_size <= 0) {
		shell_error(shell,
			    "Invalid arguments: offsets must be >= 0 and chunk_size must be > 0");
		return -EINVAL;
	}

	ret = dfu_multi_image_write(write_offset,
				    (const void *const) (DFU_MULTI_IMAGE_HELPER_ADDRESS
							 + read_offset),
				    chunk_size);
	if (ret < 0) {
		shell_error(shell, "DFU multi image write failed: %d", ret);
		return ret;
	} else {
		shell_print(shell, "Successfully wrote %d bytes at offset %d", chunk_size,
			    write_offset);
	}

	return 0;
}

static int cmd_dfu_multi_image_offset_get(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "%zu", dfu_multi_image_offset());

	return 0;
}

static int cmd_dfu_multi_image_done(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	bool successful = false;

	if (argc != 2) {
		shell_error(shell, "Invalid number of arguments. "
					       "Usage: dfu_multi_image done <success|fail>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "success") == 0) {
		successful = true;
	} else if (strcmp(argv[1], "fail") == 0) {
		successful = false;
	} else {
		shell_error(shell, "Invalid argument. Usage: dfu_multi_image done <success|fail>");
		return -EINVAL;
	}

	ret = dfu_multi_image_done(successful);
	if (ret < 0) {
		shell_error(shell, "DFU multi image done failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_multi_image_schedule_update(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ret = dfu_target_schedule_update(-1);
	if (ret < 0) {
		shell_error(shell, "DFU target schedule update failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_multi_image_reset(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ret = dfu_multi_image_reset();
	if (ret < 0) {
		shell_error(shell, "DFU multi image reset failed: %d", ret);
		return ret;
	}

	ret = dfu_multi_image_sample_lib_prepare();

	if (ret < 0) {
		shell_error(shell, "Failed to prepare dfu_multi_image library: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_multi_image_full_update(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int package_size;

	if (argc != 2) {
		shell_error(shell, "Usage: dfu_multi_image full_update <package_size>");
		return -EINVAL;
	}

	package_size = shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <package_size> argument");
		return -EINVAL;
	}

	if (package_size <= 0) {
		shell_error(shell, "Invalid arguments: package_size must be > 0");
		return -EINVAL;
	}

	ret = dfu_multi_image_write(0, (const void *const) (DFU_MULTI_IMAGE_HELPER_ADDRESS),
				    package_size);
	if (ret < 0) {
		shell_error(shell, "DFU multi image write failed: %d", ret);
		return ret;
	}

	ret = dfu_multi_image_done(true);
	if (ret < 0) {
		shell_error(shell, "DFU multi image done failed: %d", ret);
		return ret;
	}

	ret = dfu_target_schedule_update(-1);
	if (ret < 0) {
		shell_error(shell, "Failed to schedule update: %d", ret);
		return ret;
	}

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

#ifndef CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY
static int cmd_dfu_multi_image_mcuboot_confirm(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int img_num = -1;

	if (argc != 2) {
		shell_error(shell, "Usage: dfu_multi_image mcuboot_confirm <img_num>");
		return -EINVAL;
	}

	img_num = shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <img_num> argument");
		return -EINVAL;
	}
	ret = boot_write_img_confirmed_multi(img_num);

	if (ret < 0) {
		shell_error(shell, "Failed to confirm current MCUBOOT image: %d", ret);
		return ret;
	}

	return 0;
}
#endif

/* Define the dfu_target subcommand group */
SHELL_STATIC_SUBCMD_SET_CREATE(dfu_multi_image_cmds,
	SHELL_CMD(write, NULL,
		  "Write a chunk of data at a given offset inside the buffer in NVM "
		  "into dfu_multi_image",
		  cmd_dfu_multi_image_write),
	SHELL_CMD(offset_get, NULL, "Retrieve the current write offset of dfu_multi_image",
		  cmd_dfu_multi_image_offset_get),
	SHELL_CMD(done, NULL, "Done writing data", cmd_dfu_multi_image_done),
	SHELL_CMD(schedule_update, NULL, "Schedule update for the images in the uploaded package.",
		  cmd_dfu_multi_image_schedule_update),
	SHELL_CMD(reset, NULL, "Reset the dfu_multi_image state as well as all the underlying "
		  "dfu_target states", cmd_dfu_multi_image_reset),
	SHELL_CMD(full_update, NULL,
		  "Perform all the steps to fully update the firmware from the NVM buffer",
		  cmd_dfu_multi_image_full_update),
#ifndef CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY
	SHELL_CMD(mcuboot_confirm, NULL, "Confirm one of the current MCUBOOT images",
		  cmd_dfu_multi_image_mcuboot_confirm),
#endif
	SHELL_SUBCMD_SET_END
);

/* Register the dfu_target command group */
SHELL_CMD_REGISTER(dfu_multi_image, &dfu_multi_image_cmds, "DFU multi_image commands", NULL);
