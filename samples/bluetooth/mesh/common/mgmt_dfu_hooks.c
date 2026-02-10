/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mgmt_dfu_hooks.c
 * @brief MCUmgr DFU hooks to properly reset BT Mesh before storage erase.
 * This ensures PSA crypto keys are destroyed before settings storage
 * is erased, preventing key ID desync on next boot.
 */
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/zephyr/zephyr_basic.h>
#include <zephyr/bluetooth/mesh.h>

/* Flag to track if storage erase was requested */
static bool storage_erase_pending;

/*
 * Callback for SMP command receive events.
 * Detects when storage erase is requested.
 */
static enum mgmt_cb_return cmd_recv_hook(uint32_t event,
					 enum mgmt_cb_return prev_status,
					 int32_t *rc, uint16_t *group,
					 bool *abort_more, void *data,
					 size_t data_size)
{
	struct mgmt_evt_op_cmd_arg *cmd = (struct mgmt_evt_op_cmd_arg *)data;

	/* Check if this is the storage erase command */
	if (cmd->group == ZEPHYR_MGMT_GRP_BASIC &&
	    cmd->id == ZEPHYR_MGMT_GRP_BASIC_CMD_ERASE_STORAGE) {
		printk("Storage erase requested - will reset mesh before reboot");
		storage_erase_pending = true;
	}

	return MGMT_CB_OK;
}

/*
 * Callback for OS reset events.
 * Performs mesh reset just before device reboot to clean up PSA keys,
 * then erases storage again to ensure clean NVS state.
 */
static enum mgmt_cb_return os_reset_hook(uint32_t event,
					 enum mgmt_cb_return prev_status,
					 int32_t *rc, uint16_t *group,
					 bool *abort_more, void *data,
					 size_t data_size)
{
	/* Reset is about to happen - now it's safe to reset mesh
	 * since the connection will be terminated anyway
	 */
	if (storage_erase_pending && bt_mesh_is_provisioned()) {
		printk("Resetting BT Mesh before reboot to clean up PSA keys");
		bt_mesh_reset();
		printk("BT Mesh reset complete");

		storage_erase_pending = false;
	}

	return MGMT_CB_OK;
}

/* Separate callbacks for different event groups - they cannot be OR'd together */
static struct mgmt_callback cmd_recv_callback = {
	.callback = cmd_recv_hook,
	.event_id = MGMT_EVT_OP_CMD_RECV,
};

static struct mgmt_callback os_reset_callback = {
	.callback = os_reset_hook,
	.event_id = MGMT_EVT_OP_OS_MGMT_RESET,
};

static int mgmt_dfu_hooks_init(void)
{
	mgmt_callback_register(&cmd_recv_callback);
	mgmt_callback_register(&os_reset_callback);
	return 0;
}

/* Initialize after application init (priority 90), use 91 */
SYS_INIT(mgmt_dfu_hooks_init, APPLICATION, 91);
