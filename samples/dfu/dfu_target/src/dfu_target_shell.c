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

#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/dfu/mcuboot.h>

#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#include <pm_config.h>
#endif

#define STREAM_BUF_SIZE 256

#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#define DFU_TARGET_HELPER_ADDRESS PM_DFU_TARGET_HELPER_ADDRESS
#define DFU_TARGET_HELPER_SIZE PM_DFU_TARGET_HELPER_SIZE
#else
#define DFU_TARGET_HELPER_CONTAINER_ADDRESS DT_REG_ADDR(DT_GPARENT(DT_ALIAS(dfu_target_helper)))
#define DFU_TARGET_HELPER_OFFSET DT_REG_ADDR(DT_ALIAS(dfu_target_helper))
#define DFU_TARGET_HELPER_ADDRESS ((uint32_t) DFU_TARGET_HELPER_CONTAINER_ADDRESS + \
				   DFU_TARGET_HELPER_OFFSET)
#define DFU_TARGET_HELPER_SIZE DT_REG_SIZE(DT_ALIAS(dfu_target_helper))
#endif

static uint8_t stream_buf[STREAM_BUF_SIZE];

void dfu_target_callback(enum dfu_target_evt_id evt_id)
{
	printf("DFU target event received: %d\n", evt_id);
}

static int cmd_dfu_target_image_type(const struct shell *shell, size_t argc, char **argv)
{
	enum dfu_target_image_type type
		= dfu_target_img_type((const void *const) DFU_TARGET_HELPER_ADDRESS,
				      DFU_TARGET_HELPER_SIZE);

	shell_print(shell, "%d", type);
	return 0;
}

static int cmd_dfu_target_init(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int img_type;
	int size;
	int img_num;
	enum dfu_target_image_type img_type_in_buffer;

	if (argc != 3) {
		shell_error(shell, "Usage: dfu_target init <img_type> <size>");
		return -EINVAL;
	}

	img_type = (enum dfu_target_image_type) shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <img_type> argument");
		return -EINVAL;
	}
	size = shell_strtol(argv[2], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <size> argument");
		return -EINVAL;
	}
	/* Currently only img_num == 0 is supported, might be changed in the future */
	img_num = 0;

	if (size <= 0) {
		shell_error(shell, "Invalid arguments: size must be > 0");
		return -EINVAL;
	}

	img_type_in_buffer = dfu_target_img_type(
		(const void *const) DFU_TARGET_HELPER_ADDRESS,
		DFU_TARGET_HELPER_SIZE);

	if (img_type_in_buffer != img_type) {
		shell_error(shell,
			    "Image type in buffer (%d) does not match provided image type (%d)",
			    img_type_in_buffer, img_type);
		return -EINVAL;
	}

	ret = dfu_target_mcuboot_set_buf(stream_buf, STREAM_BUF_SIZE);
	if (ret < 0) {
		shell_error(shell, "Failed to set buffer for MCUboot DFU target: %d", ret);
		return ret;
	}

	shell_print(shell, "Initializing DFU target with img_type=%d, img_num=%d and size=%d",
		    img_type, img_num, size);

	ret = dfu_target_init(img_type, img_num, size, dfu_target_callback);

	if (ret < 0) {
		shell_error(shell, "DFU target initialization failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_target_write(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int offset;
	int chunk_size;

	if (argc != 3) {
		shell_error(shell, "Usage: dfu_target write <offset> <chunk_size>");
		return -EINVAL;
	}

	offset = shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <offset> argument");
		return -EINVAL;
	}
	chunk_size = shell_strtol(argv[2], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <chunk_size> argument");
		return -EINVAL;
	}

	if (offset < 0 || chunk_size <= 0) {
		shell_error(shell,
			    "Invalid arguments: offset must be >= 0 and chunk_size must be > 0");
		return -EINVAL;
	}

	ret = dfu_target_write((const void *const) (DFU_TARGET_HELPER_ADDRESS + offset),
			       chunk_size);
	if (ret < 0) {
		shell_error(shell, "DFU target write failed: %d", ret);
		return ret;
	} else {
		shell_print(shell, "Successfully wrote %d bytes at offset %d", chunk_size,
			    offset);
	}

	return 0;
}

static int cmd_dfu_target_offset_get(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	size_t offset = 0;

	ret = dfu_target_offset_get(&offset);
	if (ret < 0) {
		shell_error(shell, "DFU target offset get failed: %d", ret);
		return ret;
	}

	shell_print(shell, "%zu", offset);

	return 0;
}

static int cmd_dfu_target_done(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	bool successful = false;

	if (argc != 2) {
		shell_error(shell, "Invalid argument. Usage: dfu_target done <success|fail>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "success") == 0) {
		successful = true;
	} else if (strcmp(argv[1], "fail") == 0) {
		successful = false;
	} else {
		shell_error(shell, "Invalid argument. Usage: dfu_target done <success|fail>");
		return -EINVAL;
	}

	ret = dfu_target_done(successful);
	if (ret < 0) {
		shell_error(shell, "DFU target done failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_target_schedule_update(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ret = dfu_target_schedule_update(0);
	if (ret < 0) {
		shell_error(shell, "DFU target schedule update failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_target_reset(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ret = dfu_target_reset();
	if (ret < 0) {
		shell_error(shell, "DFU target reset failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_dfu_target_full_update(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int image_size;
	enum dfu_target_image_type image_type;

	if (argc != 2) {
		shell_error(shell, "Usage: dfu_target full_update <image_size>");
		return -EINVAL;
	}

	image_size = shell_strtol(argv[1], 10, &ret);
	if (ret != 0) {
		shell_error(shell, "Invalid <image_size> argument");
		return -EINVAL;
	}

	if (image_size <= 0) {
		shell_error(shell, "Invalid arguments: image_size must be > 0");
		return -EINVAL;
	}

	image_type = dfu_target_img_type((const void *const) DFU_TARGET_HELPER_ADDRESS,
					 DFU_TARGET_HELPER_SIZE);

	if (image_type == DFU_TARGET_IMAGE_TYPE_NONE) {
		shell_error(shell, "No valid DFU image type found in the buffer");
		return -EINVAL;
	}
	if (image_type != DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
		shell_error(shell,
			    "Currently only MCUBOOT image type is supported, the image in buffer "
			    "is of type: %d",
			    image_type);
		return -EINVAL;
	}

	ret = dfu_target_mcuboot_set_buf(stream_buf, STREAM_BUF_SIZE);
	if (ret < 0) {
		shell_error(shell, "Failed to set buffer for MCUboot DFU target: %d", ret);
		return ret;
	}

	ret = dfu_target_init(image_type, 0, image_size, dfu_target_callback);
	if (ret < 0) {
		shell_error(shell, "Failed to initialize DFU target: %d", ret);
		return ret;
	}

	ret = dfu_target_write((const void *const)DFU_TARGET_HELPER_ADDRESS, image_size);
	if (ret < 0) {
		shell_error(shell, "Failed to write image data to DFU target: %d", ret);
		return ret;
	}

	ret = dfu_target_done(true);
	if (ret < 0) {
		shell_error(shell, "Failed to finalize DFU target: %d", ret);
		return ret;
	}

	ret = dfu_target_schedule_update(0);
	if (ret < 0) {
		shell_error(shell, "Failed to schedule update: %d", ret);
		return ret;
	}

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

static int cmd_dfu_target_mcuboot_confirm(const struct shell *shell, size_t argc, char **argv)
{
	int ret = boot_write_img_confirmed();

	if (ret < 0) {
		shell_error(shell, "Failed to confirm current MCUBOOT image: %d", ret);
		return ret;
	}

	return 0;
}

/* Define the dfu_target subcommand group */
SHELL_STATIC_SUBCMD_SET_CREATE(dfu_target_cmds,
	SHELL_CMD(image_type, NULL, "Find the image type for the buffer",
		  cmd_dfu_target_image_type),
	SHELL_CMD(init, NULL, "Initialize DFU target with <img_type> and <size>",
		  cmd_dfu_target_init),
	SHELL_CMD(write, NULL,
		  "Write a chunk of data at a given offset inside the buffer in NVM "
		  "into dfu_target",
		  cmd_dfu_target_write),
	SHELL_CMD(offset_get, NULL, "Retrieve the current offset in the image stored in dfu_target",
		  cmd_dfu_target_offset_get),
	SHELL_CMD(done, NULL, "Done writing data", cmd_dfu_target_done),
	SHELL_CMD(schedule_update, NULL, "Schedule update of one or more images.",
		  cmd_dfu_target_schedule_update),
	SHELL_CMD(reset, NULL, "Reset the dfu_target state", cmd_dfu_target_reset),
	SHELL_CMD(full_update, NULL,
			  "Perform all the steps to fully update the firmware from the NVM buffer",
			  cmd_dfu_target_full_update),
	SHELL_CMD(mcuboot_confirm, NULL, "Confirm the current MCUBOOT image",
		  cmd_dfu_target_mcuboot_confirm),
	SHELL_SUBCMD_SET_END
);

/* Register the dfu_target command group */
SHELL_CMD_REGISTER(dfu_target, &dfu_target_cmds, "DFU target commands", NULL);
