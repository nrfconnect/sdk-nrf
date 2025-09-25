/*
 * Copyright (c) 2023-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>

#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/sys/math_extras.h>

#include <app_event_manager.h>
#include "dfu_lock.h"

#define MODULE dfu_mcumgr
#include <caf/events/module_state_event.h>
#include <caf/events/ble_smp_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_DFU_MCUMGR_LOG_LEVEL);

#define DFU_TIMEOUT	K_SECONDS(5)

static struct k_work_delayable dfu_timeout;

static atomic_t mcumgr_event_active = ATOMIC_INIT(false);

/* nRF Desktop MCUmgr DFU module cannot be enabled together with the CAF BLE SMP module. */
BUILD_ASSERT(!IS_ENABLED(CONFIG_CAF_BLE_SMP));

static void dfu_lock_owner_changed(const struct dfu_lock_owner *new_owner)
{
	LOG_DBG("MCUmgr progress reset due to the different DFU owner: %s", new_owner->name);

#ifdef CONFIG_DESKTOP_DFU_LOCK
	/* The function declaration is not included in MCUmgr's header file if the mutex locking
	 * of the image management state object is disabled.
	 */
	img_mgmt_reset_upload();
#endif /* CONFIG_DESKTOP_DFU_LOCK */
}

const static struct dfu_lock_owner mcumgr_owner = {
	.name = "MCUmgr",
	.owner_changed = dfu_lock_owner_changed,
};

static void dfu_timeout_handler(struct k_work *work)
{
	LOG_WRN("MCUmgr DFU timed out");

	if (IS_ENABLED(CONFIG_DESKTOP_DFU_LOCK)) {
		int err;

		err = dfu_lock_release(&mcumgr_owner);
		__ASSERT_NO_MSG(!err);
	}
}

static bool smp_cmd_is_dfu_cmd(const struct mgmt_evt_op_cmd_arg *cmd)
{
	const char *smp_cmd_name = "Unknown";
	bool dfu_transfer_cmd = true;

	if (cmd->group == MGMT_GROUP_ID_IMAGE) {
		smp_cmd_name = "Image Management";
	} else if ((cmd->group == MGMT_GROUP_ID_OS) && (cmd->id == OS_MGMT_ID_RESET)) {
		smp_cmd_name = "OS Management Reset";
	} else {
		dfu_transfer_cmd = false;
	}

	if (dfu_transfer_cmd && smp_cmd_name) {
		LOG_DBG("MCUmgr %s event", smp_cmd_name);
	}

	return dfu_transfer_cmd;
}

static enum mgmt_cb_return smp_cmd_recv(uint32_t event, enum mgmt_cb_return prev_status,
					int32_t *rc, uint16_t *group, bool *abort_more,
					void *data, size_t data_size)
{
	const struct mgmt_evt_op_cmd_arg *cmd_recv;

	if (event != MGMT_EVT_OP_CMD_RECV) {
		LOG_ERR("Spurious event in recv cb: %" PRIu32, event);
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	if (data_size != sizeof(*cmd_recv)) {
		LOG_ERR("Invalid data size in recv cb: %zu (expected: %zu)",
			data_size, sizeof(*cmd_recv));
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	cmd_recv = data;

	LOG_DBG("MCUmgr SMP Command Recv Event: group_id=%d cmd_id=%d",
		cmd_recv->group, cmd_recv->id);

	/* Ignore commands not related to DFU over SMP. */
	if (!smp_cmd_is_dfu_cmd(cmd_recv)) {
		return MGMT_CB_OK;
	}

	k_work_reschedule(&dfu_timeout, DFU_TIMEOUT);
	if (IS_ENABLED(CONFIG_DESKTOP_DFU_LOCK) && dfu_lock_claim(&mcumgr_owner)) {
		(void)k_work_cancel_delayable(&dfu_timeout);
		*rc = MGMT_ERR_EACCESSDENIED;
		return MGMT_CB_ERROR_RC;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    atomic_cas(&mcumgr_event_active, false, true)) {
		APP_EVENT_SUBMIT(new_ble_smp_transfer_event());
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback cmd_recv_cb = {
	.callback = smp_cmd_recv,
	.event_id = MGMT_EVT_OP_CMD_RECV,
};

static void dfu_backend_init(void)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_DFU_MCUMGR_MCUBOOT_DIRECT_XIP)) {
		int err = boot_write_img_confirmed();

		if (err) {
			LOG_ERR("Cannot confirm a running image: %d", err);
		}
	}

	k_work_init_delayable(&dfu_timeout, dfu_timeout_handler);

	mgmt_callback_register(&cmd_recv_cb);

	LOG_INF("MCUboot image version: %s", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    is_ble_smp_transfer_event(aeh)) {
		bool res = atomic_cas(&mcumgr_event_active, true, false);

		__ASSERT_NO_MSG(res);
		ARG_UNUSED(res);

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			dfu_backend_init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_MCUMGR_TRANSPORT_BT
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_smp_transfer_event);
#endif
