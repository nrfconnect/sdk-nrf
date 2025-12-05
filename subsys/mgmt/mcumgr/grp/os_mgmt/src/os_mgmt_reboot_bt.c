/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <pm_config.h>
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

static enum mgmt_cb_return reboot_bt_hook(uint32_t event, enum mgmt_cb_return prev_status,
					  int32_t *rc, uint16_t *group, bool *abort_more,
					  void *data, size_t data_size)
{
	int err_rc;
	struct os_mgmt_reset_data *reboot_data = (struct os_mgmt_reset_data *)data;

	if (event != MGMT_EVT_OP_OS_MGMT_RESET || data_size != sizeof(*reboot_data)) {
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

#ifdef CONFIG_BT
	err_rc = bt_disable();
	if (err_rc) {
		LOG_ERR("BT disable failed before reboot: %d", err_rc);
	}
#endif
#ifdef CONFIG_MPSL
	mpsl_uninit();
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
