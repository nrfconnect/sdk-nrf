/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_common.h>
#include <zcbor_encode.h>

#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include "suitfu_mgmt_priv.h"

static enum mgmt_cb_return bootloader_info_hook(uint32_t event, enum mgmt_cb_return prev_status,
					int32_t *rc, uint16_t *group, bool *abort_more,
					void *data, size_t data_size)
{
	struct os_mgmt_bootloader_info_data *bootloader_info_data;

	if ((event != MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO)
	    || (data_size != sizeof(*bootloader_info_data))) {
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	bootloader_info_data = data;

	/* If no parameter is recognized then just introduce the bootloader. */
	if (*(bootloader_info_data->decoded) == 0) {
		bool ok = zcbor_tstr_put_lit(bootloader_info_data->zse, "bootloader") &&
		     zcbor_tstr_put_lit(bootloader_info_data->zse, "SUIT");

		*rc = (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
	} else {
		*rc = MGMT_ERR_ENOTSUP;
	}

	return MGMT_CB_ERROR_RC;
}

static struct mgmt_callback cmd_bootloader_info_cb = {
	.callback = bootloader_info_hook,
	.event_id = MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO,
};

void suitfu_mgmt_register_bootloader_info_hook(void)
{
	mgmt_callback_register(&cmd_bootloader_info_cb);
}
