/*
 * Copyright (c) 2024 Junho Lee <tot0roprog@gmail.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include "blob_shell.h"
#include "blob_xfer.h"

static const struct shell *blob_shell;
static int targets[MAX_TARGET_NODES];
static int target_count;

static bool contained_target(uint16_t addr)
{
	int i;
	for (i = 0; i < target_count; i++) {
		if (targets[i] == addr)
			return true;
	}
	return false;
}

static int cmd_send_add_targets(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	int i;
	uint16_t addr;

	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	if (ARRAY_SIZE(targets) - target_count < argc - 1) {
		shell_error(shell, "No more room");
		return -1;
	}

	for (i = 1; i < argc; i++) {
		addr = shell_strtoul(argv[i], 16, &err);
		if (err) {
			shell_error(shell, "Unable to parse input string argument");
			return err;
		} else if (addr == 0 || addr >= 0xC000) {
			shell_error(shell, "Only unicast addresses are allowed");
			return -1;
		}

		if (contained_target(addr))
			continue;

		targets[target_count++] = addr;
		shell_print(shell, "Added target %4.4x", addr);
	}

	return 0;
}

static int cmd_send_show_targets(const struct shell *shell, size_t argc, char *argv[])
{
	int i;

	if (target_count == 0) {
		shell_print(shell, "No target");
		return 0;
	}

	for (i = 0; i < target_count; i++) {
		shell_print(shell, "%2d Target: 0x%4.4x", i + 1, targets[i]);
	}

	return 0;
}

static int cmd_send_clean_targets(const struct shell *shell, size_t argc, char *argv[])
{
	target_count = 0;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(blob_send_targerts_cmds,
	SHELL_CMD_ARG(add, NULL, "Add target addresses (Up to 32 nodes). <addr> [addr, ...]",
		      cmd_send_add_targets, 2, 31),
	SHELL_CMD_ARG(show, NULL, "Show target addresses", cmd_send_show_targets, 1, 0),
	SHELL_CMD_ARG(clean, NULL, "Clear target addresses", cmd_send_clean_targets, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_send_target(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return 0;
}

static int cmd_send_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	off_t offset;
	size_t size;

	offset = shell_strtoul(argv[1], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	size = shell_strtoul(argv[2], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	if (offset < STORAGE_START) {
		shell_error(shell, "Offset must be %#x or greater", STORAGE_START);
		return -1;
	} else if (size == 0) {
		shell_error(shell, "Size 0 are not allowed");
		return -1;
	} else if (offset + size > STORAGE_END) {
		shell_error(shell, "Allowed storage range: %#x - %#x", STORAGE_START, STORAGE_END);
		return -1;
	} else if (offset % STORAGE_ALIGN != 0) {
		shell_error(shell, "Offset must be aligned in %#x", STORAGE_ALIGN);
		return -1;
	}

	err = blob_tx_flash_init(offset, size);

	if (err) {
		shell_error(shell, "Failed to init BLOB IO Flash module: %d\n", err);
		return err;
	}

	shell_print(shell, "Set flash to send blob: offset: %#lx, size: %#x", offset, size);

	return 0;
}

static int cmd_send_start(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	int i;
	uint64_t blob_id;
	uint16_t base_timeout = 2;
	uint16_t group = BT_MESH_ADDR_UNASSIGNED;
	uint16_t appkey_index = 0;
	uint8_t ttl = BT_MESH_TTL_DEFAULT;
	enum bt_mesh_blob_xfer_mode mode = BT_MESH_BLOB_XFER_MODE_PUSH;

	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	for (i = 2; i < argc; i += 2) {
		if (!strncmp(argv[i], "-t", 2)) {
			ttl = shell_strtoul(argv[i + 1], 10, &err);
		} else if (!strncmp(argv[i], "-b", 2)) {
			base_timeout = shell_strtoul(argv[i + 1], 10, &err);
		} else if (!strncmp(argv[i], "-g", 2)) {
			group = shell_strtoul(argv[i + 1], 16, &err);
		} else if (!strncmp(argv[i], "-k", 2)) {
			appkey_index = shell_strtoul(argv[i + 1], 10, &err);
		} else if (!strncmp(argv[i], "-m", 2)) {
			if (!strncmp(argv[i + 1], "push", 4)) {
				mode = BT_MESH_BLOB_XFER_MODE_PUSH;
			} else if (!strncmp(argv[i + 1], "pull", 4)) {
				mode = BT_MESH_BLOB_XFER_MODE_PULL;
			} else {
				err = -EINVAL;
			}
		} else {
			shell_error(shell, "Wrong argument: %s", argv[i]);
			return err;
		}
		if (err) {
			shell_error(shell, "Unable to parse input string argument");
			return err;
		}
	}

	blob_id = shell_strtoull(argv[1], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	if (target_count == 0) {
		shell_error(shell, "No target. Should be added targets first.");
		return 1;
	}

	if (!blob_tx_flash_is_init()) {
		shell_error(shell, "No set flash. Should be set offset and size of flash to use.");
		return 1;
	}

	blob_tx_targets_init(targets, target_count);

	shell_print(shell,
		    "Set blob send ID: %#16.16llx, ttl: %#2.2x, base_timeout: %d\n, group: %4.4x, "
		    "appkey_index: %d, mode: %d", blob_id, ttl, base_timeout, group, appkey_index,
		     mode);

	err = blob_tx_start(blob_id, base_timeout, group, appkey_index, ttl, mode);

	if (err) {
		shell_error(shell, "Failed to start BLOB Transfer: %d", err);
		return err;
	}

	return 0;
}

static int cmd_send_cancel(const struct shell *shell, size_t argc, char *argv[])
{
	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	if (blob_tx_cancel()) {
		shell_error(shell, "BLOB Transfer TX is NOT in active");
		return 1;
	}

	shell_print(shell, "Cancel to send blob");

	return 0;
}

static int cmd_send_suspend(const struct shell *shell, size_t argc, char *argv[])
{
	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	if (blob_tx_suspend()) {
		shell_error(shell, "Failed to suspend");
		return 1;
	}

	shell_print(shell, "Suspend to send blob");

	return 0;
}

static int cmd_send_resume(const struct shell *shell, size_t argc, char *argv[])
{
	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	if (blob_tx_resume()) {
		shell_error(shell, "Failed to resume");
		return 1;
	}

	shell_print(shell, "Resume to send blob");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_send_cmds,
	SHELL_CMD_ARG(init, NULL, "Set flash area to send blob data. <offset> <size>",
		      cmd_send_init, 3, 0),
	SHELL_CMD(target, &blob_send_targerts_cmds, "Blob Transfer Target commands",
		  cmd_send_target),
	SHELL_CMD_ARG(start, NULL,
		      "Start to send blob data. <blob_id> [-t ttl] [-b base timeout] [-g multicast "
		      "address] [-k appkey index] [-m {push|pull}]",
		      cmd_send_start, 2, 10),
	SHELL_CMD(cancel, NULL, "Cancel transfer", cmd_send_cancel),
	SHELL_CMD(suspend, NULL, "Suspend transfer", cmd_send_suspend),
	SHELL_CMD(resume, NULL, "Resume transfer", cmd_send_resume),
	SHELL_SUBCMD_SET_END);

static int cmd_receive_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	off_t offset;

	offset = shell_strtoul(argv[1], 16, &err);
	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	if (offset < STORAGE_START) {
		shell_error(shell, "Offset must be %#x or greater", STORAGE_START);
		return -1;
	} else if (offset % STORAGE_ALIGN != 0) {
		shell_error(shell, "Offset must be aligned in %#x", STORAGE_ALIGN);
		return -1;
	}

	err = blob_rx_flash_init(offset);

	if (err) {
		shell_error(shell, "Failed to init BLOB IO Flash module: %d\n", err);
		return err;
	}

	shell_print(shell, "Set flash to receive blob: offset: %#lx", offset);

	return 0;
}

static int cmd_receive_start(const struct shell *shell, size_t argc, char *argv[])
{
	int i;
	int err = 0;
	uint64_t blob_id;
	uint16_t base_timeout = 2;
	uint8_t ttl = BT_MESH_TTL_DEFAULT;

	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	for (i = 2; i < argc; i += 2) {
		if (!strncmp(argv[i], "-t", 2)) {
			ttl = shell_strtoul(argv[i + 1], 10, &err);
		} else if (!strncmp(argv[i], "-b", 2)) {
			base_timeout = shell_strtoul(argv[i + 1], 10, &err);
		} else {
			shell_error(shell, "Wrong argument: %s", argv[i]);
			return err;
		}
		if (err) {
			shell_error(shell, "Unable to parse input string argument");
			return err;
		}
	}

	blob_id = shell_strtoull(argv[1], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	if (!blob_rx_flash_is_init()) {
		shell_error(shell, "No set flash. Should be set offset and size of flash to use.");
		return 1;
	}

	err = blob_rx_start(blob_id, base_timeout, ttl);

	if (err) {
		shell_error(shell, "Failed to start receiving blob: %d", err);
		return err;
	}

	shell_print(shell, "Set blob receive ID: %#16.16llx, ttl: %#2.2x, base_timeout: %d\n",
		    blob_id, ttl, base_timeout);

	return 0;
}

static int cmd_receive_cancel(const struct shell *shell, size_t argc, char *argv[])
{
	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	if (blob_rx_cancel()) {
		shell_error(shell, "Failed to cancel");
		return 1;
	}
	shell_print(shell, "Cancel to receive blob");

	return 0;
}

static int cmd_receive_progress(const struct shell *shell, size_t argc, char *argv[])
{
	int progress;

	if (!bt_mesh_is_provisioned()) {
		shell_print(shell, "The mesh node is not provisioned. Please provision the mesh "
				   "node before using the blob transfer.");
		return 1;
	}

	progress = blob_rx_progress();

	if (progress < 0) {
		shell_error(shell, "BLOB Transfer RX is NOT in active");
		return 1;
	}
	shell_print(shell, "Current transfer progress: %d", progress);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_receive_cmds,
	SHELL_CMD_ARG(init, NULL, "Set flash area to receive blob data. <offset>", cmd_receive_init,
		      2, 0),
	SHELL_CMD_ARG(start, NULL,
		      "Start to receive blob data. <blob_id> [-t ttl] [-b base_timeout]",
		      cmd_receive_start, 2, 4),
	SHELL_CMD(cancel, NULL, "Cancel transfer", cmd_receive_cancel),
	SHELL_CMD(progress, NULL, "Show progress of transfer", cmd_receive_progress),
	SHELL_SUBCMD_SET_END);

static int cmd_send(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return 0;
}

static int cmd_receive(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return 0;
}

static int _cmd_blob(blob_cb cb, const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	off_t offset;
	size_t size;

	offset = shell_strtoul(argv[1], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	size = shell_strtoul(argv[2], 16, &err);

	if (err) {
		shell_error(shell, "Unable to parse input string argument");
		return err;
	}

	if (offset < STORAGE_START) {
		shell_error(shell, "Offset must be %#x or greater", STORAGE_START);
		return -1;
	} else if (size == 0) {
		shell_error(shell, "Size 0 are not allowed");
		return -1;
	} else if (offset + size > STORAGE_END) {
		shell_error(shell, "Allowed storage range: %#x - %#x", STORAGE_START, STORAGE_END);
		return -1;
	} else if (offset % STORAGE_ALIGN != 0) {
		shell_error(shell, "Offset must be aligned in %#x", STORAGE_ALIGN);
		return -1;
	}

	cb(get_flash_area_id(), offset, size);

	return 0;
}

inline static int cmd_read_blob(const struct shell *shell, size_t argc, char *argv[])
{
	return _cmd_blob(read_flash_area, shell, argc, argv);
}

inline static int cmd_write_blob(const struct shell *shell, size_t argc, char *argv[])
{
	return _cmd_blob(write_flash_area, shell, argc, argv);
}

inline static int cmd_hash_blob(const struct shell *shell, size_t argc, char *argv[])
{
	return _cmd_blob(hash_flash_area, shell, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_cmds, SHELL_CMD(send, &blob_send_cmds, "Send Binary Large Object", cmd_send),
	SHELL_CMD(receive, &blob_receive_cmds, "Receive Binary Large Object", cmd_receive),
	SHELL_CMD_ARG(read, NULL, "Read flash area <offset> <size>", cmd_read_blob, 3, 0),
	SHELL_CMD_ARG(write, NULL, "Write data to flash <offset> <size>", cmd_write_blob, 3, 0),
	SHELL_CMD_ARG(hash, NULL, "Hash of data in flash <offset> <size>", cmd_hash_blob, 3, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_blob(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(blob, &blob_cmds, "Bluetooth Mesh Blob Transfer commands", cmd_blob, 1, 1);

void blob_shell_init(void)
{
	blob_shell = shell_backend_uart_get_ptr();
	shell_print(blob_shell, ">>> Bluetooth Mesh Blob Transfer sample <<<");
	target_count = 0;
}
