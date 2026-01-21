/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#endif
#ifdef CONFIG_MPSL
#include <mpsl.h>
#endif

LOG_MODULE_REGISTER(os_mgmt_reboot_bt, CONFIG_MCUMGR_GRP_OS_LOG_LEVEL);

#ifdef CONFIG_MULTITHREADING
static void bt_disable_work_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(bt_disable_work, bt_disable_work_handler);

static void bt_disable_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

#ifdef CONFIG_BT
	int err_rc = bt_disable();

	if (err_rc) {
		LOG_ERR("BT disable failed before reboot: %d", err_rc);
	}
#endif
#if defined(CONFIG_MPSL) && !defined(CONFIG_BT_UNINIT_MPSL_ON_DISABLE)
	mpsl_uninit();
#endif
}
#endif

static enum mgmt_cb_return reboot_bt_hook(uint32_t event, enum mgmt_cb_return prev_status,
					  int32_t *rc, uint16_t *group, bool *abort_more,
					  void *data, size_t data_size)
{
	struct os_mgmt_reset_data *reboot_data = (struct os_mgmt_reset_data *)data;

	if (event != MGMT_EVT_OP_OS_MGMT_RESET || data_size != sizeof(*reboot_data)) {
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}
#ifdef CONFIG_MULTITHREADING
	/* disable bluetooth from the system workqueue thread. */
	k_work_schedule(&bt_disable_work, K_MSEC(CONFIG_MCUMGR_GRP_OS_RESET_MS/2));
#else
#ifdef CONFIG_BT
	int err_rc = bt_disable();

	if (err_rc) {
		LOG_ERR("BT disable failed before reboot: %d", err_rc);
	}
#endif
#if defined(CONFIG_MPSL) && !defined(CONFIG_BT_UNINIT_MPSL_ON_DISABLE)
	mpsl_uninit();
#endif
#endif

	return MGMT_CB_OK;
}

static struct mgmt_callback cmd_reboot_bt_info_cb  = {
	.callback = reboot_bt_hook,
	.event_id = MGMT_EVT_OP_OS_MGMT_RESET,
};

static int os_mgmt_register_reboot_bt(void)
{
	mgmt_callback_register(&cmd_reboot_bt_info_cb);
	return 0;
}

SYS_INIT(os_mgmt_register_reboot_bt, APPLICATION, 0);
