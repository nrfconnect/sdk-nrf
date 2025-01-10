/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <pm_config.h>
#include <fw_info.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>

static enum mgmt_cb_return bootloader_info_hook(uint32_t event, enum mgmt_cb_return prev_status,
						int32_t *rc, uint16_t *group, bool *abort_more,
						void *data, size_t data_size)
{
	struct os_mgmt_bootloader_info_data *bootloader_info_data =
						(struct os_mgmt_bootloader_info_data *)data;

	if (event != MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO || data_size !=
	    sizeof(*bootloader_info_data)) {
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	if (*(bootloader_info_data->decoded) >= 1 && (sizeof("active_b0_slot") - 1) ==
	    bootloader_info_data->query->len && memcmp("active_b0_slot",
						       bootloader_info_data->query->value,
						       bootloader_info_data->query->len) == 0) {
		const struct fw_info *s0_info = fw_info_find(PM_S0_ADDRESS);
		const struct fw_info *s1_info = fw_info_find(PM_S1_ADDRESS);

		if (s0_info || s1_info) {
			uint32_t active_slot;
			bool ok;

			if (!s1_info || (s0_info && s0_info->version >= s1_info->version)) {
				active_slot = 0;
			} else if (!s0_info || (s1_info && s1_info->version > s0_info->version)) {
				active_slot = 1;
			}

			ok = zcbor_tstr_put_lit(bootloader_info_data->zse, "active") &&
			     zcbor_uint32_put(bootloader_info_data->zse, active_slot);
			*rc = (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
			return MGMT_CB_ERROR_RC;
		}

		/* Return response not valid error when active slot cannot be determined */
		*group = MGMT_GROUP_ID_OS;
		*rc = OS_MGMT_ERR_QUERY_RESPONSE_VALUE_NOT_VALID;
		return MGMT_CB_ERROR_ERR;
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback cmd_bootloader_info_cb = {
	.callback = bootloader_info_hook,
	.event_id = MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO,
};

static int os_mgmt_register_bootloader_info_hook_b0_active_slot(void)
{
	mgmt_callback_register(&cmd_bootloader_info_cb);
	return 0;
}

SYS_INIT(os_mgmt_register_bootloader_info_hook_b0_active_slot, APPLICATION, 0);
